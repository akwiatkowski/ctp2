require "spec"
require "../src/server"

# In-process stand-in for the game's smoke server (smoketest_server.cpp):
# a UNIXServer speaking the same newline-delimited JSON protocol, with knobs
# for the failure modes the GameClient must survive.
#
#   responses  — canned reply per verb line; anything else gets a default
#                {"status":"ok","cmd":<verb>}. A canned value can be garbage
#                (non-JSON) to exercise the protocol-error path.
#   delay      — sleep before every reply (drives client timeouts / busy).
#   close_at   — close the connection INSTEAD of answering request number N
#                (1-based, counted across connections) — drives reconnect.
class FakeGame
  getter path : String
  getter received = [] of String

  def initialize(@responses : Hash(String, String) = {} of String => String,
                 @delay : Time::Span? = nil,
                 @close_at : Int32? = nil)
    @path = File.tempname("ctp2-fake", ".sock")
    @server = UNIXServer.new(@path)
    @count = 0
    spawn(name: "fake-game-accept") { accept_loop }
  end

  def close : Nil
    @server.close
    File.delete?(@path)
  end

  private def accept_loop
    # The real game serves one client at a time; so do we.
    while client = @server.accept?
      serve(client)
    end
  end

  private def serve(client : UNIXSocket)
    while line = client.gets
      cmd = JSON.parse(line)["cmd"].as_s
      @received << cmd
      @count += 1
      if @close_at == @count
        client.close
        return
      end
      if d = @delay
        sleep d
      end
      client.puts(@responses[cmd]? || %({"status":"ok","cmd":"#{cmd}"}))
    end
  rescue IO::Error
    # Client vanished mid-conversation (e.g. it timed out and dropped the
    # poisoned connection while we were sleeping) — go accept the next one.
  ensure
    client.close rescue nil
  end
end

# Build a FakeGame + GameClient pair, run the block, tear both down.
def with_client(responses = {} of String => String,
                delay : Time::Span? = nil,
                close_at : Int32? = nil,
                timeout : Time::Span = 2.seconds,
                queue_capacity : Int32 = 32, &)
  fake = FakeGame.new(responses, delay, close_at)
  client = Ctp2Gateway::GameClient.new(fake.path, timeout: timeout, queue_capacity: queue_capacity)
  begin
    yield client, fake
  ensure
    client.close
    fake.close
  end
end
