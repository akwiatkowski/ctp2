require "json"
require "./game_client"

module Ctp2Gateway
  # Composes query_map + query_cities + query_armies + query_units +
  # query_terrains into an ASCII viewport an LLM (or a human in /map) can
  # actually reason over — coordinate rulers, terrain glyphs, city letters,
  # '@' for own armies, '!' for foreign units, '·' for fog. LLMs are poor at
  # reconstructing 2D space from coordinate lists but good at reading
  # character grids; this is the "eyes" of the play loop.
  #
  # Also home of the settle advisor: the spatial math (distances, yield
  # neighbourhoods) models are worst at, done in code.
  class MapRenderer
    record Spot, x : Int32, y : Int32, score : Int32, dist : Int32, terrain : String

    MAX_RADIUS = 30

    def initialize(@client : GameClient)
    end

    # --- data plumbing -----------------------------------------------------

    # Run a query, return its "result" or set @error once.
    private def fetch(cmd : String) : JSON::Any?
      return nil if @error
      case r = @client.command(cmd)
      in GameClient::Ok
        if r.payload["status"]?.try(&.as_s?) == "ok"
          r.payload["result"]?
        else
          @error = "game says: #{r.payload["detail"]? || "error"} (#{cmd})"
          nil
        end
      in GameClient::Err
        @error = "#{r.kind.to_s.underscore}: #{r.detail} (#{cmd})"
        nil
      end
    end

    @error : String? = nil

    getter error : String?

    private def terrain_glyph(name : String, water : Bool, mountain : Bool) : Char
      n = name.downcase
      return '≈' if n.includes?("deep water")
      return '~' if water
      return '^' if mountain || n.includes?("mountain")
      return 'h' if n.includes?("hill")
      return ',' if n.includes?("grass")
      return '.' if n.includes?("plains")
      return 'f' if n.includes?("forest")
      return 'j' if n.includes?("jungle")
      return 's' if n.includes?("swamp")
      return 'd' if n.includes?("desert")
      return 't' if n.includes?("tundra")
      return '*' if n.includes?("glacier")
      n[0]? || '?'
    end

    # --- the map -----------------------------------------------------------

    # Render the explored world around (center_x, center_y) — defaults to the
    # first own city, else the first army. Returns the ASCII text, or nil
    # with #error set.
    def render(center_x : Int32? = nil, center_y : Int32? = nil, radius : Int32 = 14) : String?
      radius = radius.clamp(4, MAX_RADIUS)

      map = fetch("query_map")
      terrains = fetch("query_terrains").try(&.["terrains"].as_a)
      cities = fetch("query_cities").try(&.["cities"].as_a)
      armies = fetch("query_armies").try(&.["armies"].as_a)
      units = fetch("query_units").try(&.["units"].as_a)
      return nil unless map && terrains && cities && armies && units

      me = map["visible_player"].as_i
      tiles = {} of {Int32, Int32} => JSON::Any
      map["tiles"].as_a.each { |t| tiles[{t["x"].as_i, t["y"].as_i}] = t }
      if tiles.empty?
        @error = "nothing explored yet — found a city or move a unit first"
        return nil
      end

      tinfo = {} of Int32 => {Char, String}
      terrains.each do |t|
        tinfo[t["id"].as_i] = {
          terrain_glyph(t["name"].as_s, t["water"].as_bool, t["mountain"].as_bool),
          t["name"].as_s,
        }
      end

      # Viewport center: explicit > first own city > first army > bbox middle.
      cx = center_x || cities.find { |c| c["owner"].as_i == me }.try(&.["pos"]["x"].as_i) ||
           armies.first?.try(&.["pos"]["x"].as_i) || tiles.keys.sum(&.[0]) // tiles.size
      cy = center_y || cities.find { |c| c["owner"].as_i == me }.try(&.["pos"]["y"].as_i) ||
           armies.first?.try(&.["pos"]["y"].as_i) || tiles.keys.sum(&.[1]) // tiles.size

      x0 = {cx - radius, 0}.max
      y0 = {cy - radius, 0}.max
      x1 = {cx + radius, map["width"].as_i - 1}.min
      y1 = {cy + radius, map["height"].as_i - 1}.min

      # Overlays. City letters: first letter of the name, next free A-Z on
      # collision; '@' own armies; '!' foreign units.
      grid_city = {} of {Int32, Int32} => Char
      legend_cities = [] of String
      used = Set(Char).new
      cities.each do |c|
        pos = {c["pos"]["x"].as_i, c["pos"]["y"].as_i}
        letter = c["name"].as_s.upcase[0]? || 'C'
        letter = ('A'..'Z').find { |ch| !used.includes?(ch) } || '#' if used.includes?(letter)
        used << letter
        grid_city[pos] = letter
        owner = c["owner"].as_i
        legend_cities << "#{letter}=#{c["name"].as_s}(#{pos[0]},#{pos[1]})#{owner == me ? "" : " owner #{owner}"}"
      end
      own_army = Set({Int32, Int32}).new
      armies.each { |a| own_army << {a["pos"]["x"].as_i, a["pos"]["y"].as_i} }
      foreign = Set({Int32, Int32}).new
      units.each do |u|
        foreign << {u["pos"]["x"].as_i, u["pos"]["y"].as_i} if u["owner"].as_i != me
      end

      used_glyphs = {} of Char => String
      out = String.build do |io|
        io << "viewport (" << x0 << ".." << x1 << ") x (" << y0 << ".." << y1 << ")"
        io << " · you are player " << me << "\n\n"
        # Two-line x ruler: tens, then units.
        io << "    "
        (x0..x1).each { |x| io << (x % 10 == 0 ? (x // 10) % 10 : ' ') }
        io << "\n    "
        (x0..x1).each { |x| io << x % 10 }
        io << "\n"
        (y0..y1).each do |y|
          io << y.to_s.rjust(3) << ' '
          (x0..x1).each do |x|
            pos = {x, y}
            ch = if c = grid_city[pos]?
                   c
                 elsif own_army.includes?(pos)
                   '@'
                 elsif foreign.includes?(pos)
                   '!'
                 elsif t = tiles[pos]?
                   g, name = tinfo[t["terrain"].as_i]? || {'?', "unknown"}
                   used_glyphs[g] = name
                   g
                 else
                   '·'
                 end
            io << ch
          end
          io << "\n"
        end
        io << "\nlegend: ·=unexplored @=your army !=foreign unit "
        io << used_glyphs.map { |g, n| "#{g}=#{n}" }.join(" ")
        io << "\ncities: " << (legend_cities.empty? ? "none known" : legend_cities.join(" · "))
        io << "\nyour armies: "
        if armies.empty?
          io << "none"
        else
          io << armies.map { |a|
            "##{a["index"].as_i}@(#{a["pos"]["x"].as_i},#{a["pos"]["y"].as_i})" \
            "#{a["can_settle"].as_bool ? " settler" : ""}"
          }.join(" · ")
        end
      end
      out
    end

    # --- the settle advisor --------------------------------------------------

    # Candidate city sites: explored, land, not mountain, Chebyshev >= 3 from
    # every known city, scored by the summed base yields of the tile and its
    # explored neighbours. The model gets a menu instead of doing geometry.
    def settle_spots(max : Int32 = 5) : Array(Spot)?
      map = fetch("query_map")
      terrains = fetch("query_terrains").try(&.["terrains"].as_a)
      cities = fetch("query_cities").try(&.["cities"].as_a)
      return nil unless map && terrains && cities

      land = {} of Int32 => {Bool, Int32, String} # id => {settleable, yield, name}
      terrains.each do |t|
        ok = t["land"].as_bool && !t["mountain"].as_bool && !t["water"].as_bool
        score = t["food"].as_i + t["shields"].as_i + t["gold"].as_i
        land[t["id"].as_i] = {ok, score, t["name"].as_s}
      end

      tiles = {} of {Int32, Int32} => Int32
      map["tiles"].as_a.each { |t| tiles[{t["x"].as_i, t["y"].as_i}] = t["terrain"].as_i }
      city_pos = cities.map { |c| {c["pos"]["x"].as_i, c["pos"]["y"].as_i} }

      spots = [] of Spot
      tiles.each do |(x, y), tid|
        ok, _, name = land[tid]? || {false, 0, ""}
        next unless ok
        dist = city_pos.min_of? { |(cx2, cy2)| {(x - cx2).abs, (y - cy2).abs}.max } || 99
        next if dist < 3
        score = 0
        (-1..1).each do |dx|
          (-1..1).each do |dy|
            if nt = tiles[{x + dx, y + dy}]?
              score += land[nt]?.try(&.[1]) || 0
            end
          end
        end
        spots << Spot.new(x, y, score, dist, name)
      end
      spots.sort_by! { |s| {-s.score, s.dist} }
      spots.first(max)
    end
  end
end
