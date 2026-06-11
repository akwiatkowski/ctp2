require "./spec_helper"

# MCP endpoint specs — JSON-RPC over the kernel, FakeGame behind.
struct McpTest < GatewayTestCase
  private def rpc(payload : String) : ::HTTP::Server::Response
    self.post("/mcp", body: payload)
  end

  private def rpc_json(payload : String) : JSON::Any
    JSON.parse(rpc(payload).body)
  end

  def test_initialize_negotiates_protocol_and_issues_session : Nil
    resp = rpc(%({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"spec","version":"0"}}}))
    resp.status_code.should eq 200
    resp.headers["Mcp-Session-Id"]?.should_not be_nil
    body = JSON.parse(resp.body)
    body["result"]["protocolVersion"].as_s.should eq "2025-06-18"
    body["result"]["serverInfo"]["name"].as_s.should eq "ctp2-gateway"
    body["result"]["capabilities"]["tools"]?.should_not be_nil
    body["result"]["instructions"].as_s.should contain "start_game"
  end

  def test_initialize_falls_back_on_unknown_version : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"1999-01-01"}}))
    body["result"]["protocolVersion"].as_s.should eq "2025-06-18"
  end

  def test_initialized_notification_gets_202 : Nil
    rpc(%({"jsonrpc":"2.0","method":"notifications/initialized"})).status_code.should eq 202
  end

  def test_ping : Nil
    rpc_json(%({"jsonrpc":"2.0","id":7,"method":"ping"}))["result"].as_h.should be_empty
  end

  def test_tools_list_shape : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":2,"method":"tools/list"}))
    tools = body["result"]["tools"].as_a
    tools.size.should eq Ctp2Gateway::Mcp::TOOLS.size
    names = tools.map(&.["name"].as_s)
    %w[start_game end_turn build_city set_production query_turn query_players gateway_health raw_cmd].each do |n|
      names.should contain n
    end
    end_turn = tools.find! { |t| t["name"].as_s == "end_turn" }
    end_turn["inputSchema"]["properties"]["turns"]["maximum"].as_i.should eq 20
  end

  def test_tools_call_runs_a_query : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"query_turn","arguments":{}}}))
    body["result"]["isError"].as_bool.should be_false
    body["result"]["content"].as_a.first["text"].as_s.should contain %("round": 42)
    self.fake.received.should eq ["query_turn"]
  end

  def test_start_game_is_composite : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"start_game"}}))
    body["result"]["isError"].as_bool.should be_false
    self.fake.received.should eq ["new_game", "start_game"]
  end

  def test_tool_args_map_onto_verbs : Nil
    rpc_json(%({"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"set_production","arguments":{"city_index":0,"what":"settler"}}}))
    self.fake.received.should eq ["set_production 0 settler"]
  end

  def test_game_error_becomes_tool_error : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"query_player_cities","arguments":{"player_id":7}}}))
    body["result"]["isError"].as_bool.should be_true
    body["result"]["content"].as_a.first["text"].as_s.should contain "bad_player"
  end

  def test_gateway_error_becomes_tool_error : Nil
    self.disconnect_game!
    body = rpc_json(%({"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"name":"query_units"}}))
    body["result"]["isError"].as_bool.should be_true
    body["result"]["content"].as_a.first["text"].as_s.should contain "disconnected"
  end

  def test_missing_arguments_become_tool_error : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":9,"method":"tools/call","params":{"name":"set_production","arguments":{}}}))
    body["result"]["isError"].as_bool.should be_true
    body["result"]["content"].as_a.first["text"].as_s.should contain "invalid arguments"
  end

  def test_unknown_tool_is_invalid_params : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":10,"method":"tools/call","params":{"name":"launch_nukes"}}))
    body["error"]["code"].as_i.should eq -32602
  end

  def test_unknown_method : Nil
    body = rpc_json(%({"jsonrpc":"2.0","id":11,"method":"resources/list"}))
    body["error"]["code"].as_i.should eq -32601
  end

  def test_parse_error : Nil
    body = rpc_json("{nope")
    body["error"]["code"].as_i.should eq -32700
  end

  def test_batch_rejected : Nil
    body = rpc_json(%([{"jsonrpc":"2.0","id":1,"method":"ping"}]))
    body["error"]["code"].as_i.should eq -32600
  end

  def test_get_and_delete_are_405 : Nil
    self.get("/mcp").status_code.should eq 405
    self.delete("/mcp").status_code.should eq 405
  end
end

struct McpArmyToolsTest < GatewayTestCase
  def test_move_army_maps_args : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"move_army","arguments":{"army_index":0,"x":30,"y":11}}}))
    self.fake.received.should eq ["move_army 0 30 11"]
  end

  def test_auto_explore_and_query_armies : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"auto_explore","arguments":{"army_index":2}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"query_armies"}}))
    self.fake.received.should eq ["auto_explore 2", "query_armies"]
  end
end

struct McpWarDepthToolsTest < GatewayTestCase
  def test_bombard_buy_production_propose_peace_map_to_verbs : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"bombard","arguments":{"army_index":1,"x":19,"y":39}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"buy_production","arguments":{"city_index":2}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"propose_peace","arguments":{"player_id":3}}}))
    self.fake.received.should eq ["bombard 1 19 39", "buy_production 2", "propose_peace 3"]
  end

  def test_war_depth_tools_listed : Nil
    body = JSON.parse(self.post("/mcp", body: %({"jsonrpc":"2.0","id":4,"method":"tools/list"})).body)
    names = body["result"]["tools"].as_a.map(&.["name"].as_s)
    %w[bombard buy_production propose_peace].each { |n| names.should contain n }
  end
end

struct McpEconomyToolsTest < GatewayTestCase
  def test_research_and_terraform_tools_map_to_verbs : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"set_research","arguments":{"advance_id":106}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"set_material_tax","arguments":{"percent":30}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"query_terraform","arguments":{"x":31,"y":9}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"terraform","arguments":{"x":31,"y":9,"improvement_id":29}}}))
    self.fake.received.should eq ["set_research 106", "set_material_tax 30", "query_terraform 31 9", "terraform 31 9 29"]
  end

  def test_query_research_listed : Nil
    body = JSON.parse(self.post("/mcp", body: %({"jsonrpc":"2.0","id":5,"method":"tools/list"})).body)
    names = body["result"]["tools"].as_a.map(&.["name"].as_s)
    %w[query_research set_research set_material_tax query_terraform terraform].each { |n| names.should contain n }
  end
end

struct McpBuildingTest < GatewayTestCase
  def test_building_and_clear_pass_through : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"set_production","arguments":{"city_index":0,"what":"building 30"}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"set_production","arguments":{"city_index":1,"what":"clear"}}}))
    self.fake.received.should eq ["set_production 0 building 30", "set_production 1 clear"]
  end
end

struct McpCombatTest < GatewayTestCase
  def test_war_tools_map_to_verbs : Nil
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"declare_war","arguments":{"player_id":2}}}))
    self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"attack","arguments":{"army_index":0,"x":30,"y":11}}}))
    self.fake.received.should eq ["declare_war 2", "attack 0 30 11"]
  end
end
