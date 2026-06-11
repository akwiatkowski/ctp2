require "./game_process"
require "./game_client"

module Ctp2Gateway
  # Process-wide runtime configuration, set by the entry point (CLI/env)
  # before the first request.
  module Config
    DEFAULT_SOCKET = "/tmp/ctp2-smoke.sock"
    DEFAULT_SESSION_DIR = File.expand_path("../../sessions", __DIR__)

    class_property socket_path : String = DEFAULT_SOCKET
    class_property session_dir : String = DEFAULT_SESSION_DIR

    # Present only when the gateway spawned the game (--spawn).
    # Not a DI service on purpose: it's optional process state owned by the
    # entry point, not a collaborator controllers should construct.
    class_property process : GameProcess? = nil

    # THE active Session — exactly one per process, owned here and handed to
    # the DI container by the service factory. This must NOT be a plain
    # container singleton: Athena's container is FIBER-LOCAL and every
    # request runs in its own fiber, so a container-managed GameClient
    # would be rebuilt per request — each with its own socket connection.
    # The game accepts ONE client (backlog 1); per-request connections
    # orphan in its accept queue until connect() returns ECONNREFUSED.
    # One process-level instance = one serializer = correctness.
    #
    # Using a Session (instead of a bare GameClient) gives us a stable id,
    # a durable journal path, and a data shape that is ready for multiple
    # games in the future.
    @@session : Session? = nil

    def self.session : Session
      @@session ||= Session.new(socket_path, process, session_dir)
    end

    def self.client : GameClient
      session.client
    end

    # Specs: point the singleton at a per-test FakeGame. Creating a new
    # Session gives each test a fresh id and journal while keeping the
    # process-level singleton contract intact.
    def self.reset_client!(socket_path : String) : GameClient
      self.socket_path = socket_path
      old = @@session
      @@session = Session.new(socket_path, process, session_dir)
      old.try &.client.close
      client
    end
  end
end

require "./session"
