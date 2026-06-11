require "ecr"
require "html"
require "json"
require "./mcp/tools"
require "./command_descriptions"
require "./map_renderer"

module Ctp2Gateway::Views
  # ECR has NO auto-escaping: every piece of GAME data interpolated in a
  # template goes through HTML.escape at the point of use. The layout's
  # <%= @content %> is raw on purpose — it embeds gateway-generated page
  # bodies, never game data directly.
  struct Layout
    # @path drives the nav's aria-current highlight.
    def initialize(@title : String, @content : String, @path : String)
    end

    ECR.def_to_s "src/views/layout.ecr"
  end

  # The live-polled part of the dashboard (status strip + stat ledger).
  # Rendered standalone for GET /fragments/dashboard (htmx swaps it in
  # every 5s) and embedded pre-rendered into the full Dashboard page.
  struct DashboardLedger
    def initialize(@socket : String, @connected : Bool, @spawn_line : String?,
                   @game_error : String?, @player_count : Int32,
                   @alive_count : Int32, @city_count : Int32,
                   @leader_name : String?, @leader_country : String?,
                   @leader_score : Int32, @round : Int32?, @year : Int32?)
    end

    # CTP2 years are astronomical: negative = BC.
    private def year_label(year : Int32) : String
      year < 0 ? "#{-year} BC" : "#{year} AD"
    end

    ECR.def_to_s "src/views/dashboard_ledger.ecr"
  end

  struct MapPage
    def initialize(@chart : Ctp2Gateway::MapRenderer::Chart?,
                   @ascii : String?, @error : String?,
                   @spots : Array(Ctp2Gateway::MapRenderer::Spot)?)
    end

    # Human-readable names for the terrain swatch legend.
    TERRAIN_LABELS = {
      "t-deep" => "deep water", "t-shallow" => "coastal water",
      "t-kelp" => "kelp/reef", "t-grass" => "grassland",
      "t-plains" => "plains", "t-forest" => "forest",
      "t-jungle" => "jungle", "t-swamp" => "swamp",
      "t-desert" => "desert", "t-tundra" => "tundra",
      "t-glacier" => "glacier", "t-hill" => "hills",
      "t-mountain" => "mountains", "t-unknown" => "other",
    }

    ECR.def_to_s "src/views/map.ecr"
  end

  struct Tools
    def initialize(@tools : Array(Ctp2Gateway::Mcp::Tool))
    end

    ECR.def_to_s "src/views/tools.ecr"
  end

  struct DebugPage
    def initialize(@socket : String, @connected : Bool, @last_error : String?,
                   @spawn_line : String?, @session_id : String,
                   @exchanges : Array(Ctp2Gateway::GameClient::Exchange))
    end

    # Human-readable label for a game verb; falls back to a neutral phrase.
    private def describe(verb : String) : String
      Ctp2Gateway::CommandDescriptions.for(verb) || "game command"
    end

    ECR.def_to_s "src/views/debug.ecr"
  end

  struct DebugError
    def initialize(@klass : String, @message : String, @backtrace : Array(String),
                   @request_line : String, @connected : Bool, @socket : String,
                   @exchanges : Array(Ctp2Gateway::GameClient::Exchange))
    end

    ECR.def_to_s "src/views/error_debug.ecr"
  end

  struct Dashboard
    def initialize(@ledger : String)
    end

    ECR.def_to_s "src/views/dashboard.ecr"
  end

  struct Players
    def initialize(@players : Array(JSON::Any))
    end

    ECR.def_to_s "src/views/players.ecr"
  end

  struct PlayerCities
    def initialize(@owner : Int32, @owner_name : String, @cities : Array(JSON::Any))
    end

    ECR.def_to_s "src/views/player_cities.ecr"
  end

  struct ErrorPage
    def initialize(@heading : String, @detail : String)
    end

    ECR.def_to_s "src/views/error.ecr"
  end

  def self.page(title : String, body, path : String) : String
    Layout.new(title, body.to_s, path).to_s
  end
end
