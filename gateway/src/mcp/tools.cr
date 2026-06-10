require "json"
require "../game_client"

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
      "or a numeric unit type id from query_city's buildable list.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0,"description":"index into your city list (see query_cities)"},"what":{"type":"string","description":"settler | cheapest_military | unit type id"}},"required":["city_index","what"],"additionalProperties":false}),
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
