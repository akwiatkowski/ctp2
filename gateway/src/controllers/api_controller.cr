require "./base_controller"

module Ctp2Gateway
  # JSON face: REST sugar over the game verbs + the raw command channel.
  @[ADI::Register]
  class ApiController < BaseController
    # Athena's DI macros only inspect constructors defined on the concrete
    # class — an inherited one resolves to `.new()`. Delegate explicitly.
    def initialize(client : Ctp2Gateway::GameClient)
      super(client)
    end

    @[ARTA::Get("/healthz")]
    def healthz : AHTTP::Response
      AHTTP::Response.new(
        {
          status:      "ok",
          session_id:  Config.session.id,
          socket:      @client.socket_path,
          connected:   @client.connected?,
          last_error:  @client.last_error,
          spawn:       Config.process.try(&.status_json),
          journal:     Config.session.journal_path,
        }.to_json,
        headers: HTTP::Headers{"content-type" => JSON_CT},
      )
    end

    @[ARTA::Post("/api/cmd")]
    def raw_cmd(request : AHTTP::Request) : AHTTP::Response
      body = request.body.try(&.gets_to_end)
      unless body
        return gateway_error(400, "bad_request", "missing body")
      end
      cmd = JSON.parse(body)["cmd"]?.try(&.as_s?)
      unless cmd
        return gateway_error(400, "bad_request", %(body must be {"cmd":"<verb>"}))
      end
      passthrough(@client.command(cmd))
    rescue JSON::ParseException
      gateway_error(400, "bad_request", "body is not valid JSON")
    end

    @[ARTA::Get("/api/players")]
    def players : AHTTP::Response
      passthrough(@client.command("query_players"))
    end

    @[ARTA::Get("/api/players/{id}", requirements: {"id" => /\d+/})]
    def player(id : Int32) : AHTTP::Response
      passthrough(@client.command("query_player #{id}"))
    end

    @[ARTA::Get("/api/players/{id}/cities", requirements: {"id" => /\d+/})]
    def player_cities(id : Int32) : AHTTP::Response
      passthrough(@client.command("query_player_cities #{id}"))
    end

    @[ARTA::Get("/api/turn")]
    def turn : AHTTP::Response
      passthrough(@client.command("query_turn"))
    end

    @[ARTA::Get("/api/cities")]
    def cities : AHTTP::Response
      passthrough(@client.command("query_cities"))
    end

    @[ARTA::Get("/api/city/{idx}", requirements: {"idx" => /\d+/})]
    def city(idx : Int32) : AHTTP::Response
      passthrough(@client.command("query_city #{idx}"))
    end

    @[ARTA::Get("/api/units")]
    def units : AHTTP::Response
      passthrough(@client.command("query_units"))
    end

    @[ARTA::Get("/api/armies")]
    def armies : AHTTP::Response
      passthrough(@client.command("query_armies"))
    end

    @[ARTA::Get("/api/map")]
    def map : AHTTP::Response
      passthrough(@client.command("query_map"))
    end

    # Intentional raiser — exercises the JSON error rendering end-to-end.
    @[ARTA::Get("/api/debug/boom")]
    def boom : Nil
      raise "intentional test exception (/api/debug/boom)"
    end

    # GET /api — machine- and human-readable description of the whole
    # surface, so `curl localhost:8666/api` is the documentation.
    @[ARTA::Get("/api")]
    def index : AHTTP::Response
      doc = {
        service: "ctp2-gateway",
        about:   "HTTP face over CTP2's Unix-socket test API. HTML pages: / (dashboard), /players, /players/<id>/cities, /debug.",
        status_contract: {
          "200" => "game replied — including the game's own {\"status\":\"error\"} answers",
          "400" => "bad request (malformed body, multi-line command)",
          "404" => "no such route/asset",
          "429" => "request queue full (backpressure)",
          "500" => "gateway exception — body carries class, backtrace, recent exchanges",
          "502" => "game replied with non-JSON or a desynced reply",
          "503" => "game socket disconnected",
          "504" => "game did not reply within the timeout",
        },
        endpoints: [
          {method: "GET", path: "/api", about: "this document"},
          {method: "POST", path: "/api/cmd", body: %({"cmd":"<verb ...args>"}), about: "raw passthrough of any game verb"},
          {method: "GET", path: "/api/players", maps_to: "query_players", about: "every player slot (omniscient): id, name, civ, country, human, dead, gold, num_cities, num_units, num_armies, government, score"},
          {method: "GET", path: "/api/players/<id>", maps_to: "query_player <id>", about: "one player slot, same shape as a query_players entry"},
          {method: "GET", path: "/api/players/<id>/cities", maps_to: "query_player_cities <id>", about: "ALL cities of one player (omniscient), incl. per-turn yields"},
          {method: "GET", path: "/api/turn", maps_to: "query_turn", about: "the clock: round (full rounds completed) and calendar year"},
          {method: "GET", path: "/api/cities", maps_to: "query_cities", about: "cities visible to the human player (fog-filtered)"},
          {method: "GET", path: "/api/city/<idx>", maps_to: "query_city <idx>", about: "one of the human's cities + buildable units"},
          {method: "GET", path: "/api/units", maps_to: "query_units", about: "units visible to the human player (fog-filtered)"},
          {method: "GET", path: "/api/armies", maps_to: "query_armies", about: "YOUR armies: index, pos, moves_left, can_settle, units — feed move_army/auto_explore"},
          {method: "GET", path: "/api/map", maps_to: "query_map", about: "explored tiles: terrain, visibility, city markers"},
          {method: "GET", path: "/healthz", about: "gateway + socket + spawned-game state"},
          {method: "GET", path: "/fragments/dashboard", about: "htmx fragment: dashboard status strip + stats"},
          {method: "GET", path: "/api/debug/boom", about: "raises intentionally — exercises the JSON error rendering"},
          {method: "POST", path: "/mcp", about: "MCP endpoint (streamable HTTP, tools-only) — claude mcp add --transport http ctp2 http://localhost:8666/mcp"},
        ],
        verbs: {
          commands: ["build_city (errors: tile_occupied, settle_rejected — min distance ~3)",
                     "move_army <army_idx> <x> <y>", "auto_explore <army_idx>",
                     "set_production <city_idx> <settler|cheapest_military|N>",
                     "end_turn [N] (headless: advance N full rounds; UI: queue director end-turn)",
                     "save_game <path>", "load_game <path>", "new_game", "start_game", "quit"],
          queries_player_view: ["query_cities", "query_city <idx>", "query_units", "query_armies", "query_map"],
          queries_admin: ["query_players", "query_player <id>", "query_player_cities <id>", "query_turn", "query_research", "query_terraform <x> <y>"],
          commands_economy: ["set_research <advance_id>", "set_material_tax <0..100>", "terraform <x> <y> <improvement_id>", "grant_advance <id> (DEBUG cheat for tests)"],
        },
      }
      AHTTP::Response.new(doc.to_pretty_json, headers: HTTP::Headers{"content-type" => JSON_CT})
    end
  end
end
