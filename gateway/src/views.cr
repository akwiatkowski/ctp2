require "ecr"
require "html"
require "json"

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
                   @leader_name : String?, @leader_score : Int32)
    end

    ECR.def_to_s "src/views/dashboard_ledger.ecr"
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
