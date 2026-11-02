#include "../hellboost_client.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Failed at line %d: %s\n", __LINE__, #condition); \
    std::exit(1); } } while (false)

static uint64_t begin(HB_Context* context, uint32_t source = HB_INTERNET) {
    const char endpoint[] = "play.example.com:27015";
    uint64_t id = 0;
    CHECK(hb_begin(context, endpoint, sizeof(endpoint) - 1, "Player", 6, source, &id) == HB_OK);
    CHECK(id != 0);
    return id;
}

int main() {
    CHECK(hb_abi_version() == 1);
    HB_Context* context = hb_create(0);
    CHECK(context);
    HB_Result result{};
    uint64_t id = 0;
    CHECK(hb_begin(context, "host:27015", 10, nullptr, 0, HB_INTERNET, &id) == HB_DISABLED);
    CHECK(hb_set_enabled(context, 1) == HB_OK);

    const char* invalid[] = {"host:0", "host:65536", "host:12;quit", "host\n:12", ":12", "host:"};
    for (const auto text : invalid) {
        CHECK(hb_begin(context, text, static_cast<uint32_t>(std::strlen(text)), nullptr, 0, HB_INTERNET, &id) == HB_INVALID);
    }
    CHECK(hb_begin(context, "host:27015", 10, "a\0b", 3, HB_INTERNET, &id) == HB_INVALID);
    CHECK(hb_begin(context, "host:27015", 10, "\xc0\x80", 2, HB_INTERNET, &id) == HB_INVALID);
    CHECK(hb_begin(context, "host:27015", 10, "\xed\xa0\x80", 3, HB_INTERNET, &id) == HB_INVALID);
    CHECK(hb_begin(context, "host:27015", 10, "\xc8\x99", 2, HB_INTERNET, &id) == HB_OK);
    CHECK(hb_complete(context, id, HB_CANCELLED) == HB_OK);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_OK);
    CHECK(std::strcmp(result.nickname, "\xc8\x99") == 0);
    CHECK(hb_ack_result(context, id) == HB_OK);

    const auto first = begin(context, HB_FAVORITES);
    const auto second = begin(context, HB_HISTORY);
    CHECK(hb_complete(context, first, HB_ENTERED_GAME) == HB_IGNORED);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_OK);
    CHECK(result.attempt_id == first && result.outcome == HB_SUPERSEDED);
    CHECK(result.source == HB_FAVORITES);
    CHECK(hb_ack_result(context, second) == HB_IGNORED);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_OK);
    CHECK(result.attempt_id == first);
    CHECK(hb_ack_result(context, first) == HB_OK);
    CHECK(hb_complete(context, second, HB_ENTERED_GAME) == HB_OK);
    CHECK(hb_complete(context, second, HB_ENTERED_GAME) == HB_IGNORED);
    CHECK(hb_peek_result(context, &result, sizeof(result) - 1) == HB_INVALID);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_OK);
    CHECK(result.attempt_id == second && result.source == HB_HISTORY && result.outcome == HB_ENTERED_GAME);
    CHECK(std::strcmp(result.endpoint, "play.example.com:27015") == 0);
    CHECK(hb_ack_result(context, second) == HB_OK);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_EMPTY);

    for (unsigned i = 0; i < 128; ++i) {
        const auto next = begin(context);
        CHECK(hb_complete(context, next, HB_FAILED) == HB_OK);
    }
    const auto pending = begin(context);
    CHECK(hb_complete(context, pending, HB_ENTERED_GAME) == HB_QUEUE_FULL);
    CHECK(hb_begin(context, "host:27015", 10, nullptr, 0, HB_INTERNET, &id) == HB_QUEUE_FULL);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_OK);
    CHECK(hb_ack_result(context, result.attempt_id) == HB_OK);
    CHECK(hb_complete(context, pending, HB_ENTERED_GAME) == HB_OK);

    CHECK(hb_set_enabled(context, 0) == HB_OK);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_DISABLED);
    CHECK(hb_set_enabled(context, 1) == HB_OK);
    CHECK(hb_peek_result(context, &result, sizeof(result)) == HB_EMPTY);
    CHECK(hb_complete(context, pending, HB_ENTERED_GAME) == HB_IGNORED);

    std::vector<std::thread> workers;
    for (unsigned i = 0; i < 4; ++i) {
        workers.emplace_back([context]() {
            for (unsigned j = 0; j < 16; ++j) {
                const auto attempt = begin(context);
                const int code = hb_complete(context, attempt, HB_FAILED);
                CHECK(code == HB_OK || code == HB_IGNORED);
            }
        });
    }
    for (auto& worker : workers) worker.join();
    unsigned count = 0;
    while (hb_peek_result(context, &result, sizeof(result)) == HB_OK) {
        CHECK(hb_ack_result(context, result.attempt_id) == HB_OK);
        ++count;
    }
    CHECK(count == 64);
    CHECK(hb_complete(nullptr, 1, HB_FAILED) == HB_INVALID);
    hb_destroy(context);
    std::puts("Native ABI: lifecycle, validation, supersession, deduplication, queue pressure, consent and concurrency passed.");
}
