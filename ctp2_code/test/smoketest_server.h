/**
 * @file smoketest_server.h
 * @brief Unix domain socket server for smoke test command injection.
 *
 * The game runs normally with SDL rendering. An external test harness
 * connects via Unix socket and sends commands like {"cmd":"new_game"}.
 * The server executes the same handler functions that real UI clicks would.
 */

#ifndef SMOKE_TEST_SERVER_H
#define SMOKE_TEST_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the smoke test server.
 * Spawns a background thread listening on /tmp/ctp2-smoke.sock.
 */
void smoketest_server_init(void);

/**
 * Shut down the smoke test server and clean up the socket.
 */
void smoketest_server_shutdown(void);

/**
 * Poll for a pending command from the test harness.
 * Must be called from the main thread (e.g. in CivApp::ProcessUI).
 *
 * @param out_cmd  Buffer to copy command into.
 * @param max_len  Size of out_cmd buffer.
 * @return         1 if a command is available, 0 otherwise.
 */
int smoketest_poll_command(char* out_cmd, int max_len);

/**
 * Send a response back to the test harness.
 * Must be called from the main thread after executing a command.
 *
 * @param status  "ok" or "error"
 * @param cmd     The command being responded to.
 * @param detail  Optional detail message (can be NULL).
 */
void smoketest_send_response(const char* status, const char* cmd, const char* detail);

/**
 * Send a raw single-line JSON response back to the test harness.
 * Used by GameController dispatch, whose handlers build their own JSON
 * (including structured query "result" objects). The newline terminator is
 * appended by the server. Must be called from the main thread after polling.
 *
 * @param json_line  Complete JSON object as a single line, no trailing newline.
 */
void smoketest_send_json(const char* json_line);

#ifdef __cplusplus
}
#endif

#endif // SMOKE_TEST_SERVER_H
