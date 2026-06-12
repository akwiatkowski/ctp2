require "spec"
require "file_utils"

# Sources first (they pull "athena"); athena/spec's component spec helpers
# reference framework constants and must load after it.
require "../src/config"
require "../src/services"
require "../src/error_listener"
require "../src/assets_handler"
require "../src/controllers/pages_controller"
require "../src/controllers/api_controller"
require "../src/controllers/mcp_controller"

require "athena/spec"

# Keep spec session journals out of the repo session dir.
SPEC_SESSION_DIR = File.join(Dir.tempdir, "ctp2-gateway-spec-#{Process.pid}-#{Random::Secure.hex(4)}")
FileUtils.mkdir_p(SPEC_SESSION_DIR)
Ctp2Gateway::Config.session_dir = SPEC_SESSION_DIR

at_exit { FileUtils.rm_rf(SPEC_SESSION_DIR) }

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

  @closed = false

  def initialize(@responses : Hash(String, String) = {} of String => String,
                 @delay : Time::Span? = nil,
                 @close_at : Int32? = nil)
    @path = File.tempname("ctp2-fake", ".sock")
    @server = UNIXServer.new(@path)
    @count = 0
    spawn(name: "fake-game-accept") { accept_loop }
  end

  def close : Nil
    # Idempotent: the test-case lifecycle closes the previous fake from the
    # NEXT test's initialize as well as from tear_down.
    return if @closed
    @closed = true
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

# Build a FakeGame + plain (non-DI) GameClient pair, run the block, tear
# both down. Used by the framework-free game_client specs.
def with_client(responses = {} of String => String,
                 delay : Time::Span? = nil,
                 close_at : Int32? = nil,
                 timeout : Time::Span = 2.seconds,
                 queue_capacity : Int32 = 32,
                 journal_path : String? = nil, &)
  fake = FakeGame.new(responses, delay, close_at)
  client = Ctp2Gateway::GameClient.new(fake.path,
    timeout: timeout, queue_capacity: queue_capacity,
    session_id: "spec-session", journal_path: journal_path)
  begin
    yield client, fake
  ensure
    client.close
    fake.close
  end
end

# Canned admin-query responses matching the real game_controller.cpp shapes.
PLAYERS_JSON = %({"status":"ok","cmd":"query_players","result":{"players":[) +
               %({"id":0,"name":"Barbarians","civ":"","country":"","human":false,"dead":false,"gold":0,"num_cities":0,"score":0},) +
               %({"id":1,"name":"Caesar <Rome>","civ":"Roman","country":"Rome <Empire>","human":true,"dead":false,"gold":300,"num_cities":2,"score":25},) +
               %({"id":2,"name":"Gandhi","civ":"Indian","country":"India","human":false,"dead":true,"gold":0,"num_cities":0,"score":3}]}})

CITIES_JSON = %({"status":"ok","cmd":"query_player_cities","result":{"owner":1,"cities":[) +
              %({"owner":1,"index":0,"name":"Rome <b>","pos":{"x":31,"y":10},"population":2,) +
              %("building":{"category":1,"type":54,"cost":740}},) +
              %({"owner":1,"index":1,"name":"Ostia","pos":{"x":35,"y":12},"population":1,"building":null}]}})

# Action Log sample: two narratable beats in round 2 (a city + a filtered
# order), one in round 4, one in round 5 — exercises grouping, name lookup,
# location rendering, and the order-filtering the chronicle does.
LOG_JSON = %({"status":"ok","cmd":"log_get","result":{"count":4,"action_log":[) +
           %({"turn":2,"player":1,"event":"CreateCity","args":[{"kind":"player","value":1},{"kind":"location","x":18,"y":39,"z":0},{"kind":"city","id":1,"out":true}]},) +
           %({"turn":2,"player":1,"event":"MoveOrder","args":[{"kind":"army","id":7}]},) +
           %({"turn":4,"player":1,"event":"GrantAdvance","args":[{"kind":"player","value":1}]},) +
           %({"turn":5,"player":2,"event":"Battle","args":[{"kind":"location","x":20,"y":40,"z":0}]}]}})

def admin_responses
  {
    "query_players"         => PLAYERS_JSON,
    "query_turn"            => %({"status":"ok","cmd":"query_turn","result":{"round":42,"year":-3160}}),
    "query_player 1"        => %({"status":"ok","cmd":"query_player","result":{"id":1,"name":"Caesar <Rome>","num_units":5}}),
    "query_player_cities 1" => CITIES_JSON,
    "query_player_cities 7" => %({"status":"error","cmd":"query_player_cities","detail":"bad_player"}),
    "log_get"               => LOG_JSON,
  }
end

# Base for kernel-level request specs: each test method gets a fresh
# FakeGame and points the PROCESS-level GameClient singleton (the one the
# DI factory hands out — see Config.client) at it. No real HTTP server.
abstract struct GatewayTestCase < ATH::Spec::APITestCase
  # The inline default exists for ASPEC's allocation-time construct() —
  # Crystal runs ivar defaults at ALLOCATION, not on each initialize call.
  # ASPEC reuses ONE instance and re-calls #initialize before every test
  # (before_all and before_each), so the per-test fake MUST be assigned in
  # the body; relying on the default alone left every test after the first
  # pointing at a closed fake whose socket file tear_down had deleted.
  @fake : FakeGame = FakeGame.new(::admin_responses)

  def initialize
    @fake.close # previous test's fake (or the construct-time orphan)
    @fake = FakeGame.new(::admin_responses)
    Ctp2Gateway::Config.reset_client!(@fake.path)
    super
  end

  def tear_down : Nil
    @fake.close
  end

  protected def fake : FakeGame
    @fake
  end

  # Point the singleton client at a dead socket (disconnected scenarios).
  protected def disconnect_game! : Nil
    Ctp2Gateway::Config.reset_client!("/tmp/ctp2-gateway-spec-absent.sock")
  end
end
