require "./spec_helper"
require "../src/game_process"

# GameProcess is exercised with throwaway shell stubs instead of the real
# game — lifecycle behaviour (start/monitor/stop/escalate) is binary-agnostic.
private def stub_binary(script : String) : String
  path = File.tempname("ctp2-stub")
  File.write(path, "#!/bin/sh\n#{script}\n")
  File.chmod(path, 0o755)
  path
end

describe Ctp2Gateway::GameProcess do
  it "starts a process and observes its exit" do
    bin = stub_binary("exit 0")
    proc = Ctp2Gateway::GameProcess.new(bin, [] of String, "/tmp", File.tempname("stub", ".log"))
    proc.start.should be_true
    proc.pid.should_not be_nil
    # The stub exits immediately; the monitor fiber should record it.
    proc.wait_exit(2.seconds).should be_true
    proc.running?.should be_false
    proc.exit_code.should eq 0
  ensure
    bin.try { |b| File.delete?(b) }
  end

  it "records a non-zero exit code" do
    bin = stub_binary("exit 3")
    proc = Ctp2Gateway::GameProcess.new(bin, [] of String, "/tmp", File.tempname("stub", ".log"))
    proc.start.should be_true
    proc.wait_exit(2.seconds).should be_true
    proc.exit_code.should eq 3
  ensure
    bin.try { |b| File.delete?(b) }
  end

  it "stop() terminates a long-running process" do
    bin = stub_binary("sleep 30")
    proc = Ctp2Gateway::GameProcess.new(bin, [] of String, "/tmp", File.tempname("stub", ".log"))
    proc.start.should be_true
    proc.running?.should be_true
    proc.stop(grace: 2.seconds)
    proc.running?.should be_false
    # SIGTERM death → no exit code, but the process is gone.
    proc.exit_code.should be_nil
  ensure
    bin.try { |b| File.delete?(b) }
  end

  it "escalates to SIGKILL when SIGTERM is ignored" do
    bin = stub_binary("trap '' TERM; sleep 30")
    proc = Ctp2Gateway::GameProcess.new(bin, [] of String, "/tmp", File.tempname("stub", ".log"))
    proc.start.should be_true
    # Give the shell a beat to install its trap, then stop with a short grace.
    sleep 100.milliseconds
    proc.stop(grace: 300.milliseconds)
    proc.running?.should be_false
  ensure
    bin.try { |b| File.delete?(b) }
  end

  it "returns false for a missing binary instead of raising" do
    proc = Ctp2Gateway::GameProcess.new("/no/such/binary", [] of String, "/tmp", "/tmp/x.log")
    proc.start.should be_false
    proc.running?.should be_false
  end

  it "passes args and cwd, and captures output in the log file" do
    log = File.tempname("stub", ".log")
    bin = stub_binary(%(echo "args:$@ cwd:$(pwd)"))
    proc = Ctp2Gateway::GameProcess.new(bin, ["--serve", "--seed", "7"], "/tmp", log)
    proc.start.should be_true
    proc.wait_exit(2.seconds).should be_true
    content = File.read(log)
    content.should contain "args:--serve --seed 7"
    # macOS: /tmp is a symlink to /private/tmp, so compare against realpath.
    content.should contain "cwd:#{File.realpath("/tmp")}"
  ensure
    bin.try { |b| File.delete?(b) }
    log.try { |l| File.delete?(l) }
  end
end
