require "json"
require "./game_client"

module Ctp2Gateway
  # Turns the engine's Action Log (the `log_get` verb) into a readable "story
  # of the civilization": a timelapse of the meaningful beats — cities founded,
  # captured and lost, wonders, ages, advances, battles, drama and diplomacy —
  # grouped into chapters by round.
  #
  # The raw log also carries low-level orders (move, entrench, group, ...) and
  # high-frequency bookkeeping (every pop gain, every unit built). The story
  # FILTERS to narrative beats (STORY_EVENTS) so it reads like a chronicle,
  # not a command transcript. C++ stays the raw data source; all narration
  # lives here in Crystal (matches the gateway's render_map / settle-advisor
  # split).
  class StoryReducer
    # One round's worth of narrated beats.
    record Chapter, round : Int32, lines : Array(String)

    # The assembled chronicle (or an error explaining the empty result).
    record Story,
      error : String?,
      total_events : Int32, # entries in the raw log
      told : Int32,         # beats actually narrated
      headline : String,    # one-line summary
      chapters : Array(Chapter)

    # Narrative-worthy events only. Orders (Bucket 1), unit/pop churn, and
    # low-signal bookkeeping are deliberately excluded — they'd flood the
    # chronicle. Battle is kept (it can repeat in a long war, but it IS the
    # story); Settle/BattleAftermath are omitted as duplicates of
    # CreateCity/Battle.
    STORY_EVENTS = %w[
      CreateCity CaptureCity KillCity DisbandCity GiveCity
      GrantAdvance CreateWonder BuildWonder WonderRemoved AccomplishFeat
      EnterAge NukeCity KillPlayer Battle CityRiot GlobalWarming
      OzoneDepletion ContactMade NewProposal Accept Reject
    ]

    def initialize(@client : Ctp2Gateway::GameClient)
    end

    def story : Story
      entries, error = fetch_log
      return Story.new(error, 0, 0, "No story to tell yet.", [] of Chapter) if error
      names = leader_names

      chapters = [] of Chapter
      counts = Hash(String, Int32).new(0)
      told = 0
      current = nil.as(Int32?)
      buf = [] of String

      entries.each do |e|
        ev = e["event"]?.try(&.as_s)
        next unless ev && STORY_EVENTS.includes?(ev)
        line = phrase(e, ev, names, counts)
        next unless line
        told += 1
        round = e["turn"]?.try(&.as_i?) || 0
        if current != round
          chapters << Chapter.new(current.not_nil!, buf) if current && !buf.empty?
          buf = [] of String
          current = round
        end
        buf << line
      end
      chapters << Chapter.new(current.not_nil!, buf) if current && !buf.empty?

      Story.new(nil, entries.size, told, headline(counts, chapters.size), chapters)
    end

    # --- internals --------------------------------------------------------

    private def fetch_log : {Array(JSON::Any), String?}
      case r = @client.command("log_get")
      in GameClient::Ok
        if r.payload["status"]?.try(&.as_s) == "ok"
          {r.payload.dig?("result", "action_log").try(&.as_a) || [] of JSON::Any, nil}
        else
          {[] of JSON::Any, "game says: #{r.payload["detail"]?.try(&.as_s) || "unknown error"}"}
        end
      in GameClient::Err
        {[] of JSON::Any, "gateway: #{r.detail}"}
      end
    end

    # {playerId => leader name}; empty on any failure (we degrade to "A power").
    private def leader_names : Hash(Int32, String)
      out = Hash(Int32, String).new
      case r = @client.command("query_players")
      in GameClient::Ok
        r.payload.dig?("result", "players").try(&.as_a.each do |p|
          id = p["id"]?.try(&.as_i?)
          nm = p["name"]?.try(&.as_s?).try(&.presence)
          out[id] = nm if id && nm
        end)
      in GameClient::Err
        # leave empty
      end
      out
    end

    private def who(e : JSON::Any, names : Hash(Int32, String)) : String
      pid = e["player"]?.try(&.as_i?)
      return "An unknown power" unless pid && pid >= 0
      names[pid]? || "Player #{pid}"
    end

    # First location arg as "(x, y)", or nil.
    private def where(e : JSON::Any) : String?
      e["args"]?.try(&.as_a.each do |a|
        if a["kind"]?.try(&.as_s) == "location"
          return "(#{a["x"]?.try(&.as_i)}, #{a["y"]?.try(&.as_i)})"
        end
      end)
      nil
    end

    # One narrated sentence for an event, bumping the summary counters.
    private def phrase(e, ev, names, counts) : String?
      w = who(e, names)
      loc = where(e)
      at = loc ? " at #{loc}" : ""
      case ev
      when "CreateCity"   then counts["cities"] += 1; "#{w} founded a city#{at}."
      when "CaptureCity"  then counts["captures"] += 1; "#{w} captured a city#{at}."
      when "KillCity"     then "A city was razed#{at}."
      when "DisbandCity"  then "#{w} disbanded a city#{at}."
      when "GiveCity"     then "#{w} ceded a city to a rival."
      when "GrantAdvance" then counts["advances"] += 1; "#{w} mastered a new advance."
      when "CreateWonder", "BuildWonder" then counts["wonders"] += 1; "#{w} raised a great Wonder#{at}."
      when "WonderRemoved" then "A Wonder was lost to the ages."
      when "AccomplishFeat" then "#{w} accomplished a historic feat."
      when "EnterAge"     then "#{w} entered a new age."
      when "NukeCity"     then counts["nukes"] += 1; "#{w} loosed a nuclear strike#{at}."
      when "KillPlayer"   then "#{w} was wiped from the map."
      when "Battle"       then counts["battles"] += 1; "Battle was joined#{at}."
      when "CityRiot"     then "A city under #{w} fell into riot."
      when "GlobalWarming" then "The seas rose — global warming struck."
      when "OzoneDepletion" then "The ozone thinned over the world."
      when "ContactMade"  then "#{w} made first contact with a rival."
      when "NewProposal"  then "#{w} opened diplomatic talks."
      when "Accept"       then "A treaty was accepted."
      when "Reject"       then "A proposal was rebuffed."
      else nil
      end
    end

    private def headline(counts, rounds : Int32) : String
      parts = [] of String
      {"cities" => "founded", "captures" => "captured", "wonders" => "wonders",
       "battles" => "battles", "advances" => "advances", "nukes" => "nuked"}.each do |k, label|
        n = counts[k]
        parts << "#{n} #{label}" if n > 0
      end
      return "A quiet age — nothing of note yet recorded." if parts.empty?
      "#{rounds} chapters · #{parts.join(", ")}."
    end
  end
end
