#include "../connection_tracker.hpp"
#include <cassert>

using namespace hellboost;
using namespace std::chrono_literals;

int main() {
    ConnectionTracker tracker;
    const auto t = ConnectionTracker::Time{};
    assert(!tracker.begin({"a", "127.0.0.1:27015", Source::internet}, t));
    assert(tracker.pending());
    assert(!tracker.complete("stale", Outcome::entered_game, t + 1s));
    auto result = tracker.complete("a", Outcome::entered_game, t + 1250ms);
    assert(result && result->elapsed == 1250ms);
    assert(result->outcome == Outcome::entered_game);
    assert(result->attempt.source == Source::internet);
    assert(!tracker.pending());
    assert(!tracker.complete("a", Outcome::entered_game, t + 2s));

    tracker.begin({"b", "127.0.0.1:27015", Source::favorites}, t);
    auto replaced = tracker.begin({"c", "127.0.0.2:27015", Source::console}, t + 3s);
    assert(replaced && replaced->outcome == Outcome::superseded);
    assert(replaced->elapsed == 3000ms);
    assert(!tracker.complete("b", Outcome::entered_game, t + 4s));
    auto failed = tracker.complete("c", Outcome::failed, t + 4s);
    assert(failed && failed->outcome == Outcome::failed && failed->elapsed == 1000ms);

    tracker.begin({"d", "127.0.0.1:27015", Source::history}, t);
    try {
        tracker.complete("d", Outcome::entered_game, t - 1ms);
        assert(false);
    } catch (const std::invalid_argument&) {}
    assert(tracker.pending());
    auto cancelled = tracker.complete("d", Outcome::cancelled, t + 20ms);
    assert(cancelled && cancelled->outcome == Outcome::cancelled);
    try {
        tracker.begin({"", "server", Source::internet}, t);
        assert(false);
    } catch (const std::invalid_argument&) {}
}
