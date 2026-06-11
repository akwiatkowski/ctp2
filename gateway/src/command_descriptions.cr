require "./mcp/tools"

module Ctp2Gateway
  # Human-readable sentences for the game verbs shown in /debug. Keeps the
  # debug page useful even for people who have not memorized the smoke-test
  # command vocabulary.
  module CommandDescriptions
    # Returns a one-sentence description for a verb, or nil if none is known.
    def self.for(verb : String) : String?
      MAP[verb]?
    end

    # Build the map from the live MCP tool registry so tool and debug
    # descriptions stay in sync. Composite tools (start_game) and a few
    # internal-only verbs get manual overrides.
    MAP = begin
      map = {} of String => String
      Mcp::TOOLS.each do |tool|
        case tool.name
        when "start_game"
          # start_game is composite: new_game prepares, start_game initializes.
          map["new_game"]  = "Prepare a fresh game world (pairing step before start_game)."
          map["start_game"] = "Generate and start a new CTP2 game world."
        else
          # Most tools send exactly their name as the verb.
          map[tool.name] = tool.description
        end
      end
      map
    end
  end
end
