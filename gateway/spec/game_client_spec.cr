require "./spec_helper"

alias GC2 = Ctp2Gateway::GameClient

describe Ctp2Gateway::GameClient do
  it "round-trips a command and parses the reply" do
    with_client do |client, fake|
      result = client.command("query_cities")
      result = result.should be_a(GC2::Ok)
      result.payload["status"].as_s.should eq "ok"
      result.payload["cmd"].as_s.should eq "query_cities"
      fake.received.should eq ["query_cities"]
      client.connected?.should be_true
    end
  end

  it "passes the game's own error replies through as Ok results" do
    canned = {"build_city" => %({"status":"error","cmd":"build_city","detail":"no_settler_found"})}
    with_client(canned) do |client, _fake|
      result = client.command("build_city").should be_a(GC2::Ok)
      result.payload["status"].as_s.should eq "error"
      result.payload["detail"].as_s.should eq "no_settler_found"
    end
  end

  it "rejects multi-line commands before they reach the socket" do
    with_client do |client, fake|
      result = client.command("build_city\nquit").should be_a(GC2::Err)
      result.kind.protocol?.should be_true
      fake.received.should be_empty
    end
  end

  it "reports Disconnected when the socket path does not exist" do
    client = Ctp2Gateway::GameClient.new("/tmp/ctp2-gateway-spec-no-such.sock")
    begin
      result = client.command("query_units").should be_a(GC2::Err)
      result.kind.disconnected?.should be_true
      client.connected?.should be_false
      client.last_error.should_not be_nil
    ensure
      client.close
    end
  end

  it "times out on a slow game and drops the poisoned connection" do
    with_client(delay: 500.milliseconds, timeout: 100.milliseconds) do |client, _fake|
      result = client.command("query_map").should be_a(GC2::Err)
      result.kind.timeout?.should be_true
      client.connected?.should be_false
    end
  end

  it "survives a mid-stream disconnect and reconnects on the next command" do
    with_client(close_at: 2) do |client, fake|
      client.command("query_cities").should be_a(GC2::Ok)
      # Request 2: fake closes instead of answering → EOF.
      result = client.command("query_units").should be_a(GC2::Err)
      result.kind.disconnected?.should be_true
      # Request 3: lazy reconnect picks up a fresh connection.
      client.command("query_map").should be_a(GC2::Ok)
      fake.received.should eq ["query_cities", "query_units", "query_map"]
    end
  end

  it "keeps the connection on a garbage (non-JSON) reply and recovers" do
    canned = {"query_map" => "this is not json"}
    with_client(canned) do |client, _fake|
      result = client.command("query_map").should be_a(GC2::Err)
      result.kind.protocol?.should be_true
      # Framing was intact (one full line), so the next command still works.
      client.command("query_units").should be_a(GC2::Ok)
    end
  end

  it "applies backpressure (Busy) when the queue is full" do
    with_client(delay: 200.milliseconds, queue_capacity: 1) do |client, _fake|
      results = Channel(GC2::Result).new(5)
      5.times { spawn { results.send client.command("query_units") } }
      collected = Array(GC2::Result).new(5) { results.receive }
      collected.count(&.is_a?(GC2::Ok)).should be >= 1
      collected.count { |r| r.is_a?(GC2::Err) && r.kind.busy? }.should be >= 1
    end
  end

  it "serializes concurrent callers onto the single game connection" do
    with_client do |client, fake|
      results = Channel(GC2::Result).new(10)
      10.times { |i| spawn { results.send client.command("query_city #{i}") } }
      10.times { results.receive.should be_a(GC2::Ok) }
      # All ten arrived, each as a complete line (no interleaving possible
      # on one connection driven by one owner fiber).
      fake.received.size.should eq 10
      fake.received.sort.should eq (0...10).map { |i| "query_city #{i}" }.sort
    end
  end
end
