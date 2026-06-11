require "./spec_helper"

# A tiny synthetic world for the renderer: Rome at (5,5), a settler army at
# (6,5), a foreign unit at (3,4), a mountain, a water tile, and a fertile
# plains cluster at distance 3-4 (settle candidates).
private def world_responses
  tiles = [] of String
  (3..7).each do |x|
    (3..7).each do |y|
      terrain = if {x, y} == {4, 4}
                  8 # mountain
                elsif {x, y} == {3, 5}
                  10 # shallow water
                else
                  4 # grassland
                end
      # (7,7) is explored-but-remembered: the chart must dim it.
      visible = {x, y} != {7, 7}
      tiles << %({"x":#{x},"y":#{y},"terrain":#{terrain},"visible":#{visible}})
    end
  end
  [{8, 4}, {8, 5}, {9, 4}, {9, 5}].each do |(x, y)|
    tiles << %({"x":#{x},"y":#{y},"terrain":1,"visible":true})
  end

  {
    "query_map" => %({"status":"ok","cmd":"query_map","result":{"visible_player":1,"width":20,"height":20,"explored":#{tiles.size},"visible":#{tiles.size},"tiles":[#{tiles.join(",")}]}}),
    "query_terrains" => %({"status":"ok","cmd":"query_terrains","result":{"terrains":[) +
                       %({"id":1,"name":"Plains","internal":"TERRAIN_PLAINS","land":true,"water":false,"mountain":false,"food":10,"shields":10,"gold":5,"movement":100},) +
                       %({"id":4,"name":"Grassland","internal":"TERRAIN_GRASSLAND","land":true,"water":false,"mountain":false,"food":15,"shields":5,"gold":5,"movement":100},) +
                       %({"id":8,"name":"Mountain","internal":"TERRAIN_MOUNTAIN","land":false,"water":false,"mountain":true,"food":0,"shields":15,"gold":10,"movement":300},) +
                       %({"id":10,"name":"Shallow Water","internal":"TERRAIN_WATER_SHALLOW","land":false,"water":true,"mountain":false,"food":10,"shields":10,"gold":5,"movement":100}]}}),
    "query_cities" => %({"status":"ok","cmd":"query_cities","result":{"visible_player":1,"cities":[{"owner":1,"index":0,"name":"Rome","pos":{"x":5,"y":5},"population":2,"building":null}]}}),
    "query_armies" => %({"status":"ok","cmd":"query_armies","result":{"armies":[{"index":0,"pos":{"x":6,"y":5},"moves_left":1.0,"can_settle":true,"units":[{"type":54,"name":"Settler","hp":10.0}]}]}}),
    "query_units" => %({"status":"ok","cmd":"query_units","result":{"visible_player":1,"units":[) +
                    %({"owner":1,"type":54,"name":"Settler","pos":{"x":6,"y":5},"hp":10.0,"is_city":false},) +
                    %({"owner":2,"type":30,"name":"Warrior","pos":{"x":3,"y":4},"hp":10.0,"is_city":false}]}}),
  }
end

struct MapToolsTest < GatewayTestCase
  private def world! : Nil
    @fake.close
    @fake = FakeGame.new(world_responses)
    Ctp2Gateway::Config.reset_client!(@fake.path)
  end

  def test_render_map_draws_the_world : Nil
    world!
    body = JSON.parse(self.post("/mcp", body: %({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"render_map"}})).body)
    body["result"]["isError"].as_bool.should be_false
    map = body["result"]["content"].as_a.first["text"].as_s
    map.should contain "you are player 1"
    map.should contain "R"                       # Rome on the grid
    map.should contain "@"                       # own army
    map.should contain "!"                       # foreign unit
    map.should contain "^"                       # mountain
    map.should contain "~"                       # water
    map.should contain "·"                       # fog
    map.should contain "R=Rome(5,5)"             # city legend
    map.should contain "#0@(6,5) settler"        # army roster
    map.should contain ",=Grassland"             # terrain legend from live data
  end

  def test_settle_advisor_scores_and_distances : Nil
    world!
    body = JSON.parse(self.post("/mcp", body: %({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"suggest_settle_spots","arguments":{"max":3}}})).body)
    body["result"]["isError"].as_bool.should be_false
    spots = JSON.parse(body["result"]["content"].as_a.first["text"].as_s)["spots"].as_a
    spots.size.should eq 3
    best = spots.first
    # Food-weighted (3f+s+g): plains = 45, grassland = 55.
    # Best cluster tile: self + 3 plains (180) + 3 grassland (165) = 345.
    best["score"].as_i.should eq 345
    best["distance_to_nearest_city"].as_i.should be >= 3
    best["terrain"].as_s.should eq "Plains"
    # Every candidate respects the minimum city distance.
    spots.each { |s| s["distance_to_nearest_city"].as_i.should be >= 3 }
  end

  def test_map_page_renders : Nil
    world!
    resp = self.get("/map")
    resp.status_code.should eq 200
    resp.body.should contain "charted world"
    resp.body.should contain "Settle advisor"
    resp.body.should contain "(8, 5)" # a spot row... position formatting
    # ASCII parity view stays available in the collapsible plain chart.
    resp.body.should contain "Plain chart"
    resp.body.should contain "R=Rome(5,5)"
  end

  def test_map_page_draws_the_illuminated_chart : Nil
    world!
    body = self.get("/map").body
    body.should contain %(id="atlas")              # the tile grid
    body.should contain "t-grass"                  # terrain pigment classes
    body.should contain "t-mountain"
    body.should contain "t-shallow"
    body.should contain "tl--dim"                  # (7,7) remembered tile
    body.should contain "tl--fog"                  # unexplored vellum
    body.should contain %(data-t="Grassland · f15 s5 g5")     # tooltip ledger
    body.should contain %(data-city="Rome")        # city seal with name
    body.should contain %(class="pin pin--city">R<) # own city = gold seal
    body.should contain "⚑"                        # own settler pin
    body.should contain %(class="pin pin--foe">▲<) # foreign unit pin
    body.should contain %(data-rank="1")           # top settle site ring
    body.should contain "/assets/atlas.js"         # tooltip script wired in
  end

  def test_render_map_unstarted_game_is_tool_error : Nil
    # Default canned responses: query_map yields a default ok WITHOUT result,
    # which the renderer reports as an error rather than crashing.
    body = JSON.parse(self.post("/mcp", body: %({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"render_map"}})).body)
    body["result"]["isError"].as_bool.should be_true
  end
end
