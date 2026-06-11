require "http/server/handler"

module Ctp2Gateway
  # Static assets (css, htmx, fonts) served BEFORE Athena takes over, via
  # ATH.run's prepend_handlers — the documented escape hatch for things the
  # framework doesn't model (same seam the future /ws WebSocket face will
  # use). Everything else falls through to Athena's routing.
  class AssetsHandler
    include HTTP::Handler

    # Resolved relative to the SOURCE tree at compile time — fine for a dev
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

    def call(context : HTTP::Server::Context) : Nil
      request = context.request
      unless request.method == "GET" && request.path.starts_with?("/assets/")
        return call_next(context)
      end

      path = File.expand_path(File.join(PUBLIC_DIR, request.path.lchop("/assets/")))
      # expand_path collapses any ../ — anything escaping public/ is a 404.
      unless path.starts_with?(PUBLIC_DIR + "/") && File.file?(path)
        context.response.status_code = 404
        context.response.content_type = "application/json"
        context.response << %({"status":"error","gateway":true,"kind":"not_found","detail":"no such asset"})
        return
      end

      context.response.content_type = MIME_TYPES[File.extname(path)]? || "application/octet-stream"
      # CSS is iterated on constantly; fonts and htmx are effectively frozen.
      context.response.headers["Cache-Control"] =
        path.ends_with?(".css") ? "no-cache" : "public, max-age=604800"
      File.open(path) { |f| IO.copy(f, context.response) }
    end
  end
end
