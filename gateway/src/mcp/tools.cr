require "json"
require "../game_client"
require "../map_renderer"

module Ctp2Gateway::Mcp
  # One MCP tool: name + description (written FOR the model — they are the
  # only documentation the LLM sees), a JSON Schema for the arguments, and
  # the handler mapping arguments onto game verbs.
  record Tool,
    name : String,
    description : String,
    schema : String,
    run : Proc(Ctp2Gateway::GameClient, JSON::Any, Ctp2Gateway::GameClient::Result)

  NO_ARGS = %({"type":"object","properties":{},"additionalProperties":false})

  private def self.simple(client, verb) : Ctp2Gateway::GameClient::Result
    client.command(verb)
  end

  # Hard cap per end_turn call: rounds cost ~0.5s each and the game-socket
  # timeout is 30s — the model is told to call repeatedly for longer spans.
  MAX_TURNS_PER_CALL = 20

  TOOLS = [
    Tool.new("start_game",
      "Create and start a new CTP2 game (generates the world; takes a few seconds). " \
      "You play the single human player; the other players are AI. " \
      "Begin here unless a game is already running (check with query_turn).",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        # Frontend pairing: new_game prepares, start_game initializes.
        r = c.command("new_game")
        r.is_a?(Ctp2Gateway::GameClient::Err) ? r : c.command("start_game")
      }),

    Tool.new("end_turn",
      "Advance time by N full rounds (every player, including AI, takes a turn). " \
      "Production, growth and AI moves happen here. Max #{MAX_TURNS_PER_CALL} rounds " \
      "per call — call repeatedly for longer spans. Check results with query_turn / query_cities.",
      %({"type":"object","properties":{"turns":{"type":"integer","minimum":1,"maximum":#{MAX_TURNS_PER_CALL},"description":"rounds to advance (default 1)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        turns = (a["turns"]?.try(&.as_i?) || 1).clamp(1, MAX_TURNS_PER_CALL)
        c.command("end_turn #{turns}")
      }),

    Tool.new("build_city",
      "Found a city with your first settler-capable army AT ITS CURRENT TILE. " \
      "Errors: no_settler_found (build one via set_production), tile_occupied " \
      "(a city already stands here — move_army the settler away first), " \
      "settle_rejected (too close to another city — keep distance >= 3 — or bad terrain).",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "build_city") }),

    Tool.new("query_armies",
      "Your armies — index (use with move_army/auto_explore), position, movement " \
      "points left this turn, can_settle, and member units.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_armies") }),

    Tool.new("move_army",
      "Move one of your armies toward (x,y) via pathfinding. no_path means the " \
      "destination is UNEXPLORED, impassable, or unreachable — you can only plot " \
      "through explored tiles (see query_map), so walk in short hops and let vision " \
      "expand. Most units move ~1 tile per turn: check `arrived` in the result, and " \
      "if false, end_turn and re-issue. Typical expansion: march a settler to " \
      "distance >= 3 from any city, then build_city.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0,"description":"from query_armies"},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("move_army #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("auto_explore",
      "Put one of your armies on auto-explore: it keeps picking new targets each " \
      "turn, revealing the map without manual driving. Good for a cheap scout.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("auto_explore #{a["army_index"].as_i}")
      }),

    Tool.new("set_production",
      "Set what one of YOUR cities builds. `what` is \"settler\", \"cheapest_military\", " \
      "a numeric unit type id, \"building <id>\" (improvements from query_city's " \
      "buildable_buildings — a GRANARY is the growth lever on poor terrain), " \
      "\"wonder <id>\" (from query_city's buildable_wonders — the biggest score lever, " \
      "with empire effects like Great Library), or \"clear\" to STOP producing. " \
      "Production loops forever otherwise — idle armies drain score via upkeep, and each " \
      "settler consumes a population point.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0,"description":"index into your city list (see query_cities)"},"what":{"type":"string","description":"settler | cheapest_military | unit type id | building <id> | wonder <id> | clear"}},"required":["city_index","what"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_production #{a["city_index"].as_i} #{a["what"].as_s}")
      }),

    Tool.new("save_game",
      "Save the game to a file path (JSON format). Use as a checkpoint before risky experiments.",
      %({"type":"object","properties":{"path":{"type":"string","description":"absolute file path, e.g. /tmp/checkpoint.json"}},"required":["path"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("save_game #{a["path"].as_s}") }),

    Tool.new("load_game",
      "Load a previously saved game. Restores the full world state — combine with " \
      "save_game for checkpoint/rollback debugging.",
      %({"type":"object","properties":{"path":{"type":"string"}},"required":["path"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("load_game #{a["path"].as_s}") }),

    Tool.new("log_get",
      "The Action Log: a ledger of every meaningful action/outcome the engine fired " \
      "this game — orders (move/settle/bombard/...), world-changing beats (city founded/" \
      "captured, unit built, advance granted, pop grown, battle, wonder), and real " \
      "diplomacy (contact, proposal, accept/reject) — regardless of source (your commands, " \
      "the AI's turns, slic). Each entry is {turn, player, event, args}; args carry typed " \
      "ids ({kind:city,id}, {kind:location,x,y}, ...). The ledger rides inside the JSON " \
      "save, so it spans save_game/load_game — a forked checkpoint keeps its own history. " \
      "Returns {action_log:[...], count}. Use it to review what happened over a span of " \
      "turns, or to build a narrative of the game.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "log_get") }),

    Tool.new("query_turn",
      "The clock: round (full rounds completed) and calendar `year`, plus the scored " \
      "deadline — end_of_game_year (default 2300 AD; highest score wins then), " \
      "end_of_game_warning_year, and past_deadline (true => the game is in UNSCORED " \
      "overtime; headless keeps running past 2300 without setting a victory flag). " \
      "Cheap — use freely.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_turn") }),

    Tool.new("query_players",
      "ADMIN (omniscient): every player — id, leader name, civ, country, human/AI, " \
      "alive/dead, gold, city/unit/army counts, government, score.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_players") }),

    Tool.new("query_player",
      "ADMIN (omniscient): one player slot in the same shape as query_players entries.",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_player #{a["player_id"].as_i}") }),

    Tool.new("query_player_cities",
      "ADMIN (omniscient): ALL cities of one player, fog disregarded, including " \
      "per-turn yields (food, production, gold, science, happiness).",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_player_cities #{a["player_id"].as_i}") }),

    Tool.new("query_cities",
      "PLAYER VIEW (fog-of-war filtered): cities you can see — always your own, " \
      "enemy cities only when their tile is visible.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_cities") }),

    Tool.new("query_city",
      "PLAYER VIEW: detail for one of YOUR cities by index, including the buildable-unit " \
      "list (what set_production accepts) and per-turn yields. Use the growth-diagnostic " \
      "fields to explain why a city is/isn't growing: food.{gross,consumed,net,required," \
      "max_from_terrain} (net food surplus is the growth driver; max_from_terrain is the " \
      "untapped food ceiling), growth.{growth_rate,max_pop,at_pop_cap,starvation_turns} " \
      "(growth_rate 0 with at_pop_cap=false means too little food SURPLUS, not a cap), " \
      "gold_upkeep (wages + building maintenance this city pays per turn — NOT military " \
      "unit upkeep), buildings_built (improvements the city already has, by name), and " \
      "buildable_wonders (feed set_production \"wonder <id>\" — the biggest score lever), " \
      "specialists (worker/scientist/farmer/... counts — feed set_specialist), and governor " \
      "{enabled, build_list_sequence, build_list_name} (the mayor — feed set_governor).",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0}},"required":["city_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command("query_city #{a["city_index"].as_i}") }),

    Tool.new("query_units",
      "PLAYER VIEW (fog-of-war filtered): all units you can see — owner, type, name, " \
      "position, hp.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_units") }),

    Tool.new("query_map",
      "PLAYER VIEW: every tile you have explored — terrain, currently-visible flag, " \
      "city markers. Output can be LARGE on a well-explored map; prefer targeted " \
      "queries when you only need cities or units.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_map") }),

    Tool.new("query_research",
      "The science affordance set: what you're researching, what you could research " \
      "and each option's cost. RESEARCH IS THE MAIN SCORE ENGINE — advances unlock " \
      "units, buildings and terraforming. Keep something queued at all times.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_research") }),

    Tool.new("set_research",
      "Switch research to an advance id from query_research's available list. " \
      "Errors: already_known, prerequisites_missing.",
      %({"type":"object","properties":{"advance_id":{"type":"integer","minimum":0}},"required":["advance_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_research #{a["advance_id"].as_i}")
      }),

    Tool.new("set_material_tax",
      "Divert a percentage of city production into the Public Works pool that pays " \
      "for terraforming. Without this your PW stays 0 and terraform is unusable. " \
      "20-40% is a reasonable working rate.",
      %({"type":"object","properties":{"percent":{"type":"integer","minimum":0,"maximum":100}},"required":["percent"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_material_tax #{a["percent"].as_i}")
      }),

    Tool.new("disband_unit",
      "Disband one of YOUR armies (army_index from query_armies). Frees the player-level " \
      "GOLD upkeep that units cost above the free-support threshold (more impactful under " \
      "later governments / large armies) and removes dead-weight stacks. Refuses your very " \
      "last army. (Note: it does NOT change a city's gold_upkeep, which is wages+buildings.)",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("disband_unit #{a["army_index"].as_i}")
      }),

    Tool.new("set_government",
      "Switch government type (id from the government DB). The master multiplier on " \
      "science, gold, happiness, production and max city size — the highest-leverage " \
      "economic decision. Self-gated on the enabling advance; switching FROM an " \
      "established government routes through a short anarchy first (active_government " \
      "may lag requested_government for a turn). Errors: bad_government, " \
      "already_that_government, advance_missing, rejected.",
      %({"type":"object","properties":{"government_type":{"type":"integer","minimum":0}},"required":["government_type"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_government #{a["government_type"].as_i}")
      }),

    Tool.new("set_science_rate",
      "Set the science share of commerce as a percent 0-100 (the rest becomes gold). " \
      "A direct score dial — higher = faster advances. The engine clamps to the " \
      "government's max science rate, so the applied science_rate (see query_player's " \
      "economy block) may be lower than requested. Errors: out_of_range, bad_args.",
      %({"type":"object","properties":{"percent":{"type":"integer","minimum":0,"maximum":100}},"required":["percent"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_science_rate #{a["percent"].as_i}")
      }),

    Tool.new("set_rates",
      "Set the three social sliders: workday (production), wages (gold), rations (food " \
      "per person). Each level trades raw output for happiness around the government's " \
      "`expectation` (see query_player.economy). LOWER rations frees food for GROWTH; " \
      "higher workday adds production. Pass -1 to leave a slider unchanged.",
      %({"type":"object","properties":{"workday":{"type":"integer","minimum":-1},"wages":{"type":"integer","minimum":-1},"rations":{"type":"integer","minimum":-1}},"required":["workday","wages","rations"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_rates #{a["workday"].as_i} #{a["wages"].as_i} #{a["rations"].as_i}")
      }),

    Tool.new("set_specialist",
      "Reassign citizens in one of YOUR cities between tile-work and a specialist role. " \
      "pop_type: 1=scientist 2=entertainer 3=farmer 4=laborer 5=merchant. delta>0 turns " \
      "that many WORKERS into specialists (needs enough free workers); delta<0 reverts. " \
      "Specialists give a fixed per-head yield regardless of terrain — the lever for a " \
      "city on poor tiles (scientists=flat science, farmers=flat food). See query_city's " \
      "`specialists` block. Errors: bad_pop_type, not_enough_workers, not_enough_specialists.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0},"pop_type":{"type":"integer","minimum":1,"maximum":5},"delta":{"type":"integer"}},"required":["city_index","pop_type","delta"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("set_specialist #{a["city_index"].as_i} #{a["pop_type"].as_i} #{a["delta"].as_i}")
      }),

    Tool.new("set_governor",
      "Toggle a city's GOVERNOR (mayor): when enabled the engine auto-manages its build " \
      "queue per a named optimization profile. enabled: true/false. Optional " \
      "build_list_sequence (index from query_governor_profiles) switches the profile " \
      "(production/growth/science/gold/...). Frees you from micromanaging every city. " \
      "See query_city's `governor` block.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0},"enabled":{"type":"boolean"},"build_list_sequence":{"type":"integer","minimum":0,"description":"optional profile index"}},"required":["city_index","enabled"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        on = a["enabled"].as_bool ? 1 : 0
        seq = a["build_list_sequence"]?
        cmd = seq ? "set_governor #{a["city_index"].as_i} #{on} #{seq.as_i}" \
                  : "set_governor #{a["city_index"].as_i} #{on}"
        c.command(cmd)
      }),

    Tool.new("query_governor_profiles",
      "List the named build-list sequences a city governor can follow (index + name): " \
      "PRODUCTION / GROWTH / SCIENCE / GOLD / OFFENSE / DEFENSE etc. Feed the index to " \
      "set_governor's build_list_sequence to pick what a governed city optimizes for.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_governor_profiles") }),

    Tool.new("establish_trade_route",
      "Create a resource trade route from one of YOUR cities (source_city index) to " \
      "another visible city (dest_city index), generating trade value/gold. If `good` " \
      "is omitted, the first good the source city can collect is used. Errors: " \
      "bad_source_city, bad_dest_city, same_city, no_good_available, route_rejected.",
      %({"type":"object","properties":{"source_city":{"type":"integer","minimum":0},"dest_city":{"type":"integer","minimum":0},"good":{"type":"integer","minimum":0,"description":"optional resource/good id; auto-picked if omitted"}},"required":["source_city","dest_city"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        g = a["good"]?
        cmd = g ? "establish_trade_route #{a["source_city"].as_i} #{a["dest_city"].as_i} #{g.as_i}" \
                : "establish_trade_route #{a["source_city"].as_i} #{a["dest_city"].as_i}"
        c.command(cmd)
      }),

    Tool.new("query_terraform",
      "Buildable tile improvements for ONE tile (feed `terraform`): each option has " \
      "improvement_id, name, `class` (terraform|farm|mine|road|structure|wonder), " \
      "is_terraform, Public Works cost/turns, affordable, and (terraform only) " \
      "to_terrain/to_terrain_name. Lists BOTH terrain transforms AND ordinary " \
      "infrastructure (Farms add food, Mines add production, Roads add trade/movement). " \
      "Only works INSIDE your borders (check tile_owner); each option is gated by " \
      "advances — draining swamp needs Industrial Revolution, levelling hills needs " \
      "Explosives, Farms need Agriculture. If cities can't grow (low food), terraform " \
      "swamp/hill -> Grassland then build a Farm: that is THE food fix.",
      %({"type":"object","properties":{"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("query_terraform #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("terraform",
      "Spend Public Works to build ANY tile improvement — terrain transforms AND " \
      "infrastructure (Farm/Mine/Road), despite the name. Pass improvement_id from " \
      "query_terraform's options. Completes after the option's `turns` — keep ending " \
      "turns. Needs material_tax > 0 to keep the PW pool funded.",
      %({"type":"object","properties":{"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0},"improvement_id":{"type":"integer","minimum":0}},"required":["x","y","improvement_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("terraform #{a["x"].as_i} #{a["y"].as_i} #{a["improvement_id"].as_i}")
      }),

    Tool.new("render_map",
      "YOUR EYES: an ASCII map of the explored world — terrain glyphs, coordinate " \
      "rulers, cities as letters, @ = your armies, ! = foreign units, · = unexplored. " \
      "Read it to plan movement and expansion. Defaults to a viewport around your " \
      "first city; pass center_x/center_y/radius to look elsewhere.",
      %({"type":"object","properties":{"center_x":{"type":"integer","minimum":0},"center_y":{"type":"integer","minimum":0},"radius":{"type":"integer","minimum":4,"maximum":30,"description":"half-width of the viewport (default 14)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        r = Ctp2Gateway::MapRenderer.new(c)
        text = r.render(
          a["center_x"]?.try(&.as_i?),
          a["center_y"]?.try(&.as_i?),
          a["radius"]?.try(&.as_i?) || 14)
        if text
          Ctp2Gateway::GameClient::Ok.new(JSON::Any.new(text))
        else
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({status: "error", detail: r.error || "render failed"}.to_json))
        end
      }),

    Tool.new("suggest_settle_spots",
      "Where to found the next city: explored, passable land tiles at distance >= 3 " \
      "from every known city, scored FOOD-FIRST (3*food + shields + gold) over the " \
      "tile and its explored neighbours INCLUDING water (kelp beds and beaches feed " \
      "coastal cities) — highest score first. Score is population and population is " \
      "food. March a settler there with move_army, then build_city.",
      %({"type":"object","properties":{"max":{"type":"integer","minimum":1,"maximum":10,"description":"how many candidates (default 5)"}},"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        r = Ctp2Gateway::MapRenderer.new(c)
        spots = r.settle_spots(a["max"]?.try(&.as_i?) || 5)
        if spots
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({
            status: "ok",
            spots:  spots.map { |s| {x: s.x, y: s.y, score: s.score, distance_to_nearest_city: s.dist, terrain: s.terrain} },
            note:   spots.empty? ? "no qualifying tiles explored yet — explore further (auto_explore) before settling" : nil,
          }.to_json))
        else
          Ctp2Gateway::GameClient::Ok.new(JSON.parse({status: "error", detail: r.error || "advisor failed"}.to_json))
        end
      }),

    Tool.new("unload",
      "Amphibious landing: order a transport army (boats carrying cargo — see " \
      "query_armies' cargo field) to disembark everyone onto an ADJACENT land tile. " \
      "To BOARD, move a land army onto your transport's water tile with move_army " \
      "(allowed when the transport has free cargo_capacity).",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("unload #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("group_army",
      "Merge every unit on the army's tile into ONE stack (up to 12 fight as one " \
      "army). ESSENTIAL before combat: single-unit armies attacking a stack die one " \
      "by one — build units in a city, then group the army standing there. " \
      "ungroup via raw_cmd 'ungroup_army <idx>'.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("group_army #{a["army_index"].as_i}")
      }),

    Tool.new("fortify",
      "Entrench an army in place for a defensive bonus. Garrisons that merely stand " \
      "in a city take full damage — fortify them. Moving the army breaks the " \
      "entrenchment.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("fortify #{a["army_index"].as_i}")
      }),

    Tool.new("declare_war",
      "Formally declare war on a player (id from query_players). Requires CONTACT — " \
      "you must have met them (their units/cities seen by yours). Check the contact/" \
      "at_war fields in query_players. War enables the attack tool; conquest is worth " \
      "more score than anything else on a bad start.",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("declare_war #{a["player_id"].as_i}")
      }),

    Tool.new("attack",
      "Order an army onto an ADJACENT enemy-occupied tile — the move resolves combat; " \
      "if the defenders die and a city stands there, you capture it. Requires war " \
      "(declare_war). March with move_army first; attack only closes the last tile. " \
      "Check the result: army_survived, captured_tile, defenders_left.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("attack #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("bombard",
      "Ranged strike on an ADJACENT enemy-occupied tile. Unlike attack, your army " \
      "STAYS PUT and takes no damage — soften a defending stack (or a city's " \
      "garrison) with bombard, then attack with the survivors' odds improved. " \
      "Needs war and a unit with bombard capability (see query_city buildable " \
      "stats); each unit bombards once per turn. Result reports damage_dealt and " \
      "defenders_left — zero damage means your units can't hurt that target.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","x","y"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("bombard #{a["army_index"].as_i} #{a["x"].as_i} #{a["y"].as_i}")
      }),

    Tool.new("buy_production",
      "Rush-buy the city's CURRENT build item with gold — it completes next turn. " \
      "THE captured-city lever: a freshly conquered city riots (query_city " \
      "`rioting`) and produces nothing, so buy a happiness building immediately " \
      "instead of waiting 30 rounds. Also for emergency military. Errors: " \
      "not_enough_gold (reports cost), nothing_being_built, already_bought.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0}},"required":["city_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("buy_production #{a["city_index"].as_i}")
      }),

    Tool.new("propose_peace",
      "Send a formal PEACE TREATY proposal to a player you are at war with. The AI " \
      "evaluates it for real (its regard, war progress, relative strength) and may " \
      "REJECT — check `accepted`/`at_war` in the result. Use after taking what you " \
      "came for: peace stops counterattacks while you digest conquered cities. " \
      "Rejected? Hurt them more or wait some rounds and retry.",
      %({"type":"object","properties":{"player_id":{"type":"integer","minimum":0}},"required":["player_id"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("propose_peace #{a["player_id"].as_i}")
      }),

    Tool.new("query_unit_orders",
      "List the SPECIAL orders an army can perform (the right-click menu): each has " \
      "order_index, name, target (enemy_city/own_city/enemy_army/terrain_improvement/" \
      "none/...), and gold_cost. Covers spy/diplomat ops (investigate city, steal " \
      "technology, incite revolution, establish embassy), pillage, expel, infect, " \
      "convert, etc. Feed order_index to do_unit_order.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("query_unit_orders #{a["army_index"].as_i}")
      }),

    Tool.new("do_unit_order",
      "Execute a special unit order (order_index from query_unit_orders). With x,y it " \
      "acts on that tile — the army must be ON it or ADJACENT, so move there first " \
      "(like bombard). Without x,y it acts in place. Engine-validated: an illegal order " \
      "is reported (illegal_for_this_unit / lacks_gold / needs_target / invalid_target / " \
      "no_moves_left), never executed blindly. This is THE spy/diplomat/pillage surface.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0},"order_index":{"type":"integer","minimum":0},"x":{"type":"integer","minimum":0},"y":{"type":"integer","minimum":0}},"required":["army_index","order_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        xa = a["x"]?; ya = a["y"]?
        cmd = (xa && ya) ? "do_unit_order #{a["army_index"].as_i} #{a["order_index"].as_i} #{xa.as_i} #{ya.as_i}" \
                         : "do_unit_order #{a["army_index"].as_i} #{a["order_index"].as_i}"
        c.command(cmd)
      }),

    Tool.new("upgrade_unit",
      "Modernise every upgradable unit in an army to its current-tech equivalent " \
      "(spends gold). Reports units_upgradable + upgraded. Error: nothing_to_upgrade. " \
      "The fix for a fleet of obsolete units you'd otherwise disband.",
      %({"type":"object","properties":{"army_index":{"type":"integer","minimum":0}},"required":["army_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("upgrade_unit #{a["army_index"].as_i}")
      }),

    Tool.new("propose",
      "Send a diplomatic proposal to another contacted player and get the AI verdict. " \
      "proposal_type is a PROPOSAL_TYPE id: 17=OFFER_GIVE_ADVANCE, 18=REQUEST_GIVE_ADVANCE, " \
      "19=OFFER_GIVE_GOLD, 20=REQUEST_GIVE_GOLD, 23=OFFER_MAP, 24=REQUEST_MAP, " \
      "32=TREATY_CEASEFIRE, 33=TREATY_PEACE, 34=TRADE_PACT, 35=RESEARCH_PACT, " \
      "38=TREATY_ALLIANCE. `arg` is the advance id (for GIVE/REQUEST_ADVANCE) or gold " \
      "amount (for GIVE/REQUEST_GOLD). The richest catch-up lever: trade for tech. " \
      "Errors: no_contact, bad_proposal_type, bad_advance_arg, bad_gold_arg.",
      %({"type":"object","properties":{"target_player":{"type":"integer","minimum":0},"proposal_type":{"type":"integer","minimum":1},"arg":{"type":"integer","minimum":0,"description":"advance id or gold amount, when the proposal needs one"}},"required":["target_player","proposal_type"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        ar = a["arg"]?
        cmd = ar ? "propose #{a["target_player"].as_i} #{a["proposal_type"].as_i} #{ar.as_i}" \
                 : "propose #{a["target_player"].as_i} #{a["proposal_type"].as_i}"
        c.command(cmd)
      }),

    Tool.new("sell_building",
      "Sell a built improvement in one of YOUR cities for gold (once per city per turn). " \
      "building_type from query_city.buildings_built. Errors: building_not_present, " \
      "already_sold_this_turn, bad_building.",
      %({"type":"object","properties":{"city_index":{"type":"integer","minimum":0},"building_type":{"type":"integer","minimum":0}},"required":["city_index","building_type"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("sell_building #{a["city_index"].as_i} #{a["building_type"].as_i}")
      }),

    Tool.new("query_trade_routes",
      "Your outgoing trade routes, per source city, each with source_city + route_index " \
      "(feed to cancel_trade_route) and the destination city name.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result { simple(c, "query_trade_routes") }),

    Tool.new("cancel_trade_route",
      "Cancel one of your outgoing trade routes (source_city + route_index from " \
      "query_trade_routes). Errors: bad_city_index, bad_route_index.",
      %({"type":"object","properties":{"source_city":{"type":"integer","minimum":0},"route_index":{"type":"integer","minimum":0}},"required":["source_city","route_index"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        c.command("cancel_trade_route #{a["source_city"].as_i} #{a["route_index"].as_i}")
      }),

    Tool.new("raw_cmd",
      "Escape hatch: send a raw verb line to the game's command socket (one line, " \
      "e.g. \"query_city 0\"). See GET /api on the gateway for the verb inventory.",
      %({"type":"object","properties":{"cmd":{"type":"string"}},"required":["cmd"],"additionalProperties":false}),
      ->(c : Ctp2Gateway::GameClient, a : JSON::Any) : Ctp2Gateway::GameClient::Result { c.command(a["cmd"].as_s) }),

    Tool.new("gateway_health",
      "Gateway/observability state: game-socket connection, spawned game process, " \
      "and the recent command journal. Use when game tools fail unexpectedly.",
      NO_ARGS,
      ->(c : Ctp2Gateway::GameClient, _a : JSON::Any) : Ctp2Gateway::GameClient::Result {
        Ctp2Gateway::GameClient::Ok.new(JSON.parse({
          socket:     c.socket_path,
          connected:  c.connected?,
          last_error: c.last_error,
          spawn:      Ctp2Gateway::Config.process.try(&.status_json),
          recent_exchanges: c.recent_exchanges.first(10).map do |e|
            {at: e.at.to_rfc3339(fraction_digits: 3), cmd: e.cmd, ok: e.ok, detail: e.detail}
          end,
        }.to_json))
      }),
  ]

  def self.find(name : String) : Tool?
    TOOLS.find { |t| t.name == name }
  end
end
