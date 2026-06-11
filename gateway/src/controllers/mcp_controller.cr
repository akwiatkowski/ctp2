require "./base_controller"
require "../mcp/tools"

module Ctp2Gateway
  # MCP (Model Context Protocol) endpoint — streamable HTTP transport,
  # tools-only. Hand-rolled JSON-RPC 2.0: the surface is four methods
  # (initialize, ping, tools/list, tools/call) plus notifications, which is
  # not worth a framework. Register with Claude Code via:
  #
  #   claude mcp add --transport http ctp2 http://localhost:8666/mcp
  #
  # Transport behaviours (2025-06-18 spec):
  #   * POST one JSON-RPC message → application/json response (no SSE).
  #   * Notifications (no "id") → 202 Accepted, empty body.
  #   * GET /mcp → 405 (no server-initiated stream in v0.1).
  #   * Mcp-Session-Id issued on initialize; accepted but not enforced —
  #     the gateway is stateless, all game state lives in the game.
  #   * Batches → -32600 (removed from the spec in 2025-06-18).
  @[ADI::Register]
  class McpController < BaseController
    JSONRPC_PARSE_ERROR     = -32700
    JSONRPC_INVALID_REQUEST = -32600
    JSONRPC_METHOD_MISSING  = -32601
    JSONRPC_INVALID_PARAMS  = -32602

    # Versions we know; an unknown requested version gets our latest.
    PROTOCOL_VERSIONS = ["2025-06-18", "2025-03-26", "2024-11-05"]

    def initialize(client : Ctp2Gateway::GameClient)
      super(client)
    end

    @[ARTA::Post("/mcp")]
    def endpoint(request : AHTTP::Request) : AHTTP::Response
      body = request.body.try(&.gets_to_end)
      return rpc_error(nil, JSONRPC_PARSE_ERROR, "empty body") unless body

      msg = begin
        JSON.parse(body)
      rescue ex : JSON::ParseException
        return rpc_error(nil, JSONRPC_PARSE_ERROR, "parse error: #{ex.message}")
      end

      if msg.as_a?
        return rpc_error(nil, JSONRPC_INVALID_REQUEST, "batching was removed in MCP 2025-06-18")
      end

      id = msg["id"]?
      method = msg["method"]?.try(&.as_s?)
      return rpc_error(id, JSONRPC_INVALID_REQUEST, "missing method") unless method

      # Notifications (no id) are acknowledged and otherwise ignored —
      # nothing to do for notifications/initialized in a stateless server.
      if id.nil?
        return AHTTP::Response.new(nil, status: 202)
      end

      case method
      when "initialize"      then initialize_response(id, msg["params"]?)
      when "ping"            then rpc_result(id, JSON.parse("{}"))
      when "tools/list"      then tools_list(id)
      when "tools/call"      then tools_call(id, msg["params"]?)
      else
        rpc_error(id, JSONRPC_METHOD_MISSING, "method not supported: #{method}")
      end
    end

    # The transport's other verbs: explicit 405s beat Athena's routing 404 —
    # they tell a conforming client "this server has no SSE stream / no
    # session state to delete" rather than "wrong URL".
    @[ARTA::Get("/mcp")]
    @[ARTA::Delete("/mcp")]
    def no_stream : AHTTP::Response
      gateway_error(405, "method_not_allowed", "no SSE stream / session deletion; POST JSON-RPC messages to /mcp")
    end

    private def initialize_response(id : JSON::Any, params : JSON::Any?) : AHTTP::Response
      requested = params.try(&.["protocolVersion"]?).try(&.as_s?)
      version = PROTOCOL_VERSIONS.includes?(requested) ? requested.not_nil! : PROTOCOL_VERSIONS.first

      response = rpc_result(id, JSON.parse({
        protocolVersion: version,
        capabilities:    {tools: {} of String => String},
        serverInfo:      {name: "ctp2-gateway", version: "0.1.0"},
        instructions:    "CTP2 (Call to Power 2) game control. Typical flow: start_game → " \
                         "build_city → set_production → end_turn → queries. PLAYER VIEW tools " \
                         "are fog-of-war filtered; ADMIN tools are omniscient. save_game/" \
                         "load_game give you checkpoints for experiments.",
      }.to_json))
      response.headers["Mcp-Session-Id"] = Random::Secure.hex(8)
      response
    end

    private def tools_list(id : JSON::Any) : AHTTP::Response
      tools = Mcp::TOOLS.map do |t|
        {
          "name"        => JSON::Any.new(t.name),
          "description" => JSON::Any.new(t.description),
          "inputSchema" => JSON.parse(t.schema),
        }
      end
      rpc_result(id, JSON.parse({tools: tools}.to_json))
    end

    private def tools_call(id : JSON::Any, params : JSON::Any?) : AHTTP::Response
      name = params.try(&.["name"]?).try(&.as_s?)
      return rpc_error(id, JSONRPC_INVALID_PARAMS, "missing tool name") unless name

      tool = Mcp.find(name)
      return rpc_error(id, JSONRPC_INVALID_PARAMS, "unknown tool: #{name}") unless tool

      arguments = params.try(&.["arguments"]?) || JSON.parse("{}")

      result = begin
        tool.run.call(@client, arguments)
      rescue ex : KeyError | TypeCastError
        # Schema-shaped arguments missing/badly typed — tell the model what
        # was wrong so it can correct the call.
        return tool_result(id, %({"error":"invalid arguments for #{name}: #{ex.message}"}), is_error: true)
      end

      case result
      in GameClient::Ok
        # String payloads (render_map) pass through as raw text; JSON
        # payloads pretty-print. The game's own {"status":"error"} answers
        # are TOOL errors: the model should read the detail and adapt
        # (e.g. no_settler_found).
        if raw = result.payload.as_s?
          return tool_result(id, raw, is_error: false)
        end
        game_error = result.payload["status"]?.try(&.as_s?) == "error"
        tool_result(id, result.payload.to_pretty_json, is_error: game_error)
      in GameClient::Err
        tool_result(id, %({"gateway_error":"#{result.kind.to_s.underscore}","detail":#{result.detail.to_json}}), is_error: true)
      end
    end

    private def tool_result(id : JSON::Any, text : String, is_error : Bool) : AHTTP::Response
      rpc_result(id, JSON.parse({
        content: [{type: "text", text: text}],
        isError: is_error,
      }.to_json))
    end

    private def rpc_result(id : JSON::Any?, result : JSON::Any) : AHTTP::Response
      AHTTP::Response.new(
        {jsonrpc: "2.0", id: id, result: result}.to_json,
        headers: HTTP::Headers{"content-type" => JSON_CT},
      )
    end

    # JSON-RPC errors ride HTTP 200: the envelope carries the failure.
    private def rpc_error(id : JSON::Any?, code : Int32, message : String) : AHTTP::Response
      AHTTP::Response.new(
        {jsonrpc: "2.0", id: id, error: {code: code, message: message}}.to_json,
        headers: HTTP::Headers{"content-type" => JSON_CT},
      )
    end
  end
end
