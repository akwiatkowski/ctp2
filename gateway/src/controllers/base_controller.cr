require "athena"
require "../game_client"
require "../config"
require "../views"

module Ctp2Gateway
  # Shared plumbing for all gateway controllers: the game-client dependency
  # (constructor-injected by the DI container) and the response vocabulary.
  #
  # Status-code contract (unchanged from the pre-Athena gateway): game
  # replies pass through as 200 — including the game's own
  # {"status":"error"} answers — and non-200 always means a GATEWAY
  # failure, marked with "gateway":true.
  abstract class BaseController < ATH::Controller
    JSON_CT = "application/json"
    HTML_CT = "text/html; charset=utf-8"

    def initialize(@client : Ctp2Gateway::GameClient)
    end

    # Raw passthrough of a game roundtrip → JSON response.
    protected def passthrough(result : GameClient::Result) : AHTTP::Response
      case result
      in GameClient::Ok
        AHTTP::Response.new(result.payload.to_json, headers: HTTP::Headers{"content-type" => JSON_CT})
      in GameClient::Err
        gateway_error(err_status(result), result.kind.to_s.underscore, result.detail)
      end
    end

    protected def gateway_error(status : Int32, kind : String, detail : String) : AHTTP::Response
      AHTTP::Response.new(
        {status: "error", gateway: true, kind: kind, detail: detail}.to_json,
        status: status,
        headers: HTTP::Headers{"content-type" => JSON_CT},
      )
    end

    protected def err_status(err : GameClient::Err) : Int32
      status = case err.kind
               in .busy?         then 429
               in .protocol?     then 502
               in .disconnected? then 503
               in .timeout?      then 504
               end
      # `protocol` from our own pre-validation (multi-line cmd) is caller
      # error, not the game's fault.
      status = 400 if err.kind.protocol? && err.detail.includes?("single line")
      status
    end

    # Run a query and split the outcome: {result, gateway_error,
    # game_error_detail}. Exactly one of the latter two is non-nil on
    # failure.
    protected def fetch(cmd : String) : {JSON::Any?, GameClient::Err?, String?}
      case r = @client.command(cmd)
      in GameClient::Ok
        if r.payload["status"]?.try(&.as_s) == "ok"
          {r.payload["result"]?, nil, nil}
        else
          {nil, nil, r.payload["detail"]?.try(&.as_s) || "unknown game error"}
        end
      in GameClient::Err
        {nil, r, nil}
      end
    end

    # `nav_path` drives the masthead's aria-current highlight.
    protected def html(status : Int32, title : String, body, nav_path : String) : AHTTP::Response
      AHTTP::Response.new(
        Views.page(title, body, nav_path),
        status: status,
        headers: HTTP::Headers{"content-type" => HTML_CT},
      )
    end

    protected def spawn_line : String?
      Config.process.try do |p|
        s = p.status_json
        s[:running] ? "pid #{s[:pid]}, running" : "exited#{s[:exit_code] ? " (code #{s[:exit_code]})" : ""}"
      end
    end

    # The live part of the dashboard (status strip + stat cards) — shared
    # between the full page and the htmx polling fragment.
    protected def build_ledger : Views::DashboardLedger
      result, gw_err, game_err = fetch("query_players")
      error = gw_err.try(&.detail) || game_err
      players = result.try(&.["players"].as_a) || [] of JSON::Any
      leader = players.reject { |p| p["dead"].as_bool }.max_by? { |p| p["score"].as_i }

      # The clock — skipped when the players query already failed (same
      # failure would just repeat).
      round = year = nil.as(Int32?)
      if error.nil?
        tresult, _, _ = fetch("query_turn")
        round = tresult.try(&.["round"].as_i)
        year = tresult.try(&.["year"].as_i)
      end

      Views::DashboardLedger.new(
        socket: @client.socket_path,
        connected: @client.connected?,
        spawn_line: spawn_line,
        game_error: error,
        player_count: players.size,
        alive_count: players.count { |p| !p["dead"].as_bool },
        city_count: players.sum { |p| p["num_cities"].as_i },
        leader_name: leader.try { |l| l["name"].as_s.presence },
        leader_country: leader.try { |l| l["country"]?.try(&.as_s?).try(&.presence) },
        leader_score: leader.try(&.["score"].as_i) || 0,
        round: round,
        year: year,
      )
    end
  end
end
