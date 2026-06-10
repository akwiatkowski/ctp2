require "athena"
require "./game_client"
require "./views"

module Ctp2Gateway
  # Debug tool first: no error may render bare. Every exception becomes a
  # full-diagnostics response — class, message, complete backtrace, request
  # line, connection state, recent game exchanges — as HTML, or JSON for
  # /api and /fragments routes.
  #
  # Default listener priority (0) beats Athena's built-in error renderer
  # (-50), and setting the response stops propagation, so this REPLACES the
  # framework's terse error pages rather than racing them. Athena's
  # HTTPExceptions (404 no route, 405 wrong method...) keep their status
  # codes but get our gateway-error body shape.
  @[ADI::Register]
  class ErrorListener
    def initialize(@client : Ctp2Gateway::GameClient)
    end

    @[AEDA::AsEventListener]
    def on_exception(event : AHK::Events::Exception) : Nil
      ex = event.exception
      request = event.request
      req_line = "#{request.method} #{request.path}"
      json = request.path.starts_with?("/api") || request.path.starts_with?("/fragments")

      # Routing-level errors (no route, wrong method) aren't bugs — keep
      # their semantics, in our body shape.
      if ex.is_a?(AHK::Exception::HTTPException)
        status = ex.status.code
        event.response = if json
                           json_error(status, "http_error", "#{ex.message} (#{req_line})")
                         else
                           html_page(status, "Error",
                             Views::ErrorPage.new("#{status} — #{ex.status}", "#{ex.message} (#{req_line})"))
                         end
        return
      end

      trace = ex.backtrace? || [] of String
      STDERR.puts "[error] #{req_line}: #{ex.class}: #{ex.message}\n  #{trace.join("\n  ")}"

      event.response = if json
                         json_exception(ex, req_line, trace)
                       else
                         html_page(500, "Error", Views::DebugError.new(
                           klass: ex.class.name,
                           message: ex.message || "(no message)",
                           backtrace: trace,
                           request_line: req_line,
                           connected: @client.connected?,
                           socket: @client.socket_path,
                           exchanges: @client.recent_exchanges.first(8),
                         ))
                       end
    end

    private def json_exception(ex : ::Exception, req_line : String, trace : Array(String)) : AHTTP::Response
      AHTTP::Response.new(
        {
          status:    "error",
          gateway:   true,
          kind:      "exception",
          class:     ex.class.name,
          message:   ex.message,
          request:   req_line,
          backtrace: trace,
          connected: @client.connected?,
          exchanges: @client.recent_exchanges.first(8).map do |e|
            {at: e.at.to_rfc3339(fraction_digits: 3), cmd: e.cmd, ok: e.ok,
             duration_ms: e.duration.total_milliseconds.round(1), detail: e.detail}
          end,
        }.to_json,
        status: 500,
        headers: HTTP::Headers{"content-type" => "application/json"},
      )
    end

    private def json_error(status : Int32, kind : String, detail : String) : AHTTP::Response
      AHTTP::Response.new(
        {status: "error", gateway: true, kind: kind, detail: detail}.to_json,
        status: status,
        headers: HTTP::Headers{"content-type" => "application/json"},
      )
    end

    private def html_page(status : Int32, title : String, body) : AHTTP::Response
      AHTTP::Response.new(
        Views.page(title, body, "/"),
        status: status,
        headers: HTTP::Headers{"content-type" => "text/html; charset=utf-8"},
      )
    end
  end
end
