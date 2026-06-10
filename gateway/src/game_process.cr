require "json"

module Ctp2Gateway
  # Supervisor for a gateway-spawned game process (--spawn).
  #
  # Lifecycle: start() launches the binary and a monitor fiber that waits for
  # it; the child's stdout/stderr go to a log file (the game's spdlog output
  # would otherwise interleave with the gateway's own). stop() sends SIGTERM
  # and escalates to SIGKILL after a grace period.
  #
  # Deliberately NO auto-respawn: a crashing game would loop, and the lazy
  # reconnect in GameClient already heals the connection when Olek (or a
  # later supervisor mode) restarts it. /healthz exposes pid + exit status
  # so the operator can tell "game died" from "game never started".
  class GameProcess
    getter binary : String
    getter args : Array(String)
    getter log_path : String
    getter pid : Int64?
    getter exit_code : Int32?
    getter? running : Bool = false

    def initialize(@binary : String, @args : Array(String),
                   @chdir : String, @log_path : String)
      # Closed by the monitor fiber when the child exits; lets stop() wait
      # for termination without polling.
      @done = Channel(Nil).new
    end

    # Launch the child. Returns false (with a message on STDERR) instead of
    # raising when the binary is missing/not executable, so the gateway can
    # keep serving in attach mode.
    def start : Bool
      unless File.exists?(@binary) && File::Info.executable?(@binary)
        STDERR.puts "[spawn] binary not found or not executable: #{@binary}"
        return false
      end

      log = File.open(@log_path, "a")
      process = Process.new(@binary, @args, chdir: @chdir, output: log, error: log)
      @process = process
      @pid = process.pid
      @running = true
      puts "[spawn] started #{@binary} #{@args.join(' ')} (pid #{process.pid}, log #{@log_path}, cwd #{@chdir})"

      spawn(name: "game-process-monitor") do
        status = process.wait
        log.close rescue nil
        @running = false
        # exit_code? is nil when the child died from a signal (e.g. our
        # SIGKILL escalation) — /healthz then shows running:false, code:null.
        @exit_code = status.exit_code?
        puts "[spawn] game exited (#{status.success? ? "ok" : @exit_code ? "code #{@exit_code}" : "signal"})"
        @done.close
      end
      true
    end

    # SIGTERM, then SIGKILL after the grace period. Safe to call when the
    # child already exited (or never started).
    def stop(grace : Time::Span = 5.seconds) : Nil
      process = @process
      return unless process && @running

      process.terminate rescue nil
      select
      when @done.receive?
        # monitor saw the exit — clean shutdown
      when timeout(grace)
        STDERR.puts "[spawn] game ignored SIGTERM for #{grace}, sending SIGKILL"
        process.signal(Signal::KILL) rescue nil
        @done.receive? # KILL is not ignorable; monitor will close promptly
      end
    end

    # Block until the child exits (true) or the timeout fires (false).
    # Used by specs and useful for future supervisor logic.
    def wait_exit(timeout : Time::Span) : Bool
      select
      when @done.receive?
        true
      when timeout(timeout)
        false
      end
    end

    # Shape consumed by /healthz.
    def status_json : NamedTuple(pid: Int64?, running: Bool, exit_code: Int32?)
      {pid: @pid, running: @running, exit_code: @exit_code}
    end

    @process : Process?
  end
end
