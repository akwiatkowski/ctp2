require "./spec_helper"

# Kernel-level request specs (ATH::Spec::APITestCase — no real HTTP server;
# requests go straight into Athena's route handler). The game side is a real
# Unix-socket FakeGame per test, wired through the process-level GameClient
# singleton. Executed by Athena::Spec.run_all in zz_athena_runner_spec.cr.

struct PagesTest < GatewayTestCase
  def test_dashboard_renders_summary : Nil
    resp = self.get("/")
    resp.status_code.should eq 200
    resp.headers["Content-Type"].should contain "text/html"
    resp.body.should contain %(>Players</a>) # masthead nav
    resp.body.should contain %(data-stat="players">3<)
    resp.body.should contain %(data-stat="alive">2<)
    resp.body.should contain %(data-stat="cities">2<)
    resp.body.should contain %(data-stat="round">42<)
    resp.body.should contain "3160 BC"
    resp.body.should contain "Caesar &lt;Rome&gt;" # leading player, escaped
  end

  def test_dashboard_renders_when_game_is_down : Nil
    self.disconnect_game!
    resp = self.get("/")
    resp.status_code.should eq 200
    resp.body.should contain "game unavailable"
  end

  def test_players_table_links_and_escapes : Nil
    resp = self.get("/players")
    resp.status_code.should eq 200
    resp.body.should contain "Caesar &lt;Rome&gt;"        # leader, escaped
    resp.body.should contain "Rome &lt;Empire&gt;"        # civilization column
    resp.body.should contain %(<a href="/players/1/cities">)
    resp.body.should contain "dead" # Gandhi
  end

  def test_player_cities_table : Nil
    resp = self.get("/players/1/cities")
    resp.status_code.should eq 200
    resp.body.should contain "Caesar &lt;Rome&gt;" # leader name lookup
    resp.body.should contain "Rome &lt;b&gt;"      # escaped city name
    resp.body.should contain "(31, 10)"
    resp.body.should contain "category 1 · type 54"
    resp.body.should contain "cost 740"
    resp.body.should contain "idle" # Ostia builds nothing
    self.fake.received.first.should eq "query_player_cities 1"
  end

  def test_player_cities_bad_player_is_404 : Nil
    self.get("/players/7/cities").status_code.should eq 404
  end

  def test_player_cities_non_numeric_id_is_404 : Nil
    # requirements: {"id" => /\d+/} — no route matches.
    self.get("/players/abc/cities").status_code.should eq 404
  end

  def test_fragment_is_bare : Nil
    resp = self.get("/fragments/dashboard")
    resp.status_code.should eq 200
    resp.body.should contain %(data-stat="players">3<)
    resp.body.should_not contain "<html" # fragment, not a full page
  end

  def test_debug_page_shows_exchange_journal : Nil
    self.get("/api/players") # produce one exchange
    resp = self.get("/debug")
    resp.status_code.should eq 200
    resp.body.should contain "query_players"
    resp.body.should contain %(class="badge badge--ok")
    resp.body.should contain "/debug/boom"
  end

  def test_tools_catalog_renders_registry : Nil
    resp = self.get("/tools")
    resp.status_code.should eq 200
    resp.body.should contain "MCP tools"
    resp.body.should contain "move_army"          # tool name
    resp.body.should contain "army_index"         # schema property
    resp.body.should contain "max 20"             # end_turn constraint
    resp.body.should contain "claude mcp add"     # registration crib
  end

  def test_html_boom_renders_full_debug_error : Nil
    resp = self.get("/debug/boom")
    resp.status_code.should eq 500
    resp.body.should contain "intentional test exception"
    resp.body.should contain "Backtrace"
    resp.body.should contain "Exception"
  end
end

