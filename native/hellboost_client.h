#ifndef HELLBOOST_CLIENT_H
#define HELLBOOST_CLIENT_H

#include <stdint.h>

#if defined(_WIN32)
# if defined(HB_BUILD)
#  define HB_API __declspec(dllexport)
# else
#  define HB_API __declspec(dllimport)
# endif
# define HB_CALL __cdecl
#else
# define HB_API __attribute__((visibility("default")))
# define HB_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HB_Context HB_Context;
enum HB_Status {
    HB_OK = 0, HB_EMPTY = 1, HB_IGNORED = 2, HB_DISABLED = 3,
    HB_INVALID = -1, HB_QUEUE_FULL = -2, HB_INTERNAL = -3
};
enum HB_Source { HB_INTERNET = 1, HB_FAVORITES = 2, HB_HISTORY = 3, HB_CONSOLE = 4 };
enum HB_Outcome { HB_ENTERED_GAME = 1, HB_FAILED = 2, HB_CANCELLED = 3, HB_SUPERSEDED = 4 };

/* All timestamps originate in the library's monotonic clock, not wall time.
 * IDs are unique within one context. A transport must pair them with its own
 * per-process/session UUID before sending them to a backend.
 * Result records are untrusted client observations, not authenticated Steam IDs.
 */
typedef struct HB_Result {
    uint32_t abi_version;
    uint32_t source;
    uint32_t outcome;
    uint32_t reserved;
    uint64_t attempt_id;
    uint64_t elapsed_ms;
    char endpoint[256];
    char nickname[129]; /* optional UTF-8, at most 128 bytes */
} HB_Result;

HB_API uint32_t HB_CALL hb_abi_version(void);
/* Disabled by default in consumers until the user enables collection. */
HB_API HB_Context* HB_CALL hb_create(int enabled);
/* No calls may run concurrently with destroy. All other calls are serialized. */
HB_API void HB_CALL hb_destroy(HB_Context* context);
/* Disabling discards pending attempts and queued records immediately. */
HB_API int HB_CALL hb_set_enabled(HB_Context* context, int enabled);
/* Called by an ENGINE adapter at connection start, NOT by process launch.
 * Strings use explicit byte lengths; nickname may be NULL only with length 0.
 * A full result queue rejects a superseding begin without changing the attempt.
 */
HB_API int HB_CALL hb_begin(HB_Context* context,
    const char* endpoint, uint32_t endpoint_length,
    const char* nickname, uint32_t nickname_length,
    uint32_t source, uint64_t* attempt_id);
/* Called only for a verified engine event belonging to this attempt.
 * Accepted outcomes: entered_game, failed, cancelled. Duplicate/stale IDs ignored.
 * HB_QUEUE_FULL means no state changed; drain the queue then retry the event.
 */
HB_API int HB_CALL hb_complete(HB_Context* context, uint64_t attempt_id, uint32_t outcome);
/* Peek does not consume. Acknowledge only after durable handling by transport.
 * No network requests or disk writes occur in this library.
 */
HB_API int HB_CALL hb_peek_result(HB_Context* context, HB_Result* result, uint32_t result_size);
HB_API int HB_CALL hb_ack_result(HB_Context* context, uint64_t attempt_id);

#ifdef __cplusplus
}
#endif
#endif
