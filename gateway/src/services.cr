require "athena"
require "./game_client"
require "./config"

# DI wiring lives here so GameClient itself stays framework-free (plain
# specs construct it directly). The factory returns the PROCESS-level
# singleton owned by Config — Athena's container is fiber-local (one per
# request), so a container-managed instance would mean one socket
# connection per request, which the game's one-client socket cannot
# serve (see Config.client for the full story).
@[ADI::Register(public: true, name: "game_client", factory: "for_config")]
class Ctp2Gateway::GameClient
  def self.for_config : self
    Ctp2Gateway::Config.client
  end
end
