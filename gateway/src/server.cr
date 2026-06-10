require "http/server"
require "./game_client"
require "./game_process"

module Ctp2Gateway
  # HTTP face of the gateway. v0.1 routes:
  #
  #   GET  /healthz          gateway + socket status
  #   POST /api/cmd          {"cmd":"<verb ...>"} — raw passthrough
  #   GET  /api/cities       → query_cities
  #   GET  /api/units        → query_units
  #   GET  /api/map          → query_map
  #   GET  /api/city/<idx>   → query_city <idx>
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
    CITY_PATH = %r{\A/api/city/(\d+)\z}

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
      when {"GET", "/healthz"}
        healthz(ctx)
      when {"POST", "/api/cmd"}
        raw_cmd(ctx)
      when {"GET", "/api/cities"}
        respond(ctx, @client.command("query_cities"))
      when {"GET", "/api/units"}
        respond(ctx, @client.command("query_units"))
      when {"GET", "/api/map"}
        respond(ctx, @client.command("query_map"))
      else
        if req.method == "GET" && (m = req.path.match(CITY_PATH))
          respond(ctx, @client.command("query_city #{m[1]}"))
        else
          gateway_error(ctx, 404, "not_found", "no route for #{req.method} #{req.path}")
        end
      end
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
        status = case result.kind
                 in .busy?         then 429
                 in .protocol?     then 502
                 in .disconnected? then 503
                 in .timeout?      then 504
                 end
        # `protocol` from our own pre-validation (multi-line cmd) is caller
        # error, not the game's fault.
        status = 400 if result.kind.protocol? && result.detail.includes?("single line")
        gateway_error(ctx, status, result.kind.to_s.underscore, result.detail)
      end
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
