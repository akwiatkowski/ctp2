# ctp2-gateway

One-process observability/control server for CTP2. It fronts the game's
Unix-socket test API (`smoketest_server.cpp` + `game_controller.cpp`) and will
grow four faces on one port:

| Face        | Route        | Status |
|-------------|--------------|--------|
| curl API    | `/api/*`     | ✅ v0.1 |
| health      | `/healthz`   | ✅ v0.1 |
| admin panel (game internals, future web UI)  | `/`, `/players`, `/players/<id>/cities` | ✅ v0.1 |
| debug / exchange journal                     | `/debug`    | ✅ v0.1 |
| MCP (streamable HTTP, Claude plays the game) | `POST /mcp` | ✅ v0.1 |
| WebSocket (live events)                      | `/ws`       | planned (v0.2, needs C++ event push) |

The admin pages are **omniscient** (no fog of war) — they ride the game's
admin-query verbs (`query_players`, `query_player_cities <id>`), unlike the
`/api/cities|units|map` routes which report the human player's view. JSON
twins exist at `/api/players` and `/api/players/<id>/cities`.

## Run

```sh
cd gateway && mise exec -- shards build

# Option A — one command: spawn and supervise the headless game yourself
./bin/ctp2-gateway --spawn --spawn-args "--players 3 --seed 42"

# Option B — attach: start the game separately (either binary), any order
./build/ctp2_headless --serve --players 3 --seed 42   # from the repo root
./gateway/bin/ctp2-gateway
```

`--spawn` auto-detects `build/ctp2_headless` (from the repo root or
`gateway/`), runs it with the repo root as cwd (asset loading), writes its
output to `--spawn-log` (default `/tmp/ctp2-headless.log`), reports its
pid/exit state under `"spawn"` in `/healthz`, and terminates it on gateway
shutdown (SIGTERM, then SIGKILL after 5s). If a game is already serving the
socket, the gateway refuses to race it and attaches instead. There is
deliberately no auto-respawn — a crash would loop; the lazy reconnect picks
the game back up whenever it returns.

Flags: `--port`, `--socket`, `--session-dir`, `--spawn`, `--binary`,
`--spawn-args`, `--spawn-log`, `--spawn-cwd`. Env: `CTP2_GATEWAY_PORT`,
`CTP2_SOCKET`, `CTP2_SESSION_DIR`, `CTP2_BINARY`.

Each gateway run gets a **session id** (timestamp + random suffix). The
exchange journal — every command sent to the game and its full response — is
appended to `<session-dir>/<id>.journal.jsonl`. If the gateway restarts, a
new session is created; if a client is pointed back at an existing journal
path, the most recent exchanges are replayed into memory so the debug page
stays useful after a crash. The session id is printed at startup and exposed
in `/healthz`.

## curl cookbook

```sh
curl localhost:8666/healthz
curl -X POST localhost:8666/api/cmd -d '{"cmd":"start_game"}'   # takes seconds: world gen
curl -X POST localhost:8666/api/cmd -d '{"cmd":"build_city"}'
curl localhost:8666/api/cities
curl localhost:8666/api/city/0                                  # incl. buildable list
curl localhost:8666/api/units
curl localhost:8666/api/map
curl -X POST localhost:8666/api/cmd -d '{"cmd":"set_production 0 settler"}'
curl -X POST localhost:8666/api/cmd -d '{"cmd":"save_game /tmp/test.json"}'
```

`POST /api/cmd` is a raw passthrough — any verb `game_controller::Dispatch`
(or the frontend serve loop) understands.

## MCP — Claude plays the game

```sh
claude mcp add --transport http ctp2 http://localhost:8666/mcp
```

16 tools, tools-only server (JSON-RPC 2.0 over streamable HTTP, protocol
2025-06-18): `start_game` (composite new_game+start_game), `end_turn`
(max 20 rounds/call — socket timeout budget), `build_city`,
`set_production`, `save_game`/`load_game` (checkpointing for experiments),
the full query family (player-view AND omniscient admin), `raw_cmd`
escape hatch, and `gateway_health` (connection state + command journal,
for when the model needs to debug the *gateway*). Game-level errors
(`no_settler_found`...) surface as tool errors with the JSON detail so
the model can adapt. Notifications → 202; GET/DELETE → 405 (no SSE
stream, stateless sessions).

## Who said no? — status code contract

Game responses pass through verbatim with **HTTP 200**, including the game's
own `{"status":"error",...}` replies — "the game says no" is a successful
roundtrip. Non-200 codes are reserved for *gateway* failures and always carry
`"gateway": true`:

| Code | Meaning |
|------|---------|
| 400  | malformed body / multi-line command (protocol injection guard) |
| 429  | request queue full (backpressure) |
| 502  | game replied with non-JSON |
| 503  | game socket disconnected |
| 504  | game accepted the command but didn't reply within the timeout |

## Design

The game accepts **one client and one in-flight command at a time** (the C++
side holds an SDL mutex from command poll to response send). The gateway is
the single serializer: one fiber owns the `UNIXSocket`, every HTTP request
funnels through a `Channel(Request)`, reply channels are buffered so a caller
that times out can never wedge the owner fiber. A timed-out connection is
*poisoned* (its late reply would pair with the next request) and dropped;
reconnection is lazy and per-request, which makes the gateway self-healing
across game restarts.

Built on [Athena Framework](https://athenaframework.org) 0.22 (adopted
2026-06-10, previously stdlib-only): annotation routing with typed/regex-
constrained params (`@[ARTA::Get("/players/{id}/cities")]`), constructor DI
for controllers, and an exception-event listener that owns ALL error
rendering. Two architecture notes future readers need:

* **GameClient is a process singleton owned by `Config`**, handed to the DI
  container by a factory. Athena's container is fiber-local with one fiber
  per request — a container-managed client would open one game connection
  per request and wedge the game's one-client socket (ECONNREFUSED after
  the backlog fills).
* **Static assets ride `ATH.run`'s `prepend_handlers`** (before the
  framework) — the same seam the future /ws WebSocket face will use.

System dependency: `brew install libmagic` (athena-mime links it).

## Tests

```sh
cd gateway && mise exec -- crystal spec
```

Specs run against an in-process `FakeGame` Unix server that speaks the real
line-JSON protocol with failure knobs (slow replies, mid-stream disconnects,
garbage responses). No game binary required.

## Roadmap

See `~/projects/claude/plans/ctp2-gateway.md`. Next: map-grid admin view
(session C remainder), C++ event push → live WebSocket face (v0.2).
