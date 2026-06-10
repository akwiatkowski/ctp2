require "./spec_helper"
require "http/client"

# Boot the full HTTP stack (FakeGame ← GameClient ← Server) on an ephemeral
# port and yield a base URL for real HTTP requests.
private def with_http(responses = {} of String => String,
                      socket_path : String? = nil, &)
  fake = socket_path ? nil : FakeGame.new(responses)
  client = Ctp2Gateway::GameClient.new(socket_path || fake.not_nil!.path, timeout: 2.seconds)
  server = Ctp2Gateway::Server.new(client)
  address = server.bind("127.0.0.1", 0) # port 0 → OS picks a free one
  spawn { server.listen }
  begin
    yield "http://#{address}", fake
  ensure
    server.close
    client.close
    fake.try &.close
  end
end

describe Ctp2Gateway::Server do
  it "GET /healthz reports socket state" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/healthz")
      resp.status_code.should eq 200
      body = JSON.parse(resp.body)
      body["status"].as_s.should eq "ok"
      body["socket"].as_s.should_not be_empty
      # Lazy connect: no command issued yet, so not connected is fine —
      # the field just has to be present and boolean.
      body["connected"].as_bool?.should_not be_nil
    end
  end

  it "GET /api/cities maps to query_cities and passes the reply through" do
    with_http do |base, fake|
      resp = HTTP::Client.get("#{base}/api/cities")
      resp.status_code.should eq 200
      JSON.parse(resp.body)["cmd"].as_s.should eq "query_cities"
      fake.not_nil!.received.should eq ["query_cities"]
    end
  end

  it "GET /api/city/3 maps to 'query_city 3'" do
    with_http do |base, fake|
      HTTP::Client.get("#{base}/api/city/3").status_code.should eq 200
      fake.not_nil!.received.should eq ["query_city 3"]
    end
  end

  it "POST /api/cmd passes an arbitrary verb line through" do
    with_http do |base, fake|
      resp = HTTP::Client.post("#{base}/api/cmd", body: %({"cmd":"set_production 0 settler"}))
      resp.status_code.should eq 200
      fake.not_nil!.received.should eq ["set_production 0 settler"]
    end
  end

  it "game-level errors stay HTTP 200 (the game answered; that's a success)" do
    canned = {"build_city" => %({"status":"error","cmd":"build_city","detail":"game_not_loaded"})}
    with_http(canned) do |base, _fake|
      resp = HTTP::Client.post("#{base}/api/cmd", body: %({"cmd":"build_city"}))
      resp.status_code.should eq 200
      body = JSON.parse(resp.body)
      body["status"].as_s.should eq "error"
      body["gateway"]?.should be_nil # the game's error, not ours
    end
  end

  it "POST /api/cmd with invalid JSON body → 400 gateway error" do
    with_http do |base, _fake|
      resp = HTTP::Client.post("#{base}/api/cmd", body: "not json")
      resp.status_code.should eq 400
      JSON.parse(resp.body)["gateway"].as_bool.should be_true
    end
  end

  it "POST /api/cmd without a cmd field → 400" do
    with_http do |base, _fake|
      HTTP::Client.post("#{base}/api/cmd", body: %({"verb":"x"})).status_code.should eq 400
    end
  end

  it "POST /api/cmd with an embedded newline → 400 (no protocol injection)" do
    with_http do |base, fake|
      resp = HTTP::Client.post("#{base}/api/cmd", body: %({"cmd":"build_city\\nquit"}))
      resp.status_code.should eq 400
      fake.not_nil!.received.should be_empty
    end
  end

  it "unknown route → 404 gateway error" do
    with_http do |base, _fake|
      resp = HTTP::Client.get("#{base}/api/nope")
      resp.status_code.should eq 404
      JSON.parse(resp.body)["gateway"].as_bool.should be_true
    end
  end

  it "no game socket → 503" do
    with_http(socket_path: "/tmp/ctp2-gateway-spec-absent.sock") do |base, _fake|
      resp = HTTP::Client.get("#{base}/api/units")
      resp.status_code.should eq 503
      body = JSON.parse(resp.body)
      body["gateway"].as_bool.should be_true
      body["kind"].as_s.should eq "disconnected"
    end
  end
end
