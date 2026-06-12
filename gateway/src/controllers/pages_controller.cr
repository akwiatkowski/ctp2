require "./base_controller"
require "../map_renderer"
require "../story_reducer"

module Ctp2Gateway
  # The HTML admin panel — omniscient debugging surface (rides the game's
  # admin-query verbs, no fog of war).
  @[ADI::Register]
  class PagesController < BaseController
    # Athena's DI macros only inspect constructors defined on the concrete
    # class — an inherited one resolves to `.new()`. Delegate explicitly.
    def initialize(client : Ctp2Gateway::GameClient)
      super(client)
    end

    @[ARTA::Get("/")]
    def dashboard : AHTTP::Response
      # Always renders 200 — showing "game unavailable" IS the dashboard's
      # job, unlike the API routes where non-200 signals the failure.
      html(200, "Dashboard", Views::Dashboard.new(build_ledger.to_s), "/")
    end

    # htmx polls this every 5s and swaps it into #ledger.
    @[ARTA::Get("/fragments/dashboard")]
    def dashboard_fragment : AHTTP::Response
      AHTTP::Response.new(build_ledger.to_s, headers: HTTP::Headers{"content-type" => HTML_CT})
    end

    @[ARTA::Get("/players")]
    def players : AHTTP::Response
      result, gw_err, game_err = fetch("query_players")
      if gw_err
        return html(err_status(gw_err), "Error", Views::ErrorPage.new("Gateway error", gw_err.detail), "/players")
      end
      if game_err
        return html(200, "Players", Views::ErrorPage.new("Players", "game says: #{game_err}"), "/players")
      end
      list = result.try(&.["players"].as_a) || [] of JSON::Any
      html(200, "Players", Views::Players.new(list), "/players")
    end

    @[ARTA::Get("/players/{id}/cities", requirements: {"id" => /\d+/})]
    def player_cities(id : Int32) : AHTTP::Response
      result, gw_err, game_err = fetch("query_player_cities #{id}")
      if gw_err
        return html(err_status(gw_err), "Error", Views::ErrorPage.new("Gateway error", gw_err.detail), "/players")
      end
      if game_err
        status = game_err == "bad_player" ? 404 : 200
        return html(status, "Cities", Views::ErrorPage.new("Player #{id}", "game says: #{game_err}"), "/players")
      end
      cities = result.try(&.["cities"].as_a) || [] of JSON::Any

      # Second roundtrip for the leader name — admin pages favour clarity
      # over saving a local-socket call. Fall back silently if it fails.
      name = "Player #{id}"
      presult, _, _ = fetch("query_players")
      presult.try(&.["players"].as_a.each do |p|
        name = p["name"].as_s if p["id"].as_i == id && !p["name"].as_s.empty?
      end)

      html(200, "Cities of #{name}", Views::PlayerCities.new(id, name, cities), "/players")
    end

    # The illuminated chart: an HTML tile map for humans, with the model's
    # ASCII view (render_map) kept in a collapsible section for parity.
    @[ARTA::Get("/map")]
    def map : AHTTP::Response
      renderer = MapRenderer.new(@client)
      spots = renderer.settle_spots(5)
      chart = renderer.chart(spots: spots)
      ascii = renderer.render(radius: 30)
      body = Views::MapPage.new(chart, ascii, renderer.error, spots)
      html(200, "Map", body, "/map")
    end

    # The chronicle: the engine's Action Log narrated as a "story of the
    # civilization", one section per round (StoryReducer over log_get). The
    # page frames a live-polled body; htmx swaps fresh chapters every 10s.
    @[ARTA::Get("/story")]
    def story : AHTTP::Response
      body = Views::StoryBody.new(StoryReducer.new(@client).story).to_s
      html(200, "Chronicle", Views::Story.new(body), "/story")
    end

    # htmx polls this every 10s and swaps it into #chronicle.
    @[ARTA::Get("/fragments/story")]
    def story_fragment : AHTTP::Response
      body = Views::StoryBody.new(StoryReducer.new(@client).story).to_s
      AHTTP::Response.new(body, headers: HTTP::Headers{"content-type" => HTML_CT})
    end

    # The MCP tool catalog, rendered from the live registry (Mcp::TOOLS) —
    # always in sync with what tools/list serves to models.
    @[ARTA::Get("/tools")]
    def tools : AHTTP::Response
      html(200, "Tools", Views::Tools.new(Mcp::TOOLS), "/tools")
    end

    @[ARTA::Get("/debug")]
    def debug : AHTTP::Response
      body = Views::DebugPage.new(
        socket: @client.socket_path,
        connected: @client.connected?,
        last_error: @client.last_error,
        spawn_line: spawn_line,
        session_id: Ctp2Gateway::Config.session.id,
        exchanges: @client.recent_exchanges,
      )
      html(200, "Debug", body, "/debug")
    end

    # Intentional raiser — exercises the HTML error rendering end-to-end
    # (the JSON twin lives in ApiController).
    @[ARTA::Get("/debug/boom")]
    def boom : Nil
      raise "intentional test exception (/debug/boom)"
    end
  end
end
