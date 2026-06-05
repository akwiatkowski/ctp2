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

#include <SDL.h>

// Socket state
static int g_smoke_listen_fd = -1;
static int g_smoke_client_fd = -1;
static const char* g_smoke_socket_path = "/tmp/ctp2-smoke.sock";

// Threading primitives (SDL)
static SDL_mutex* g_smoke_mutex = nullptr;
static SDL_cond*  g_smoke_cond  = nullptr;
static SDL_Thread* g_smoke_thread = nullptr;

// Command/response buffers
static char g_smoke_command[256];
static char g_smoke_response[512];
static int  g_smoke_has_command = 0;
static int  g_smoke_has_response = 0;

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

        char read_buf[512];
        char cmd[256];

        while (true) {
            memset(read_buf, 0, sizeof(read_buf));
            int n = (int)read(client, read_buf, sizeof(read_buf) - 1);
            if (n <= 0) {
                break; // Client disconnected
            }

            // Parse command
            if (!parse_json_cmd(read_buf, cmd, sizeof(cmd))) {
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
                write(g_smoke_client_fd, g_smoke_response, strlen(g_smoke_response));
            }

            g_smoke_has_command = 0;
            g_smoke_has_response = 0;

            SDL_UnlockMutex(g_smoke_mutex);
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

    // Keep mutex locked — caller must call send_response to release it
    return 1;
}

void smoketest_send_response(const char* status, const char* cmd, const char* detail)
{
    if (!g_smoke_mutex) return;

    if (detail && detail[0]) {
        snprintf(g_smoke_response, sizeof(g_smoke_response),
                 "{\"status\":\"%s\",\"cmd\":\"%s\",\"detail\":\"%s\"}\n",
                 status, cmd, detail);
    } else {
        snprintf(g_smoke_response, sizeof(g_smoke_response),
                 "{\"status\":\"%s\",\"cmd\":\"%s\"}\n",
                 status, cmd);
    }

    g_smoke_has_response = 1;
    SDL_CondSignal(g_smoke_cond);
    SDL_UnlockMutex(g_smoke_mutex);
}
