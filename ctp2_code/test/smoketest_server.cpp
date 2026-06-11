/**
 * @file smoketest_server.cpp
 * @brief Unix domain socket server for smoke test command injection.
 *
 * Uses SDL threads/mutexes for consistency with the codebase.
 * Protocol: newline-delimited JSON over Unix domain socket.
 *
 * Request:  {"cmd":"new_game"}\n
 * Response: {"status":"ok","cmd":"new_game"}\n
 */

#include "ctp/c3.h"
#include "test/smoketest_server.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <SDL.h>

// Socket state
static int g_smoke_listen_fd = -1;
static int g_smoke_client_fd = -1;
static const char* g_smoke_socket_path = "/tmp/ctp2-smoke.sock";

// Threading primitives (SDL)
static SDL_mutex* g_smoke_mutex = nullptr;
static SDL_cond*  g_smoke_cond  = nullptr;
static SDL_Thread* g_smoke_thread = nullptr;

// Command/response buffers.  The response is a std::string, not a fixed
// buffer: query verbs (e.g. query_city's buildable list) return JSON payloads
// far larger than the old 512-byte cap, which silently truncated them.
static char        g_smoke_command[256];
static std::string g_smoke_response;
static int         g_smoke_has_command = 0;
static int         g_smoke_has_response = 0;

/**
 * Parse a simple JSON command from a buffer.
 * Looks for "cmd":"value" pattern.
 *
 * @param buf   Raw input buffer.
 * @param cmd   Output buffer for command name.
 * @param max   Size of cmd buffer.
 * @return      1 if parsed successfully, 0 otherwise.
 */
static int parse_json_cmd(const char* buf, char* cmd, int max)
{
    const char* p = strstr(buf, "\"cmd\"");
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;

    p = strchr(p, '"');
    if (!p) return 0;
    p++; // skip opening quote

    const char* end = strchr(p, '"');
    if (!end) return 0;

    int len = (int)(end - p);
    if (len <= 0 || len >= max) return 0;

    memcpy(cmd, p, len);
    cmd[len] = '\0';
    return 1;
}

/**
 * Server thread: accept connections and read commands.
 * For each command, stores it and waits for the main thread to respond.
 */
