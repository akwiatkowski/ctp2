require "./spec_helper"
require "http/client"

# Canned admin-query responses matching the real game_controller.cpp shapes.
private PLAYERS_JSON = %({"status":"ok","cmd":"query_players","result":{"players":[) +
                       %({"id":0,"name":"Barbarians","human":false,"dead":false,"gold":0,"num_cities":0,"score":0},) +
                       %({"id":1,"name":"Caesar <Rome>","human":true,"dead":false,"gold":300,"num_cities":2,"score":25},) +
                       %({"id":2,"name":"Gandhi","human":false,"dead":true,"gold":0,"num_cities":0,"score":3}]}})

private CITIES_JSON = %({"status":"ok","cmd":"query_player_cities","result":{"owner":1,"cities":[) +
                      %({"owner":1,"index":0,"name":"Rome <b>","pos":{"x":31,"y":10},"population":2,) +
                      %("building":{"category":1,"type":54,"cost":740}},) +
                      %({"owner":1,"index":1,"name":"Ostia","pos":{"x":35,"y":12},"population":1,"building":null}]}})

private def admin_responses
  {
    "query_players"         => PLAYERS_JSON,
    "query_player_cities 1" => CITIES_JSON,
    "query_player_cities 7" => %({"status":"error","cmd":"query_player_cities","detail":"bad_player"}),
  }
end

private def with_http(responses = admin_responses, socket_path : String? = nil, &)
  fake = socket_path ? nil : FakeGame.new(responses)
  client = Ctp2Gateway::GameClient.new(socket_path || fake.not_nil!.path, timeout: 2.seconds)
  server = Ctp2Gateway::Server.new(client)
  address = server.bind("127.0.0.1", 0)
  spawn { server.listen }
  begin
    yield "http://#{address}", fake
  ensure
    server.close
    client.close
    fake.try &.close
  end
end

describe "admin panel" do
  it "GET / renders the dashboard with nav and game summary" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/")
      resp.status_code.should eq 200
      resp.headers["Content-Type"].should contain "text/html"
      resp.body.should contain %(>Players</a>) # masthead nav
      resp.body.should contain %(data-stat="players">3<)
      resp.body.should contain %(data-stat="alive">2<)
      resp.body.should contain %(data-stat="cities">2<)
      resp.body.should contain "Caesar &lt;Rome&gt;" # leading player, escaped
    end
  end

  it "GET / still renders (200) when no game socket exists" do
    with_http(socket_path: "/tmp/ctp2-gateway-spec-absent.sock") do |base, _fake|
      resp = HTTP::Client.get("#{base}/")
      resp.status_code.should eq 200
      resp.body.should contain "game unavailable"
    end
  end

  it "GET /players lists players with links to their cities" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/players")
      resp.status_code.should eq 200
      resp.body.should contain "Caesar &lt;Rome&gt;" # game data is HTML-escaped
      resp.body.should contain %(<a href="/players/1/cities">)
      resp.body.should contain "dead" # Gandhi
    end
  end

  it "GET /players/1/cities renders the city table with the leader name" do
    with_http do |base, fake|
      resp = HTTP::Client.get("#{base}/players/1/cities")
      resp.status_code.should eq 200
      resp.body.should contain "Caesar &lt;Rome&gt;"
      resp.body.should contain "Rome &lt;b&gt;" # escaped city name
      resp.body.should contain "(31, 10)"
      resp.body.should contain "category 1 · type 54"
      resp.body.should contain "cost 740"
      resp.body.should contain "idle" # Ostia builds nothing
      fake.not_nil!.received.first.should eq "query_player_cities 1"
    end
  end

  it "GET /players/7/cities → 404 for a bad player id" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/players/7/cities")
      resp.status_code.should eq 404
      resp.body.should contain "bad_player"
    end
  end

  it "GET /api/players passes the admin query through as JSON" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/api/players")
      resp.status_code.should eq 200
      body = JSON.parse(resp.body)
      body["result"]["players"].as_a.size.should eq 3
    end
  end

  it "GET /api/players/1/cities passes through as JSON" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/api/players/1/cities")
      resp.status_code.should eq 200
      JSON.parse(resp.body)["result"]["owner"].as_i.should eq 1
    end
  end
end

describe "assets and fragments" do
  it "serves vendored static assets with the right content type" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/assets/app.css")
      resp.status_code.should eq 200
      resp.headers["Content-Type"].should contain "text/css"
      resp.body.should contain "THE CHANCELLERY"
      HTTP::Client.get("#{base}/assets/htmx.min.js").status_code.should eq 200
      HTTP::Client.get("#{base}/assets/fonts/fraunces-normal.woff2").status_code.should eq 200
    end
  end

  it "blocks path traversal out of public/" do
    with_http do |base, _fake|
      HTTP::Client.get("#{base}/assets/../shard.yml").status_code.should eq 404
      HTTP::Client.get("#{base}/assets/%2e%2e/shard.yml").status_code.should eq 404
    end
  end

  it "GET /fragments/dashboard returns the bare ledger fragment" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/fragments/dashboard")
      resp.status_code.should eq 200
      resp.body.should contain %(data-stat="players">3<)
      resp.body.should_not contain "<html" # fragment, not a full page
    end
  end
end
