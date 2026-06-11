require "json"
require "../game_client"
require "../map_renderer"

module Ctp2Gateway::Mcp
  # One MCP tool: name + description (written FOR the model — they are the
  # only documentation the LLM sees), a JSON Schema for the arguments, and
  # the handler mapping arguments onto game verbs.
  record Tool,
    name : String,
    description : String,
    schema : String,
    run : Proc(Ctp2Gateway::GameClient, JSON::Any, Ctp2Gateway::GameClient::Result)

  NO_ARGS = %({"type":"object","properties":{},"additionalProperties":false})

  private def self.simple(client, verb) : Ctp2Gateway::GameClient::Result
    client.command(verb)
  end

  # Hard cap per end_turn call: rounds cost ~0.5s each and the game-socket
  # timeout is 30s — the model is told to call repeatedly for longer spans.
  MAX_TURNS_PER_CALL = 20

  TOOLS = [
    Tool.new("start_game",
      "Create and start a new CTP2 game (generates the world; takes a few seconds). " \
      "You play the single human player; the other players are AI. " \
      "Begin here unless a game is already running (check with query_turn).",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        # Frontend pairing: new_game prepares, start_game initializes.
        r = c.command("new_game")
        r.is_a?(Ctp2Gateway::GameClient::Err) ? r : c.command("start_game")
      }),

    Tool.new("end_turn",
      "Advance time by N full rounds (every player, including AI, takes a turn). " \
      "Production, growth and AI moves happen here. Max #{MAX_TURNS_PER_CALL} rounds " \
      "per call — call repeatedly for longer spans. Check results with query_turn / query_cities.",
      %({"type":"object","properties":{"turns":{"type":"integer","minimum":1,"maximum":#{MAX_TURNS_PER_CALL},"description":"rounds to advance (default 1)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        turns = (a["turns"]?.try(&.as_i?) || 1).clamp(1, MAX_TURNS_PER_CALL)
        c.command("end_turn #{turns}")
      }),

    Tool.new("build_city",
      "Found a city with your first settler-capable army AT ITS CURRENT TILE. " \
      "Errors: no_settler_found (build one via set_production), tile_occupied " \
      "(a city already stands here — move_army the settler away first), " \
      "settle_rejected (too close to another city — keep distance >= 3 — or bad terrain).",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "build_city") }),

    Tool.new("query_armies",
      "Your armies — index (use with move_army/auto_explore), position, movement " \
      "points left this turn, can_settle, and member units.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_armies") }),

    Tool.new("move_army",
      "Move one of your armies toward (x,y) via pathfinding. no_path means the " \
      "destination is UNEXPLORED, impassable, or unreachable — you can only plot " \
      "through explored tiles (see query_map), so walk in short hops and let vision " \
      "expand. Most units move ~1 tile per turn: check `arrived` in the result, and " \
      "if false, end_turn and re-issue. Typical expansion: march a settler to " \
      "distance >= 3 from any city, then build_city.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0,"description":"from query_armies"},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("move_army #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("auto_explore",
      "Put one of your armies on auto-explore: it keeps picking new targets each " \
      "turn, revealing the map without manual driving. Good for a cheap scout.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("auto_explore #{a["army_index"].as_i}")
      }),

    Tool.new("set_production",
      "Set what one of YOUR cities builds. `what` is \"settler\", \"cheapest_military\", " \
      "a numeric unit type id, \"building <id>\" (city improvements from query_city's " \
      "buildable_buildings — a GRANARY is the growth lever on poor terrain), or " \
      "\"clear\" to STOP producing. Production loops forever otherwise — idle armies " \
      "drain score via upkeep, and each settler consumes a population point.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0,"description":"index into your city list (see query_cities)"},"what":{"type":"string","description":"settler | cheapest_military | unit type id | building <id> | clear"}},"required":["city_index","what"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_production #{a["city_index"].as_i} #{a["what"].as_s}")
      }),

    Tool.new("save_game",
      "Save the game to a file path (JSON format). Use as a checkpoint before risky experiments.",
      %({"type":"object","properties":{"path":{"type":"string","description":"absolute file path, e.g. /tmp/checkpoint.json"}},"required":["path"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("save_game #{a["path"].as_s}") }),

    Tool.new("load_game",
      "Load a previously saved game. Restores the full world state — combine with " \
      "save_game for checkpoint/rollback debugging.",
      %({"type":"object","properties":{"path":{"type":"string"}},"required":["path"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("load_game #{a["path"].as_s}") }),

    Tool.new("query_turn",
      "The clock: round (full rounds completed) and calendar year. Cheap — use freely.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_turn") }),

    Tool.new("query_players",
      "ADMIN (omniscient): every player — id, leader name, civ, country, human/AI, " \
      "alive/dead, gold, city/unit/army counts, government, score.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_players") }),

    Tool.new("query_player",
      "ADMIN (omniscient): one player slot in the same shape as query_players entries.",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_player #{a["player_id"].as_i}") }),

    Tool.new("query_player_cities",
      "ADMIN (omniscient): ALL cities of one player, fog disregarded, including " \
      "per-turn yields (food, production, gold, science, happiness).",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_player_cities #{a["player_id"].as_i}") }),

    Tool.new("query_cities",
      "PLAYER VIEW (fog-of-war filtered): cities you can see — always your own, " \
      "enemy cities only when their tile is visible.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_cities") }),

    Tool.new("query_city",
      "PLAYER VIEW: detail for one of YOUR cities by index, including the buildable-unit " \
      "list (what set_production accepts) and per-turn yields.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0}},"required":["city_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_city #{a["city_index"].as_i}") }),

    Tool.new("query_units",
      "PLAYER VIEW (fog-of-war filtered): all units you can see — owner, type, name, " \
      "position, hp.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_units") }),

    Tool.new("query_map",
      "PLAYER VIEW: every tile you have explored — terrain, currently-visible flag, " \
      "city markers. Output can be LARGE on a well-explored map; prefer targeted " \
      "queries when you only need cities or units.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_map") }),

    Tool.new("query_research",
      "The science affordance set: what you're researching, what you could research " \
      "and each option's cost. RESEARCH IS THE MAIN SCORE ENGINE — advances unlock " \
      "units, buildings and terraforming. Keep something queued at all times.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_research") }),

    Tool.new("set_research",
      "Switch research to an advance id from query_research's available list. " \
      "Errors: already_known, prerequisites_missing.",
      %({"type":"object","properties":{"advance_id":{"type":"integer","minimum":0}},"required":["advance_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_research #{a["advance_id"].as_i}")
      }),

    Tool.new("set_material_tax",
      "Divert a percentage of city production into the Public Works pool that pays " \
      "for terraforming. Without this your PW stays 0 and terraform is unusable. " \
      "20-40% is a reasonable working rate.",
      %({"type":"object","properties":{"percent":{"type":"integer","minimum":0,"maximum":100}},"required":["percent"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_material_tax #{a["percent"].as_i}")
      }),

    Tool.new("query_terraform",
      "Terraform options for ONE tile: what it can become, the Public Works price, " \
      "and your PW balance. Only works INSIDE your borders (check tile_owner), and " \
      "each transform is gated by advances — e.g. draining swamp needs Industrial " \
      "Revolution. If your cities can't grow (low food), terraforming toward " \
      "Grassland/Plains is the long-term fix.",
      %({"type":"object","properties":{"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("query_terraform #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("terraform",
      "Spend Public Works to transform a tile (improvement_id from query_terraform's " \
      "options). The change completes after the option's `turns` — keep ending turns.",
      %({"type":"object","properties":{"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0},"improvement_id":{"type":"integer","minimum":0}},"required":["x","y","improvement_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("terraform #{a["x"].as_i} #{a["y"].as_i} #{a["improvement_id"].as_i}")
      }),

    Tool.new("render_map",
      "YOUR EYES: an ASCII map of the explored world — terrain glyphs, coordinate " \
      "rulers, cities as letters, @ = your armies, ! = foreign units, · = unexplored. " \
      "Read it to plan movement and expansion. Defaults to a viewport around your " \
      "first city; pass center_x/center_y/radius to look elsewhere.",
      %({"type":"object","properties":{"center_x":{"type":"integer","minimum":0},"center_y":{"type":"integer","minimum":0},"radius":{"type":"integer","minimum":4,"maximum":30,"description":"half-width of the viewport (default 14)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        r = Ctp2Gateway::MapRenderer.new(c)
        text = r.render(
          a["center_x"]?.try(&.as_i?),
          a["center_y"]?.try(&.as_i?),
          a["radius"]?.try(&.as_i?) || 14)
        if text
          Ctp2Gateway::GameClient::Ok.new(JSON::Any.new(text))
        else
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({status: "error", detail: r.error || "render failed"}.to_json))
        end
      }),

    Tool.new("suggest_settle_spots",
      "Where to found the next city: explored, passable land tiles at distance >= 3 " \
      "from every known city, scored FOOD-FIRST (3*food + shields + gold) over the " \
      "tile and its explored neighbours INCLUDING water (kelp beds and beaches feed " \
      "coastal cities) — highest score first. Score is population and population is " \
      "food. March a settler there with move_army, then build_city.",
      %({"type":"object","properties":{"max":{"type":"integer","minimum":1,"maximum":10,"description":"how many candidates (default 5)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        r = Ctp2Gateway::MapRenderer.new(c)
        spots = r.settle_spots(a["max"]?.try(&.as_i?) || 5)
        if spots
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({
            status: "ok",
            spots:  spots.map { |s| {x: s.x, y: s.y, score: s.score, distance_to_nearest_city: s.dist, terrain: s.terrain} },
            note:   spots.empty? ? "no qualifying tiles explored yet — explore further (auto_explore) before settling" : nil,
          }.to_json))
        else
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({status: "error", detail: r.error || "advisor failed"}.to_json))
        end
      }),

    Tool.new("unload",
      "Amphibious landing: order a transport army (boats carrying cargo — see " \
      "query_armies' cargo field) to disembark everyone onto an ADJACENT land tile. " \
      "To BOARD, move a land army onto your transport's water tile with move_army " \
      "(allowed when the transport has free cargo_capacity).",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("unload #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("group_army",
      "Merge every unit on the army's tile into ONE stack (up to 12 fight as one " \
      "army). ESSENTIAL before combat: single-unit armies attacking a stack die one " \
      "by one — build units in a city, then group the army standing there. " \
      "ungroup via raw_cmd 'ungroup_army <idx>'.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("group_army #{a["army_index"].as_i}")
      }),

    Tool.new("fortify",
      "Entrench an army in place for a defensive bonus. Garrisons that merely stand " \
      "in a city take full damage — fortify them. Moving the army breaks the " \
      "entrenchment.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("fortify #{a["army_index"].as_i}")
      }),

    Tool.new("declare_war",
      "Formally declare war on a player (id from query_players). Requires CONTACT — " \
      "you must have met them (their units/cities seen by yours). Check the contact/" \
      "at_war fields in query_players. War enables the attack tool; conquest is worth " \
      "more score than anything else on a bad start.",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("declare_war #{a["player_id"].as_i}")
      }),

    Tool.new("attack",
      "Order an army onto an ADJACENT enemy-occupied tile — the move resolves combat; " \
      "if the defenders die and a city stands there, you capture it. Requires war " \
      "(declare_war). March with move_army first; attack only closes the last tile. " \
      "Check the result: army_survived, captured_tile, defenders_left.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("attack #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("raw_cmd",
      "Escape hatch: send a raw verb line to the game's command socket (one line, " \
      "e.g. \"query_city 0\"). See GET /api on the gateway for the verb inventory.",
      %({"type":"object","properties":{"cmd":{"type":"string"}},"required":["cmd"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command(a["cmd"].as_s) }),

    Tool.new("gateway_health",
      "Gateway/observability state: game-socket connection, spawned game process, " \
      "and the recent command journal. Use when game tools fail unexpectedly.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        Ctp2Gateway::GameClient::Ok.new(JSON.parse({
          socket:     c.socket_path,
          connected:  c.connected?,
          last_error: c.last_error,
          spawn:      Ctp2Gateway::Config.process.try(&.status_json),
          recent_exchanges: c.recent_exchanges.first(10).map do |e|
            {at: e.at.to_rfc3339(fraction_digits: 3), cmd: e.cmd, ok: e.ok, detail: e.detail}
          end,
        }.to_json))
      }),
  ]

  def self.find(name : String) : Tool?
    TOOLS.find { |t| t.name == name }
  end
end
