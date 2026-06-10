require "option_parser"
require "./server"

# ctp2-gateway entry point.
#
# Attach-only (v0.1): start the game yourself with the smoke server enabled
# (ctp2_headless --serve, or the UI build with --smoketest), then run this.
# The gateway connects lazily and reconnects automatically, so start order
# doesn't matter.

host = "127.0.0.1"
port = (ENV["CTP2_GATEWAY_PORT"]? || "8666").to_i
socket_path = ENV["CTP2_SOCKET"]? || "/tmp/ctp2-smoke.sock"

OptionParser.parse do |parser|
  parser.banner = "Usage: ctp2-gateway [options]"
  parser.on("--port PORT", "HTTP port (default 8666, env CTP2_GATEWAY_PORT)") { |v| port = v.to_i }
  parser.on("--socket PATH", "game smoke socket (default /tmp/ctp2-smoke.sock, env CTP2_SOCKET)") { |v| socket_path = v }
  parser.on("-h", "--help", "show this help") do
    puts parser
    exit
  end
  parser.invalid_option do |flag|
    STDERR.puts "unknown option: #{flag}"
    STDERR.puts parser
    exit 1
  end
end

client = Ctp2Gateway::GameClient.new(socket_path)
server = Ctp2Gateway::Server.new(client)

address = server.bind(host, port)
puts "ctp2-gateway listening on http://#{address} (game socket: #{socket_path})"
server.listen
