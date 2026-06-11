require "./spec_helper"

# True end-to-end smoke: a REAL ctp2_headless binary behind the full
# gateway stack (HTTP routing -> DI -> GameClient -> Unix socket ->
# game_controller). This is the only spec that can catch stack-integration
# breaks (cmd-echo desync, protocol drift, serve-mode regressions) that
# FakeGame, by construction, cannot.
#
# Gated: runs only when CTP2_HEADLESS points at a binary, e.g.
#
#     CTP2_HEADLESS=../build/ctp2_headless crystal spec spec/e2e_spec.cr
#
# (or `make gateway-e2e` from the repo root). Without the variable the
# spec reports itself skipped and stays green, so plain `crystal spec`
# never needs the C++ build.
struct E2ESmokeTest < ATH::Spec::APITestCase
  SOCKET = Ctp2Gateway::Config::DEFAULT_SOCKET

  @game : Process? = nil

  def initialize
    super
    if binary = ENV["CTP2_HEADLESS"]?
      File.delete?(SOCKET)
      root = File.expand_path("../..", __DIR__)
      log = File.open("/tmp/ctp2-e2e-spec.log", "w")
      @game = Process.new(File.expand_path(binary),
        ["--serve", "--players", "3", "--seed", "42"],
        chdir: root, output: log, error: log)
      deadline = Time.monotonic + 90.seconds
      until File.exists?(SOCKET)
        raise "game never opened #{SOCKET} (see /tmp/ctp2-e2e-spec.log)" if Time.monotonic > deadline
        sleep 0.2.seconds
      end
      Ctp2Gateway::Config.reset_client!(SOCKET)
    end
  end

  def tear_down : Nil
    if game = @game
      game.terminate rescue nil
      game.wait rescue nil
      File.delete?(SOCKET)
    end
  end

  private def cmd(line : String) : JSON::Any
    resp = self.post("/api/cmd", body: {cmd: line}.to_json,
      headers: HTTP::Headers{"Content-Type" => "application/json"})
    JSON.parse(resp.body)
  end

  def test_full_stack_against_real_game : Nil
    unless @game
      puts "  [e2e] skipped: set CTP2_HEADLESS=<path-to-ctp2_headless> to run"
      return
    end

    cmd("new_game")["status"].as_s.should eq "ok"
    cmd("start_game")["status"].as_s.should eq "ok"

    # The game finishes initialising asynchronously; poll until the clock
    # answers, then drive one human action and a couple of rounds.
    deadline = Time.monotonic + 60.seconds
    until cmd("query_turn")["status"].as_s == "ok"
      raise "game never became ready" if Time.monotonic > deadline
      sleep 0.5.seconds
    end

    cmd("build_city")["status"].as_s.should eq "ok"

    round0 = cmd("query_turn")["result"]["round"].as_i
    cmd("end_turn 2")["status"].as_s.should eq "ok"
    cmd("query_turn")["result"]["round"].as_i.should eq round0 + 2

    # REST face: live players, with the one human present.
    players = JSON.parse(self.get("/api/players").body)["result"]["players"].as_a
    players.any? { |p| p["human"].as_bool }.should be_true

    # MCP face: a model-visible tool runs against the live game.
    mcp = JSON.parse(self.post("/mcp",
      body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"render_map"}}),
      headers: HTTP::Headers{"Content-Type" => "application/json"}).body)
    mcp["result"]["isError"].as_bool.should be_false
    mcp["result"]["content"].as_a.first["text"].as_s.should contain "you are player"

    # Admin face: the illuminated chart renders the live world.
    page = self.get("/map")
    page.status_code.should eq 200
    page.body.should contain %(id="atlas")

    puts "  [e2e] full stack ok: REST + MCP + admin against a live game"
  end
end
