// Fast loading's decision: when a load may run ahead of real time, and every
// guard that keeps real time. No game data.
#include "kernel/fast_loading.hpp"

#include <cstdint>
#include <iostream>

namespace {
using namespace mhp3rd::fast_loading;

int failures{};

void check(bool condition, const char *message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

constexpr std::uint64_t kMs = 1000u;

Guards open_guards() {
    Guards guards;
    guards.enabled = true;
    return guards;
}

void test_nothing_read_is_not_loading() {
    Detector detector;
    check(!detector.update(10 * kMs, open_guards()), "no disc read yet: real time");
    check(detector.reason() == Reason::NotLoading, "the reason is that nothing loads");
}

void test_reads_make_a_load() {
    Detector detector;
    detector.disc_read(1000 * kMs);
    check(detector.update(1010 * kMs, open_guards()), "a read just now on a silent game runs fast");
    check(detector.update(1000 * kMs + kReadWindowUs, open_guards()), "still loading at the end of the window");
    check(!detector.update(1001 * kMs + kReadWindowUs, open_guards()), "the window after the last read has passed");
    check(detector.reason() == Reason::NotLoading, "it ended because the reads stopped");
}

void test_sound_ends_it_at_once() {
    Detector detector;
    detector.disc_read(1000 * kMs);
    check(detector.update(1010 * kMs, open_guards()), "fast before the sound");
    check(detector.audio(1020 * kMs, 0), "silence while fast is dropped");
    check(detector.audio(1025 * kMs, kAudiblePeak), "a buffer at the silence threshold is dropped");
    check(!detector.audio(1030 * kMs, kAudiblePeak + 1), "a sound is never dropped");
    check(!detector.fast(), "the sound ended it at once, before the next update");
    check(detector.reason() == Reason::Sound, "the reason is the sound");
    detector.disc_read(1040 * kMs);
    check(!detector.update(1030 * kMs + kQuietUs - 1u, open_guards()), "not again until it has been quiet a while");
    check(detector.update(1030 * kMs + kQuietUs, open_guards()), "quiet long enough and still reading: fast again");
}

void test_silence_before_is_needed() {
    Detector detector;
    detector.audio(990 * kMs, 5000);
    detector.disc_read(1000 * kMs);
    check(!detector.update(1000 * kMs, open_guards()), "sound 10 ms ago keeps real time");
    check(detector.reason() == Reason::Sound, "because of the sound");
    check(!detector.audio(1001 * kMs, 0), "silence is not dropped at real time");
}

void test_guards() {
    struct Case {
        Guards guards;
        Reason reason;
        const char *message;
    };
    Guards off = open_guards();
    off.enabled = false;
    Guards buttons = open_guards();
    buttons.buttons_held = true;
    Guards movie = open_guards();
    movie.movie = true;
    Guards online = open_guards();
    online.online = true;
    Guards menu = open_guards();
    menu.menu = true;
    const Case cases[] = {{off, Reason::Disabled, "the setting off"},
                          {buttons, Reason::Buttons, "a button held"},
                          {movie, Reason::Movie, "a movie"},
                          {online, Reason::Online, "ad hoc play"},
                          {menu, Reason::Menu, "the in-game menu"}};
    for (const Case &c : cases) {
        Detector detector;
        detector.disc_read(1000 * kMs);
        check(detector.update(1001 * kMs, open_guards()), "loading before the guard");
        check(!detector.update(1002 * kMs, c.guards), c.message);
        check(detector.reason() == c.reason, c.message);
        check(!detector.audio(1003 * kMs, 0), "nothing is dropped once a guard holds");
        check(detector.update(1004 * kMs, open_guards()), "fast again once the guard lets go");
    }
}

} // namespace

int main() {
    test_nothing_read_is_not_loading();
    test_reads_make_a_load();
    test_sound_ends_it_at_once();
    test_silence_before_is_needed();
    test_guards();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "fast loading tests passed\n";
    return 0;
}
