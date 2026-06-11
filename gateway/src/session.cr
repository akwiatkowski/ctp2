require "./game_client"
require "./game_process"

module Ctp2Gateway
  # A gateway session groups one game pairing (socket + optional supervised
  # process) under a stable identity. The session id is used to label every
  # exchange in the journal so the ledger survives gateway restarts and can
  # later be correlated across multiple concurrent or sequential games.
  class Session
    getter id : String
    getter socket_path : String
    getter journal_path : String
    getter client : GameClient
    property process : GameProcess?
    getter created_at : Time

    def initialize(@socket_path : String,
                   @process : GameProcess? = nil,
                   @session_dir : String = default_session_dir)
      @id = generate_id
      @created_at = Time.utc
      Dir.mkdir_p(@session_dir)
      @journal_path = File.join(@session_dir, "#{@id}.journal.jsonl")
      @client = GameClient.new(@socket_path, session_id: @id, journal_path: @journal_path)
    end

    private def generate_id : String
      # Timestamp + random suffix: lexicographically sortable and unique enough
      # for a dev gateway. Colons replaced so the id is filesystem-safe.
      "#{Time.utc.to_rfc3339(fraction_digits: 6).gsub(/[:]/, "-")}-#{Random::Secure.hex(4)}"
    end

    private def default_session_dir : String
      ENV["CTP2_SESSION_DIR"]? || File.expand_path("../../sessions", __DIR__)
    end
  end
end
