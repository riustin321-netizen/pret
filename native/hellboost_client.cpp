#include "hellboost_client.h"
#include "connection_tracker.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>

namespace {
constexpr std::size_t capacity = 128;

bool endpoint_valid(const char* text, uint32_t length) {
    if (!text || length < 3 || length > 255) return false;
    uint32_t colon = 0;
    bool found = false;
    for (uint32_t i = 0; i < length; ++i) {
        const char c = text[i];
        if (c == ':') {
            if (found || i == 0) return false;
            found = true;
            colon = i;
        } else if (!found) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '.' || c == '-')) return false;
        } else if (c < '0' || c > '9') return false;
    }
    if (!found || length - colon - 1 == 0 || length - colon - 1 > 5) return false;
    unsigned port = 0;
    for (uint32_t i = colon + 1; i < length; ++i) port = port * 10 + static_cast<unsigned>(text[i] - '0');
    return port >= 1 && port <= 65535;
}

bool nickname_valid(const char* text, uint32_t length) {
    if (length > 128 || (!text && length != 0)) return false;
    // Validate UTF-8, reject embedded NUL, control bytes, overlong sequences,
    // surrogate code points and values beyond Unicode's maximum.
    for (uint32_t i = 0; i < length;) {
        const auto c = static_cast<unsigned char>(text[i++]);
        if (c < 0x80) {
            if (c < 0x20 || c == 0x7f) return false;
            continue;
        }
        unsigned count = 0, value = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf) { count = 1; value = c & 0x1f; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { count = 2; value = c & 0x0f; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { count = 3; value = c & 7; minimum = 0x10000; }
        else return false;
        if (count > length - i) return false;
        for (unsigned j = 0; j < count; ++j) {
            const auto next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80) return false;
            value = (value << 6) | (next & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return false;
    }
    return true;
}

hellboost::Source source_value(uint32_t value) {
    switch (value) {
        case HB_INTERNET: return hellboost::Source::internet;
        case HB_FAVORITES: return hellboost::Source::favorites;
        case HB_HISTORY: return hellboost::Source::history;
        default: return hellboost::Source::console;
    }
}
}

struct HB_Context {
    std::mutex mutex;
    bool enabled;
    hellboost::ConnectionTracker tracker;
    uint64_t next_id = 1;
    uint64_t current_id = 0;
    uint32_t current_source = 0;
    std::array<char, 129> current_nickname{};
    std::array<HB_Result, capacity> results{};
    std::size_t head = 0;
    std::size_t size = 0;

    explicit HB_Context(bool value) : enabled(value) {}

    void enqueue(const hellboost::Result& result, uint32_t outcome) noexcept {
        HB_Result record{};
        record.abi_version = 1;
        record.source = current_source;
        record.outcome = outcome;
        record.attempt_id = current_id;
        record.elapsed_ms = static_cast<uint64_t>(result.elapsed.count());
        std::memcpy(record.endpoint, result.attempt.endpoint.data(), result.attempt.endpoint.size());
        std::memcpy(record.nickname, current_nickname.data(), current_nickname.size());
        results[(head + size) % capacity] = record;
        ++size;
    }
};

uint32_t HB_CALL hb_abi_version(void) { return 1; }

HB_Context* HB_CALL hb_create(int enabled) {
    try { return new HB_Context(enabled != 0); }
    catch (...) { return nullptr; }
}

void HB_CALL hb_destroy(HB_Context* context) { delete context; }

int HB_CALL hb_set_enabled(HB_Context* context, int enabled) {
    if (!context) return HB_INVALID;
    try {
        std::lock_guard<std::mutex> lock(context->mutex);
        context->enabled = enabled != 0;
        if (!context->enabled) {
            context->tracker = hellboost::ConnectionTracker{};
            context->results = {};
            context->current_nickname = {};
            context->current_id = 0;
            context->head = context->size = 0;
        }
        return HB_OK;
    } catch (...) { return HB_INTERNAL; }
}

int HB_CALL hb_begin(HB_Context* context, const char* endpoint, uint32_t endpoint_length,
                    const char* nickname, uint32_t nickname_length, uint32_t source,
                    uint64_t* attempt_id) {
    if (!context || !attempt_id || source < HB_INTERNET || source > HB_CONSOLE ||
        !endpoint_valid(endpoint, endpoint_length) || !nickname_valid(nickname, nickname_length)) return HB_INVALID;
    try {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (!context->enabled) return HB_DISABLED;
        if (context->tracker.pending() && context->size == capacity) return HB_QUEUE_FULL;
        if (context->next_id == std::numeric_limits<uint64_t>::max()) return HB_INTERNAL;
        const auto id = context->next_id;
        auto prior = context->tracker.begin({std::to_string(id), std::string(endpoint, endpoint_length), source_value(source)}, hellboost::ConnectionTracker::Clock::now());
        if (prior) context->enqueue(*prior, HB_SUPERSEDED);
        context->current_id = id;
        context->current_source = source;
        context->current_nickname = {};
        if (nickname_length) std::memcpy(context->current_nickname.data(), nickname, nickname_length);
        *attempt_id = id;
        ++context->next_id;
        return HB_OK;
    } catch (...) { return HB_INTERNAL; }
}

int HB_CALL hb_complete(HB_Context* context, uint64_t attempt_id, uint32_t outcome) {
    if (!context || outcome < HB_ENTERED_GAME || outcome > HB_CANCELLED) return HB_INVALID;
    try {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (!context->enabled) return HB_DISABLED;
        if (!context->tracker.pending() || attempt_id != context->current_id) return HB_IGNORED;
        if (context->size == capacity) return HB_QUEUE_FULL;
        auto value = outcome == HB_ENTERED_GAME ? hellboost::Outcome::entered_game :
            outcome == HB_FAILED ? hellboost::Outcome::failed : hellboost::Outcome::cancelled;
        auto result = context->tracker.complete(std::to_string(attempt_id), value, hellboost::ConnectionTracker::Clock::now());
        if (!result) return HB_IGNORED;
        context->enqueue(*result, outcome);
        return HB_OK;
    } catch (...) { return HB_INTERNAL; }
}

int HB_CALL hb_peek_result(HB_Context* context, HB_Result* result, uint32_t result_size) {
    if (!context || !result || result_size != sizeof(HB_Result)) return HB_INVALID;
    try {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (!context->enabled) return HB_DISABLED;
        if (!context->size) return HB_EMPTY;
        *result = context->results[context->head];
        return HB_OK;
    } catch (...) { return HB_INTERNAL; }
}

int HB_CALL hb_ack_result(HB_Context* context, uint64_t attempt_id) {
    if (!context) return HB_INVALID;
    try {
        std::lock_guard<std::mutex> lock(context->mutex);
        if (!context->enabled) return HB_DISABLED;
        if (!context->size || context->results[context->head].attempt_id != attempt_id) return HB_IGNORED;
        context->results[context->head] = {};
        context->head = (context->head + 1) % capacity;
        --context->size;
        return HB_OK;
    } catch (...) { return HB_INTERNAL; }
}
