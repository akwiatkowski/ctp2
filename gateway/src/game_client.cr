require "socket"
require "json"

module Ctp2Gateway
  # Client for the game's smoke-test socket (newline-delimited JSON over a
  # Unix domain socket, see ctp2_code/test/smoketest_server.cpp).
  #
  # The game accepts ONE client and ONE in-flight command at a time: on the
  # C++ side, polling a command leaves an SDL mutex locked until the response
  # is sent. This class is therefore the single serializer for the whole
  # gateway — every HTTP/MCP/WS request funnels through here.
  #
  # Concurrency model (idiomatic Crystal, no locks):
  #   * One "owner" fiber holds the socket and processes requests strictly
  #     one at a time.
  #   * Callers (any fiber) build a Request with a private reply channel and
  #     push it onto @requests; then block on the reply.
  #   * Reply channels are BUFFERED (capacity 1) so the owner's `send` can
  #     never block on a caller that gave up — an unbuffered channel here
  #     could wedge the owner fiber forever.
  #
  # Errors are values, not exceptions: callers always get a Result and decide
  # how to map it (the HTTP layer turns kinds into status codes).
  class GameClient
    # Why each error kind exists:
    #   Disconnected — no socket / connect refused / game closed mid-request.
    #   Timeout      — game accepted the command but didn't answer in time;
    #                  the connection is then POISONED (a late reply would be
    #                  matched to the wrong request), so we drop it.
    #   Busy         — request queue full; backpressure instead of unbounded
    #                  memory growth.
    #   Protocol     — the game answered with something that isn't JSON, or
    #                  the caller tried to send a multi-line command (which
    #                  would inject extra commands into the line protocol).
    enum ErrorKind
      Disconnected
      Timeout
      Busy
      Protocol
    end

    record Ok, payload : JSON::Any
    record Err, kind : ErrorKind, detail : String

    alias Result = Ok | Err

    # One entry of the exchange journal — the debugging primitive behind
    # /debug and the error pages: what was asked, what came back, how long
    # it took.
    #   cmd           — the full command line sent to the game
    #   verb          — first token of cmd (the action name)
    #   args          — remainder of cmd after the verb
    #   detail        — a truncated response/error snippet for tables
    #   full_response — the complete raw response or error text; kept for the
    #                   collapsible /debug inspector and for durable journals
    record Exchange,
      at : Time,
      session_id : String,
      cmd : String,
      verb : String,
      args : String,
      ok : Bool,
      detail : String,
      full_response : String,
      duration : Time::Span

    private record Request, cmd : String, reply : Channel(Result)

    # 30s default: start_game runs full world generation and takes seconds;
    # end_turn N will be the slowest verb once it lands.
    DEFAULT_TIMEOUT = 30.seconds

    # Pending-request cap. 32 is generous for a dev tool — if we're 32 deep,
    # something is polling pathologically and deserves a 429.
    DEFAULT_QUEUE_CAPACITY = 32

    # Ring size of the in-memory exchange journal and snippet length per entry.
    EXCHANGE_LOG_SIZE = 32
    SNIPPET_LEN       = 220

    getter socket_path : String
    getter session_id : String
    getter journal_path : String?
    # Connection state for /healthz. Plain (non-atomic) fields are safe here:
    # Crystal fibers are cooperatively scheduled on one thread by default,
    # and only the owner fiber writes them.
    getter? connected : Bool = false
    getter last_error : String?

    def initialize(@socket_path : String,
                   @timeout : Time::Span = DEFAULT_TIMEOUT,
                   queue_capacity : Int32 = DEFAULT_QUEUE_CAPACITY,
                   @session_id : String = "anonymous",
                   @journal_path : String? = nil)
      @socket = nil.as(UNIXSocket?)
      @requests = Channel(Request).new(queue_capacity)
      @exchanges = Deque(Exchange).new
      replay_journal
      spawn(name: "game-client-owner") { run_loop }
    end

    # Newest-first copy of the exchange journal (owner fiber owns the deque;
    # single-threaded scheduling makes the copy safe).
    def recent_exchanges : Array(Exchange)
      @exchanges.to_a.reverse
    end

    # Send one verb line (e.g. "query_city 3") and wait for the game's reply.
    # Blocks the calling fiber; never raises.
    def command(cmd : String) : Result
      # A newline inside cmd would smuggle a second command into the
      # line-delimited protocol — reject at the protocol boundary.
      if cmd.matches?(/[\r\n]/) || cmd.strip.empty?
        return Err.new(:protocol, "command must be a non-empty single line")
      end

      reply = Channel(Result).new(1) # buffered — see class comment
      select
      when @requests.send(Request.new(cmd, reply))
        reply.receive
      else
        # Channel buffer full → don't block the caller, report backpressure.
        Err.new(:busy, "request queue full")
      end
    end

    # Stop the owner fiber and drop the connection. Used by specs.
    def close : Nil
      @requests.close
      @socket.try &.close
      @socket = nil
      @connected = false
    end

    # Point the client at a different game socket: drop the current
    # connection, clear the journal, reset error state. The owner fiber and
    # request queue keep running — the next command lazily connects to the
    # new path. Exists for specs (the DI container holds ONE GameClient for
    # the process lifetime; tests swap the FakeGame under it per test).
    def reconfigure(socket_path : String) : Nil
      @socket.try &.close rescue nil
      @socket = nil
      @connected = false
      @last_error = nil
      @socket_path = socket_path
      @exchanges.clear
    end

    # --- owner fiber -------------------------------------------------------

    private def run_loop
      # receive? returns nil when the channel is closed → clean shutdown.
      while req = @requests.receive?
        started = Time.instant
        result = roundtrip(req.cmd)
        record_exchange(req.cmd, result, Time.instant - started)
        req.reply.send(result)
      end
    end

    private def record_exchange(cmd : String, result : Result, duration : Time::Span) : Nil
      verb, _, args = cmd.partition(' ')
      at = Time.utc
      ok = result.is_a?(Ok)
      full_response, detail = case result
                              in Ok
                                json = result.payload.to_json
                                {json, snippet(json)}
                              in Err
                                text = "#{result.kind}: #{result.detail}"
                                {text, snippet(text)}
                              end
      exchange = Exchange.new(at, @session_id, cmd, verb, args, ok, detail, full_response, duration)
      @exchanges.push exchange
      @exchanges.shift if @exchanges.size > EXCHANGE_LOG_SIZE
      append_journal(exchange)
    end

    # Persist the exchange as one JSON line so the ledger survives a gateway
    # restart or a game crash. The file is append-only; replay_journal reads
    # it back when the client is constructed.
    private def append_journal(exchange : Exchange) : Nil
      path = @journal_path
      return unless path

      entry = {
        at:            exchange.at.to_rfc3339(fraction_digits: 3),
        session_id:    exchange.session_id,
        cmd:           exchange.cmd,
        verb:          exchange.verb,
        args:          exchange.args,
        ok:            exchange.ok,
        detail:        exchange.detail,
        response:      exchange.full_response,
        duration_ms:   exchange.duration.total_milliseconds,
      }.to_json

      File.open(path, "a") { |f| f.puts(entry) }
    rescue ex
      # Journaling must never break the game command. Log and move on.
      STDERR.puts "[journal] failed to append #{exchange.verb}: #{ex.message}"
    end

    # On startup, restore the most recent exchanges from the previous session
    # so the debug page is useful immediately after a crash/restart.
    private def replay_journal : Nil
      path = @journal_path
      return unless path && File.exists?(path)

      File.each_line(path) do |line|
        begin
          j = JSON.parse(line)
          duration = j["duration_ms"].as_f.milliseconds
          @exchanges.push Exchange.new(
            at:            Time.parse_rfc3339(j["at"].as_s),
            session_id:    j["session_id"].as_s,
            cmd:           j["cmd"].as_s,
            verb:          j["verb"].as_s,
            args:          j["args"].as_s,
            ok:            j["ok"].as_bool,
            detail:        j["detail"].as_s,
            full_response: j["response"].as_s,
            duration:      duration)
        rescue ex
          STDERR.puts "[journal] skipping corrupt line: #{ex.message}"
        end
      end

      # Keep only the in-memory window.
      while @exchanges.size > EXCHANGE_LOG_SIZE
        @exchanges.shift
      end
    end

    private def snippet(s : String) : String
      s.size > SNIPPET_LEN ? s[0, SNIPPET_LEN] + "…" : s
    end

    private def roundtrip(cmd : String) : Result
      sock = ensure_socket
      return Err.new(:disconnected, @last_error || "no connection") unless sock

      begin
        # The game's parse_json_cmd looks for a "cmd" field; the response is
        # exactly one line.
        sock.puts({cmd: cmd}.to_json)
        line = sock.gets
        unless line
          drop_socket "game closed the connection"
          return Err.new(:disconnected, "game closed the connection")
        end
        parse_response(cmd, line)
      rescue IO::TimeoutError
        # The reply may still arrive later; reusing this connection would pair
        # that stale reply with the NEXT request. Poisoned → drop it.
        drop_socket "timed out after #{@timeout}"
        Err.new(:timeout, "no response within #{@timeout}")
      rescue ex : IO::Error
        drop_socket ex.message || ex.class.name
        Err.new(:disconnected, ex.message || ex.class.name)
      end
    end

    private def parse_response(cmd : String, line : String) : Result
      payload = JSON.parse(line)

      # Request/response pairing check. Every game reply echoes the request
      # in its "cmd" field — game_controller verbs echo just the verb, the
      # legacy frontend chain echoes the full command line — so accept
      # either. Anything else means the stream is desynced (e.g. the game
      # emitted an unsolicited line) and EVERY later reply would pair with
      # the wrong request. Drop the connection — the next command reconnects
      # to a clean stream — and say exactly what happened.
      verb = cmd.partition(' ')[0]
      if (got = payload["cmd"]?.try(&.as_s?)) && got != verb && got != cmd
        drop_socket "response desync (sent '#{verb}', got reply for '#{got}')"
        return Err.new(:protocol,
          "response desync: sent '#{verb}' but the reply was addressed to " \
          "'#{got}' — dropped the connection to resync. raw: #{snippet(line)}")
      end

      Ok.new(payload)
    rescue ex : JSON::ParseException
      # Don't drop the connection: framing is still intact (we read a full
      # line), the payload was just garbage.
      Err.new(:protocol, "unparseable response from game: #{ex.message} — raw: #{snippet(line)}")
    end

    # Lazy connect: try on every request while disconnected. A Unix-socket
    # connect is local and fails fast, so per-request retry needs no backoff —
    # this is also what makes the gateway self-healing when the game restarts.
    private def ensure_socket : UNIXSocket?
      if sock = @socket
        return sock
      end
      sock = UNIXSocket.new(@socket_path)
      sock.read_timeout = @timeout
      sock.write_timeout = @timeout
      @socket = sock
      @connected = true
      @last_error = nil
      sock
    rescue ex : IO::Error | Socket::Error
      @last_error = ex.message || ex.class.name
      @connected = false
      nil
    end

    private def drop_socket(reason : String) : Nil
      @socket.try &.close rescue nil
      @socket = nil
      @connected = false
      @last_error = reason
    end
  end
end