struct ApiTest < GatewayTestCase
  def test_api_players_passthrough : Nil
    resp = self.get("/api/players")
    resp.status_code.should eq 200
    JSON.parse(resp.body)["result"]["players"].as_a.size.should eq 3
  end

  def test_api_player_detail : Nil
    resp = self.get("/api/players/1")
    resp.status_code.should eq 200
    JSON.parse(resp.body)["result"]["num_units"].as_i.should eq 5
    self.fake.received.should eq ["query_player 1"]
  end

  def test_api_player_cities : Nil
    resp = self.get("/api/players/1/cities")
    JSON.parse(resp.body)["result"]["owner"].as_i.should eq 1
    self.fake.received.should eq ["query_player_cities 1"]
  end

  def test_api_turn : Nil
    resp = self.get("/api/turn")
    JSON.parse(resp.body)["result"]["round"].as_i.should eq 42
    self.fake.received.should eq ["query_turn"]
  end

  def test_raw_cmd_passthrough : Nil
    resp = self.post("/api/cmd", body: %({"cmd":"set_production 0 settler"}))
    resp.status_code.should eq 200
    self.fake.received.should eq ["set_production 0 settler"]
  end

  def test_game_level_errors_stay_200 : Nil
    resp = self.get("/api/players/7/cities")
    resp.status_code.should eq 200
    body = JSON.parse(resp.body)
    body["status"].as_s.should eq "error"
    body["gateway"]?.should be_nil # the game's error, not ours
  end

  def test_bad_json_body_is_400 : Nil
    resp = self.post("/api/cmd", body: "not json")
    resp.status_code.should eq 400
    JSON.parse(resp.body)["gateway"].as_bool.should be_true
  end

  def test_missing_cmd_field_is_400 : Nil
    self.post("/api/cmd", body: %({"verb":"x"})).status_code.should eq 400
  end

  def test_newline_injection_is_400 : Nil
    resp = self.post("/api/cmd", body: %({"cmd":"build_city\\nquit"}))
    resp.status_code.should eq 400
    self.fake.received.should be_empty
  end

  def test_unknown_route_is_404_gateway_error : Nil
    resp = self.get("/api/nope")
    resp.status_code.should eq 404
    JSON.parse(resp.body)["gateway"].as_bool.should be_true
  end

  def test_disconnected_game_is_503 : Nil
    self.disconnect_game!
    resp = self.get("/api/units")
    resp.status_code.should eq 503
    body = JSON.parse(resp.body)
    body["gateway"].as_bool.should be_true
    body["kind"].as_s.should eq "disconnected"
  end

  def test_json_boom_carries_backtrace_and_exchanges : Nil
    self.get("/api/players") # seed the journal
    resp = self.get("/api/debug/boom")
    resp.status_code.should eq 500
    body = JSON.parse(resp.body)
    body["gateway"].as_bool.should be_true
    body["kind"].as_s.should eq "exception"
    body["message"].as_s.should contain "intentional"
    body["backtrace"].as_a.should_not be_empty
    body["exchanges"].as_a.first["cmd"].as_s.should eq "query_players"
  end

  def test_api_index_is_self_describing : Nil
    resp = self.get("/api")
    resp.status_code.should eq 200
    body = JSON.parse(resp.body)
    body["service"].as_s.should eq "ctp2-gateway"
    body["endpoints"].as_a.any? { |e| e["path"].as_s == "/api/turn" }.should be_true
    body["verbs"]["queries_admin"].as_a.map(&.as_s).should contain "query_turn"
    body["verbs"]["commands"].as_a.any?(&.as_s.starts_with?("end_turn")).should be_true
    body["status_contract"]["503"].as_s.should contain "disconnected"
  end

  def test_healthz_reports_state : Nil
    resp = self.get("/healthz")
    resp.status_code.should eq 200
    body = JSON.parse(resp.body)
    body["status"].as_s.should eq "ok"
    body["connected"].as_bool?.should_not be_nil
  end
end

# /assets is served by a prepend handler BEFORE Athena (ATH.run
# prepend_handlers), so it can't be exercised through APITestCase — drive
# the HTTP::Handler directly instead.
private def run_asset(path : String) : HTTP::Server::Response
  handler = Ctp2Gateway::AssetsHandler.new
  handler.next = ->(ctx : HTTP::Server::Context) { ctx.response.status_code = 599 } # fall-through marker
  io = IO::Memory.new
  request = HTTP::Request.new("GET", path)
  response = HTTP::Server::Response.new(io)
  handler.call(HTTP::Server::Context.new(request, response))
  response
end

describe Ctp2Gateway::AssetsHandler do
  it "serves vendored assets with the right content type" do
    resp = run_asset("/assets/app.css")
    resp.status_code.should eq 200
    resp.headers["Content-Type"].should contain "text/css"
    resp.body.should contain "THE CHANCELLERY"
    run_asset("/assets/htmx.min.js").status_code.should eq 200
    run_asset("/assets/fonts/fraunces-normal.woff2").status_code.should eq 200
  end

  it "blocks path traversal out of public/" do
    run_asset("/assets/../shard.yml").status_code.should eq 404
  end

  it "falls through to the next handler for non-asset paths" do
    run_asset("/players").status_code.should eq 599
  end
end
