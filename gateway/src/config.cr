require "./game_process"
require "./game_client"

module Ctp2Gateway
  # Process-wide runtime configuration, set by the entry point (CLI/env)
  # before the first request.
  module Config
    DEFAULT_SOCKET = "/tmp/ctp2-smoke.sock"

    class_property socket_path : String = DEFAULT_SOCKET

    # Present only when the gateway spawned the game (--spawn).
    # Not a DI service on purpose: it's optional process state owned by the
    # entry point, not a collaborator controllers should construct.
    class_property process : GameProcess? = nil

    # THE GameClient — exactly one per process, owned here and handed to the
    # DI container by the service factory. This must NOT be a plain
    # container singleton: Athena's container is FIBER-LOCAL and every
    # request runs in its own fiber, so a container-managed GameClient
    # would be rebuilt per request — each with its own socket connection.
    # The game accepts ONE client (backlog 1); per-request connections
    # orphan in its accept queue until connect() returns ECONNREFUSED.
    # One process-level instance = one serializer = correctness.
    @@client : GameClient? = nil

    def self.client : GameClient
      @@client ||= GameClient.new(socket_path)
    end

    # Specs: point the singleton at a per-test FakeGame.
    def self.reset_client!(socket_path : String) : GameClient
      self.socket_path = socket_path
      client.reconfigure(socket_path)
      client
    end
  end
end
