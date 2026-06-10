require "http/server"
require "./game_client"
require "./game_process"
require "./views"

module Ctp2Gateway
  # HTTP face of the gateway. v0.1 routes:
  #
  # HTML (admin panel — omniscient debugging surface):
  #   GET  /                        dashboard: connection/spawn status + game summary
  #   GET  /players                 player table (query_players)
  #   GET  /players/<id>/cities     one player's cities (query_player_cities)
  #
  # JSON:
  #   GET  /healthz                 gateway + socket status
  #   POST /api/cmd                 {"cmd":"<verb ...>"} — raw passthrough
  #   GET  /api/players             → query_players
  #   GET  /api/players/<id>/cities → query_player_cities <id>
  #   GET  /api/cities              → query_cities (fog-filtered, player view)
  #   GET  /api/units               → query_units
  #   GET  /api/map                 → query_map
  #   GET  /api/city/<idx>          → query_city <idx>
  #
  # Game responses pass through verbatim with HTTP 200 — including the game's
  # own {"status":"error",...} replies, because "the game says no" is a
  # successful gateway roundtrip. Non-200 codes are reserved for GATEWAY
  # failures, so a curl user can always tell who is talking:
  #   400 bad request (malformed body / multi-line cmd)
  #   429 request queue full (backpressure)
  #   502 game replied with non-JSON
  #   503 game socket disconnected
  #   504 game accepted the command but didn't reply in time
  class Server
    CITY_PATH          = %r{\A/api/city/(\d+)\z}
    PLAYER_CITIES_PATH = %r{\A/players/(\d+)/cities\z}
    API_PLAYER_CITIES  = %r{\A/api/players/(\d+)/cities\z}

    # Vendored static assets (css, htmx, fonts) live in gateway/public/,
    # resolved relative to the SOURCE tree at compile time — fine for a dev
    # tool whose binary lives next to its repo.
    PUBLIC_DIR = File.expand_path(File.join(__DIR__, "..", "public"))

    MIME_TYPES = {
      ".css"   => "text/css; charset=utf-8",
      ".js"    => "text/javascript; charset=utf-8",
      ".woff2" => "font/woff2",
      ".svg"   => "image/svg+xml",
      ".png"   => "image/png",
      ".ico"   => "image/x-icon",
    }

    # process is present only when the gateway spawned the game (--spawn);
    # /healthz then reports its pid/exit state alongside the socket state.
    def initialize(@client : GameClient, @process : GameProcess? = nil)
      @http = HTTP::Server.new { |ctx| handle(ctx) }
    end

    # Bind separately from listen so specs can use port 0 and read the real
    # port back before starting the accept loop.
    def bind(host : String, port : Int32) : Socket::IPAddress
      @http.bind_tcp(host, port)
    end

    def listen : Nil
      @http.listen
    end

    def close : Nil
      @http.close
    end

    private def handle(ctx : HTTP::Server::Context) : Nil
      req = ctx.request
      case {req.method, req.path}
      when {"GET", "/"}
        dashboard(ctx)
      when {"GET", "/fragments/dashboard"}
        # htmx polls this every 5s and swaps it into #ledger.
        ctx.response.content_type = "text/html; charset=utf-8"
        ctx.response << build_ledger.to_s
      when {"GET", "/players"}
        players_page(ctx)
      when {"GET", "/healthz"}
        healthz(ctx)
      when {"POST", "/api/cmd"}
        raw_cmd(ctx)
      when {"GET", "/api/players"}
        respond(ctx, @client.command("query_players"))
      when {"GET", "/api/cities"}
        respond(ctx, @client.command("query_cities"))
      when {"GET", "/api/units"}
        respond(ctx, @client.command("query_units"))
      when {"GET", "/api/map"}
        respond(ctx, @client.command("query_map"))
      else
        if req.method == "GET" && req.path.starts_with?("/assets/")
          static_asset(ctx, req.path.lchop("/assets/"))
        elsif req.method == "GET" && (m = req.path.match(PLAYER_CITIES_PATH))
          player_cities_page(ctx, m[1].to_i)
        elsif req.method == "GET" && (m = req.path.match(API_PLAYER_CITIES))
          respond(ctx, @client.command("query_player_cities #{m[1]}"))
        elsif req.method == "GET" && (m = req.path.match(CITY_PATH))
          respond(ctx, @client.command("query_city #{m[1]}"))
        else
          gateway_error(ctx, 404, "not_found", "no route for #{req.method} #{req.path}")
        end
      end
    end

    # --- HTML pages (admin panel) ----------------------------------------

    # Run an admin query and split the outcome: {result, gateway_error,
    # game_error_detail}. Exactly one of the three is non-nil (result may be
    # nil for ok-with-no-result verbs, which the panel never uses).
    private def fetch(cmd : String) : {JSON::Any?, GameClient::Err?, String?}
      case r = @client.command(cmd)
      in GameClient::Ok
        if r.payload["status"]?.try(&.as_s) == "ok"
          {r.payload["result"]?, nil, nil}
        else
          {nil, nil, r.payload["detail"]?.try(&.as_s) || "unknown game error"}
        end
      in GameClient::Err
        {nil, r, nil}
      end
    end

    # Build the live part of the dashboard (status strip + stat cards) —
    # shared between the full page and the htmx polling fragment.
    private def build_ledger : Views::DashboardLedger
      spawn_line = @process.try do |p|
        s = p.status_json
        s[:running] ? "pid #{s[:pid]}, running" : "exited#{s[:exit_code] ? " (code #{s[:exit_code]})" : ""}"
      end

      result, gw_err, game_err = fetch("query_players")
      error = gw_err.try(&.detail) || game_err
      players = result.try(&.["players"].as_a) || [] of JSON::Any
      leader = players.reject { |p| p["dead"].as_bool }.max_by? { |p| p["score"].as_i }

      Views::DashboardLedger.new(
        socket: @client.socket_path,
        connected: @client.connected?,
        spawn_line: spawn_line,
        game_error: error,
        player_count: players.size,
        alive_count: players.count { |p| !p["dead"].as_bool },
        city_count: players.sum { |p| p["num_cities"].as_i },
        leader_name: leader.try { |l| l["name"].as_s.presence },
        leader_score: leader.try(&.["score"].as_i) || 0,
      )
    end

    private def dashboard(ctx) : Nil
      # The dashboard always renders (200) — showing "game unavailable" IS
      # its job, unlike the API routes where non-200 signals the failure.
      html(ctx, 200, "Dashboard", Views::Dashboard.new(build_ledger.to_s))
    end

    private def static_asset(ctx, rel : String) : Nil
      path = File.expand_path(File.join(PUBLIC_DIR, rel))
      # expand_path collapses any ../ — anything escaping public/ is a 404.
      unless path.starts_with?(PUBLIC_DIR + "/") && File.file?(path)
        return gateway_error(ctx, 404, "not_found", "no such asset")
      end
      mime = MIME_TYPES[File.extname(path)]? || "application/octet-stream"
      ctx.response.content_type = mime
      # CSS is iterated on constantly; fonts and htmx are effectively frozen.
      ctx.response.headers["Cache-Control"] =
        path.ends_with?(".css") ? "no-cache" : "public, max-age=604800"
      File.open(path) { |f| IO.copy(f, ctx.response) }
    end

    private def players_page(ctx) : Nil
      result, gw_err, game_err = fetch("query_players")
      if gw_err
        return html(ctx, err_status(gw_err), "Error", Views::ErrorPage.new("Gateway error", gw_err.detail))
      end
      if game_err
        return html(ctx, 200, "Players", Views::ErrorPage.new("Players", "game says: #{game_err}"))
      end
      players = result.try(&.["players"].as_a) || [] of JSON::Any
      html(ctx, 200, "Players", Views::Players.new(players))
    end

    private def player_cities_page(ctx, id : Int32) : Nil
      result, gw_err, game_err = fetch("query_player_cities #{id}")
      if gw_err
        return html(ctx, err_status(gw_err), "Error", Views::ErrorPage.new("Gateway error", gw_err.detail))
      end
      if game_err
        status = game_err == "bad_player" ? 404 : 200
        return html(ctx, status, "Cities", Views::ErrorPage.new("Player #{id}", "game says: #{game_err}"))
      end
      cities = result.try(&.["cities"].as_a) || [] of JSON::Any

      # Second roundtrip for the leader name — admin pages favour clarity
      # over saving a local-socket call. Fall back silently if it fails.
      name = "Player #{id}"
      presult, _, _ = fetch("query_players")
      presult.try(&.["players"].as_a.each do |p|
        name = p["name"].as_s if p["id"].as_i == id && !p["name"].as_s.empty?
      end)

      html(ctx, 200, "Cities of #{name}", Views::PlayerCities.new(id, name, cities))
    end

    private def html(ctx, status : Int32, title : String, body) : Nil
      ctx.response.status_code = status
      ctx.response.content_type = "text/html; charset=utf-8"
      ctx.response << Views.page(title, body, ctx.request.path)
    end

    private def healthz(ctx) : Nil
      ctx.response.content_type = "application/json"
      {
        status:     "ok",
        socket:     @client.socket_path,
        connected:  @client.connected?,
        last_error: @client.last_error,
        spawn:      @process.try(&.status_json),
      }.to_json(ctx.response)
    end

    private def raw_cmd(ctx) : Nil
      body = ctx.request.body.try(&.gets_to_end)
      unless body
        return gateway_error(ctx, 400, "bad_request", "missing body")
      end
      cmd = JSON.parse(body)["cmd"]?.try(&.as_s?)
      unless cmd
        return gateway_error(ctx, 400, "bad_request", %(body must be {"cmd":"<verb>"}))
      end
      respond(ctx, @client.command(cmd))
    rescue JSON::ParseException
      gateway_error(ctx, 400, "bad_request", "body is not valid JSON")
    end

    private def respond(ctx, result : GameClient::Result) : Nil
      case result
      in GameClient::Ok
        ctx.response.content_type = "application/json"
        result.payload.to_json(ctx.response)
      in GameClient::Err
        gateway_error(ctx, err_status(result), result.kind.to_s.underscore, result.detail)
      end
    end

    private def err_status(err : GameClient::Err) : Int32
      status = case err.kind
               in .busy?         then 429
               in .protocol?     then 502
               in .disconnected? then 503
               in .timeout?      then 504
               end
      # `protocol` from our own pre-validation (multi-line cmd) is caller
      # error, not the game's fault.
      status = 400 if err.kind.protocol? && err.detail.includes?("single line")
      status
    end

    # Gateway-originated errors carry "gateway":true so they can never be
    # confused with the game's own {"status":"error"} responses.
    private def gateway_error(ctx, status : Int32, kind : String, detail : String) : Nil
      ctx.response.status_code = status
      ctx.response.content_type = "application/json"
      {status: "error", gateway: true, kind: kind, detail: detail}.to_json(ctx.response)
    end
  end
end