static int smoke_server_thread(void* /*data*/)
{
    // Remove stale socket
    unlink(g_smoke_socket_path);

    g_smoke_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_smoke_listen_fd < 0) {
        fprintf(stderr, "[SMOKE] Failed to create socket\n");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strlcpy(addr.sun_path, g_smoke_socket_path, sizeof(addr.sun_path));

    if (bind(g_smoke_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[SMOKE] Failed to bind socket to %s\n", g_smoke_socket_path);
        close(g_smoke_listen_fd);
        g_smoke_listen_fd = -1;
        return 1;
    }

    if (listen(g_smoke_listen_fd, 1) < 0) {
        fprintf(stderr, "[SMOKE] Failed to listen on socket\n");
        close(g_smoke_listen_fd);
        g_smoke_listen_fd = -1;
        return 1;
    }

    fprintf(stderr, "[SMOKE] Server listening on %s\n", g_smoke_socket_path);

    // Accept one client at a time
    while (true) {
        int client = accept(g_smoke_listen_fd, nullptr, nullptr);
        if (client < 0) {
            // Socket likely closed during shutdown
            break;
        }

        g_smoke_client_fd = client;
        fprintf(stderr, "[SMOKE] Client connected\n");

        // The protocol is newline-delimited, but read() has no notion of
        // lines: one command can arrive split across reads, or several can
        // coalesce into one. Accumulate and process COMPLETE lines only —
        // handing a partial buffer to the parser used to emit an unsolicited
        // invalid_json response, which permanently desynced the client's
        // request/response pairing (every later reply was off by one).
        char read_buf[512];
        char cmd[256];
        std::string acc;

        while (true) {
            int n = (int)read(client, read_buf, sizeof(read_buf));
            if (n <= 0) {
                break; // Client disconnected
            }
            acc.append(read_buf, (size_t)n);

            size_t nl;
            while ((nl = acc.find('\n')) != std::string::npos) {
                std::string line = acc.substr(0, nl);
                acc.erase(0, nl + 1);
                if (line.empty()) continue;

                if (!parse_json_cmd(line.c_str(), cmd, sizeof(cmd))) {
                    const char* err = "{\"status\":\"error\",\"cmd\":\"\",\"detail\":\"invalid_json\"}\n";
                    write(client, err, strlen(err));
                    continue;
                }

                fprintf(stderr, "[SMOKE] Received command: %s\n", cmd);

                // Hand off to main thread
                SDL_LockMutex(g_smoke_mutex);

                strlcpy(g_smoke_command, cmd, sizeof(g_smoke_command));
                g_smoke_has_command = 1;
                g_smoke_has_response = 0;

                // Wait for main thread to process
                while (!g_smoke_has_response) {
                    SDL_CondWait(g_smoke_cond, g_smoke_mutex);
                }

                // Send response back to client
                if (g_smoke_client_fd >= 0) {
                    write(g_smoke_client_fd, g_smoke_response.data(), g_smoke_response.size());
                }

                g_smoke_has_command = 0;
                g_smoke_has_response = 0;

                SDL_UnlockMutex(g_smoke_mutex);
            }
        }

        close(client);
        g_smoke_client_fd = -1;
        fprintf(stderr, "[SMOKE] Client disconnected\n");
    }

    return 0;
}

void smoketest_server_init(void)
{
    g_smoke_mutex = SDL_CreateMutex();
    g_smoke_cond  = SDL_CreateCond();

    if (!g_smoke_mutex || !g_smoke_cond) {
        fprintf(stderr, "[SMOKE] Failed to create mutex/cond\n");
        return;
    }

    g_smoke_thread = SDL_CreateThread(smoke_server_thread, "SmokeTestServer", nullptr);
    if (!g_smoke_thread) {
        fprintf(stderr, "[SMOKE] Failed to create server thread\n");
    }
}

void smoketest_server_shutdown(void)
{
    // Signal thread to exit by closing listen socket
    if (g_smoke_listen_fd >= 0) {
        close(g_smoke_listen_fd);
        g_smoke_listen_fd = -1;
    }

    if (g_smoke_client_fd >= 0) {
        close(g_smoke_client_fd);
        g_smoke_client_fd = -1;
    }

    if (g_smoke_thread) {
        SDL_WaitThread(g_smoke_thread, nullptr);
        g_smoke_thread = nullptr;
    }

    if (g_smoke_mutex) {
        SDL_DestroyMutex(g_smoke_mutex);
        g_smoke_mutex = nullptr;
    }

    if (g_smoke_cond) {
        SDL_DestroyCond(g_smoke_cond);
        g_smoke_cond = nullptr;
    }

    unlink(g_smoke_socket_path);
}

int smoketest_poll_command(char* out_cmd, int max_len)
{
    if (!g_smoke_mutex) return 0;

    SDL_LockMutex(g_smoke_mutex);

    if (!g_smoke_has_command) {
        SDL_UnlockMutex(g_smoke_mutex);
        return 0;
    }

    // TODO(phase-2): strncpy → strlcpy — dst is char* or non-standard length, requires manual review
    strncpy(out_cmd, g_smoke_command, max_len - 1);
    out_cmd[max_len - 1] = '\0';

    // Consume the command NOW, while we still hold the mutex. The server
    // thread also clears this after writing the response, but only after it is
    // next scheduled — a caller that re-polls in a tight loop (the headless
    // --serve loop) would otherwise re-read the same command many times before
    // the server thread runs. The UI build hid this race by polling once per
    // frame. Clearing here makes a command consumed exactly once.
    g_smoke_has_command = 0;

    // Keep mutex locked — caller must call send_response to release it
    return 1;
}

/**
 * Store the response (appending the protocol's newline terminator) and wake
 * the server thread.
 *
 * Called with the smoke mutex held (locked by smoketest_poll_command).
 * ALWAYS signals and unlocks — including when storing the response throws
 * (std::bad_alloc on a huge payload) — otherwise the mutex would stay locked
 * forever and every subsequent poll would wedge.
 */
static void smoke_store_response_and_release(const char* line)
{
    try {
        g_smoke_response.assign(line);
        g_smoke_response.push_back('\n');
    } catch (...) {
        // clear() keeps the existing capacity and never throws; the short
        // fallback below reuses it.  If even that allocation fails, an
        // empty response still beats a wedged server.
        g_smoke_response.clear();
        try {
            g_smoke_response.assign(
                "{\"status\":\"error\",\"detail\":\"response_alloc_failed\"}\n");
        } catch (...) {}
    }

    g_smoke_has_response = 1;
    SDL_CondSignal(g_smoke_cond);
    SDL_UnlockMutex(g_smoke_mutex);
}

void smoketest_send_response(const char* status, const char* cmd, const char* detail)
{
    if (!g_smoke_mutex) return;

    char buf[512];
    if (detail && detail[0]) {
        snprintf(buf, sizeof(buf),
                 "{\"status\":\"%s\",\"cmd\":\"%s\",\"detail\":\"%s\"}",
                 status, cmd, detail);
    } else {
        snprintf(buf, sizeof(buf),
                 "{\"status\":\"%s\",\"cmd\":\"%s\"}",
                 status, cmd);
    }
    smoke_store_response_and_release(buf);
}

void smoketest_send_json(const char* json_line)
{
    if (!g_smoke_mutex) return;

    // json_line is a complete single-line JSON object (no trailing newline).
    smoke_store_response_and_release(json_line ? json_line : "{}");
}
