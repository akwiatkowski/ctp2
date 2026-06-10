require "option_parser"
require "socket"
require "./server"

# ctp2-gateway entry point.
#
# Two ways to pair with the game:
#   * attach (default) — start the game yourself (ctp2_headless --serve or
#     the UI build's smoke-test mode); the gateway connects lazily and
#     reconnects automatically, so start order doesn't matter.
#   * --spawn — the gateway launches and supervises ctp2_headless --serve
#     itself, and terminates it on shutdown (SIGINT/SIGTERM).

host = "127.0.0.1"
port = (ENV["CTP2_GATEWAY_PORT"]? || "8666").to_i
socket_path = ENV["CTP2_SOCKET"]? || "/tmp/ctp2-smoke.sock"
spawn_game = false
binary = ENV["CTP2_BINARY"]?
spawn_args = [] of String
spawn_log = "/tmp/ctp2-headless.log"
spawn_cwd = nil.as(String?)

OptionParser.parse do |parser|
  parser.banner = "Usage: ctp2-gateway [options]"
  parser.on("--port PORT", "HTTP port (default 8666, env CTP2_GATEWAY_PORT)") { |v| port = v.to_i }
  parser.on("--socket PATH", "game smoke socket (default /tmp/ctp2-smoke.sock, env CTP2_SOCKET)") { |v| socket_path = v }
  parser.on("--spawn", "launch ctp2_headless --serve and supervise it") { spawn_game = true }
  parser.on("--binary PATH", "game binary for --spawn (default: auto-detect build/ctp2_headless, env CTP2_BINARY)") { |v| binary = v }
  parser.on("--spawn-args ARGS", %(extra game args for --spawn, e.g. "--players 4 --seed 7")) { |v| spawn_args = v.split }
  parser.on("--spawn-log PATH", "game stdout/stderr log for --spawn (default /tmp/ctp2-headless.log)") { |v| spawn_log = v }
  parser.on("--spawn-cwd DIR", "working directory for the game (default: binary's repo root; it loads assets relative to cwd)") { |v| spawn_cwd = v }
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

process = nil.as(Ctp2Gateway::GameProcess?)

if spawn_game
  # Auto-detect the binary whether the gateway runs from the repo root or
  # from gateway/. `binary` itself is captured by the OptionParser closures,
  # so the compiler can't flow-narrow it — copy into a fresh local first.
  resolved = binary || ["build/ctp2_headless", "../build/ctp2_headless"].find { |p| File.exists?(p) }
  unless resolved
    STDERR.puts "--spawn: no game binary found (tried build/ctp2_headless, ../build/ctp2_headless); use --binary"
    exit 1
  end
  resolved = File.expand_path(resolved)

  # Refuse to race an already-running game for the socket: the game unlinks
  # and rebinds the path at startup, so spawning a second instance would
  # steal the socket from the first.
  begin
    UNIXSocket.new(socket_path).close
    STDERR.puts "--spawn: a game is already serving #{socket_path}; attaching to it instead"
  rescue IO::Error | Socket::Error
    # The game loads its asset databases relative to cwd. With the standard
    # layout (<repo>/build/ctp2_headless) the repo root is two levels up
    # from the binary.
    cwd = spawn_cwd.as(String?) || Path[resolved].parent.parent.to_s
    process = Ctp2Gateway::GameProcess.new(resolved, ["--serve"] + spawn_args, cwd, spawn_log)
    exit 1 unless process.start
  end
end

client = Ctp2Gateway::GameClient.new(socket_path)
server = Ctp2Gateway::Server.new(client, process)

# Take the child down with us. Signal handlers run on the event loop, so
# keep them short: stop the game (SIGTERM → SIGKILL), close, exit.
{Signal::INT, Signal::TERM}.each do |sig|
  sig.trap do
    puts "\n#{sig} — shutting down"
    process.try &.stop
    server.close rescue nil
    exit 0
  end
end

address = server.bind(host, port)
puts "ctp2-gateway listening on http://#{address} (game socket: #{socket_path}#{process ? ", spawned game pid #{process.pid}" : ""})"
server.listen
