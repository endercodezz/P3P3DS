#include "camera_probe.hpp"

#include "gpu/vulkan_renderer.hpp"
#include "hle/hle_common.hpp"

#include <string>

#include "psprecomp/guest_memory.hpp"
#include "psprecomp/runtime.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <vector>

namespace psprecomp {
// Defined in the core library; declared here rather than in the header so that
// arming a watch does not rebuild every generated unit and every overlay.
void set_write_watch(std::uint32_t address, std::uint32_t size);
} // namespace psprecomp

namespace mhp3rd::probe {
#if defined(MHP3RD_HAS_RENDERER)
using mhp3rd::active_renderer;
#endif
using mhp3rd::log_once;
namespace {

// A game does not have to keep its camera angle as a float: a 16-bit angle,
// where a whole turn is 65536, is just as likely, and read as a float it looks
// like noise. So every word is tried three ways.
enum class Kind : std::uint8_t { Float32, Int32, Int16 };

// Enough turning to carry information, and not a camera cut.
constexpr float kTurning = 0.3f;
constexpr float kCut = 40.0f;
// Ratios outside this band are noise rather than an angle in any unit: degrees
// give 1, radians 0.0175, a 16-bit angle 182.
constexpr double kSmallestRatio = 1e-3;
constexpr double kLargestRatio = 1e6;
// A whole-numbered word that moves by only a unit or two per frame cannot be
// told from a counter, because rounding swamps the difference. Asking that a
// full-speed turn move it by twenty units throws those out at the door instead
// of carrying them untested, which is how a thousand counters survived a
// four-hundred-frame hunt.
constexpr double kSmallestWholeRatio = 3.0;
// Only judge a candidate on a frame that turns enough to be worth judging by,
// and only start a hunt from one -- a gentle drift admits millions of words and
// then throws them all away, and each attempt costs a 64 MiB snapshot and three
// full scans. Left uncapped that alone took the game down to six frames a
// second.
constexpr float kWorthJudging = 1.5f;
// Frames a hunt waits before trying again after one comes to nothing.
constexpr int kCooldownFrames = 180;
// The console gets a summary; the whole list goes to a file, because a set
// that stops shrinking at a thousand is still small enough to read through and
// far too big to print every frame.
constexpr std::size_t kPrintable = 64u;
// Per kind, not overall: a single cap ran out during the float pass and the
// int16 pass never ran at all. Kept small on purpose -- a candidate costs
// thirty-two bytes and a read every frame, and three million of them per kind
// is most of a gigabyte and a slideshow on an eight-gigabyte machine. An
// admission worth having collapses to hundreds within a few frames anyway, so
// a kind that overflows this had nothing to say and is dropped whole rather
// than truncated.
constexpr std::size_t kMostPerKind = 400u * 1000u;
// Once a hunt is down to this few, the best of them is worth watching: the guest
// code that writes it is what the hunt was really after, and watching it in the
// same run needs no assumption that the address still means this tomorrow.
constexpr std::size_t kWorthWatching = 6u;
// A hunt that has not collapsed by now was started from a movement too weak to
// discriminate; carrying it costs a read per candidate per frame for nothing.
constexpr std::uint64_t kCollapseBy = 4u;
constexpr std::size_t kCollapsedTo = 20u * 1000u;
// One disagreeing frame is not proof. The filter's knee, where the camera goes
// from driven to coasting, moved every survivor of one hunt out of step for a
// single frame and threw all of them away.
constexpr int kStrikes = 3;

// A camera can be kept either way round: as an angle, which moves by the turn
// each frame, or as the turn itself, which *is* the rate the filter carries.
// The second is the one worth having, since scaling it is the whole point.
// A rate may also be read a frame before it shows up in the matrix. That is a
// question for the filter, which accepts either alignment, rather than for
// admission, where a third shape would let in every word in memory.
enum class Shape : std::uint8_t { Angle, Rate };

struct Candidate {
    std::uint32_t offset{};
    Kind kind{};
    Shape shape{};
    double ratio{};     // units of this word per degree of camera turn
    double previous{};
    std::uint8_t strikes{};
};

// One hunt per thing the camera can be asked about. The vertical direction is
// the harder half of #106 -- measured as one-shot commands rather than an axis
// -- so it gets the same treatment as the turn rather than an assumption.
struct Hunt {
    const char *name{};
    bool armed{};
    bool have_snapshot{};
    float snapshot_signal{};
    int attempts{};
    int cooldown{};
    bool watching{};
    std::uint64_t frames{};
    std::vector<std::uint8_t> snapshot;
    std::vector<Candidate> candidates;
};

struct Probe {
    bool started{};
    Hunt yaw{"yaw"};
    Hunt pitch{"pitch"};
    // Words holding one of the camera's own basis numbers. Three hunts have now
    // shown the camera is not kept as an angle, and a rotation's entries are
    // sines and cosines of the yaw, which hold no fixed ratio to it -- so a
    // test built on ratios can never admit them. This one asks the only
    // question that fits: does this word simply *equal* one of the nine numbers
    // the camera's rotation is made of?
    std::vector<std::uint32_t> basis;
    std::vector<std::uint8_t> basis_entry;
    std::vector<std::uint8_t> basis_strikes;
    bool basis_started{};
    std::uint64_t basis_frames{};
    bool basis_watching{};
    float previous_pitch{};
    bool have_pitch{};
};

Probe &probe() {
    static Probe value;
    return value;
}

double read_as(const std::uint8_t *base, std::uint32_t offset, Kind kind) {
    if (kind == Kind::Float32) {
        float value = 0.0f;
        std::memcpy(&value, base + offset, sizeof(value));
        return std::isfinite(value) ? static_cast<double>(value) : 0.0;
    }
    if (kind == Kind::Int32) {
        std::int32_t value = 0;
        std::memcpy(&value, base + offset, sizeof(value));
        return static_cast<double>(value);
    }
    std::int16_t value = 0;
    std::memcpy(&value, base + offset, sizeof(value));
    return static_cast<double>(value);
}

// A whole word of slack covers the rounding of an integer angle.
double slack_of(Kind kind, double expected) {
    return 0.03 * std::fabs(expected) + (kind == Kind::Float32 ? 1e-5 : 1.5);
}

// MHP3RD_FIND_CAMERA_OUT names a file the surviving list is rewritten into
// whenever it changes, so a run can be read without restarting the game.
void write_list(const Hunt &h, std::uint32_t base, std::ofstream &out);

const char *name_of(Kind kind) {
    return kind == Kind::Float32 ? "float" : kind == Kind::Int32 ? "int32" : "int16";
}

const char *shape_of(Shape shape) { return shape == Shape::Angle ? "angle" : "rate"; }

bool plausible(Kind kind, double ratio) {
    const double size_of = std::fabs(ratio);
    if (!std::isfinite(ratio) || size_of < kSmallestRatio || size_of > kLargestRatio) return false;
    return kind == Kind::Float32 || size_of >= kSmallestWholeRatio;
}

void write_list(const Hunt &h, std::uint32_t base, std::ofstream &out) {
    out << "# " << h.name << ": " << h.candidates.size() << " words still tracking, after " << h.frames
        << " moving frames\n";
    out << std::setprecision(10);
    for (const Candidate &c : h.candidates)
        out << h.name << " " << name_of(c.kind) << " " << shape_of(c.shape) << " 0x" << std::hex
            << (base + c.offset) << std::dec << " value=" << c.previous << " per_degree=" << c.ratio << "\n";
}

void write_lists(const Probe &p, std::uint32_t base) {
    const char *path = std::getenv("MHP3RD_FIND_CAMERA_OUT");
    if (path == nullptr || *path == '\0') return;
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    write_list(p.yaw, base, out);
    write_list(p.pitch, base, out);
}

void admit(Hunt &p, const std::uint8_t *ram, std::uint32_t size, float before_signal, float turn) {
    const auto try_kind = [&](Kind kind, std::uint32_t stride, std::uint32_t width) {
        const std::size_t before_kind = p.candidates.size();
        const std::size_t ceiling = before_kind + kMostPerKind;
        bool overflowed = false;
        for (std::uint32_t offset = 0; offset + width <= size; offset += stride) {
            if (p.candidates.size() >= ceiling) {
                overflowed = true;
                break;
            }
            const double before = read_as(p.snapshot.data(), offset, kind);
            const double now = read_as(ram, offset, kind);
            // An angle moves by the turn; a rate simply is the turn.
            const double angle_ratio = (now - before) / static_cast<double>(turn);
            if (now != before && plausible(kind, angle_ratio))
                p.candidates.push_back({offset, kind, Shape::Angle, angle_ratio, now});
            // A rate is the movement, so it has to keep the same proportion to
            // it on both frames. Asking that at the door is what keeps this
            // from admitting every non-zero word in sixty-four megabytes.
            const double rate_ratio = now / static_cast<double>(turn);
            const double was_ratio = before / static_cast<double>(before_signal);
            if (now != 0.0 && before != 0.0 && plausible(kind, rate_ratio) &&
                std::fabs(rate_ratio - was_ratio) <= 0.05 * std::fabs(rate_ratio))
                p.candidates.push_back({offset, kind, Shape::Rate, rate_ratio, now});
        }
        if (overflowed) {
            // Too many to be a signal; keeping a truncated prefix would only
            // hide whichever kind came after it.
            p.candidates.resize(before_kind);
            std::cout << "[find-camera] " << p.name << ": too many " << name_of(kind)
                      << " words matched to mean anything; that kind is dropped for this attempt\n";
        }
    };
    try_kind(Kind::Float32, 4u, 4u);
    try_kind(Kind::Int32, 4u, 4u);
    try_kind(Kind::Int16, 2u, 2u);
}

void step(Hunt &h, const std::uint8_t *ram, std::uint32_t size, float signal, std::uint32_t base) {
    if (h.cooldown > 0) {
        --h.cooldown;
        return;
    }
    // Judging wants any real movement; starting wants enough of it to be worth
    // a scan.
    const float least = h.armed ? kTurning : kWorthJudging;
    const bool moving = std::fabs(signal) >= least && std::fabs(signal) <= kCut;
    if (!moving) {
        if (!h.armed) h.have_snapshot = false;
        return;
    }
    if (!h.armed && !h.have_snapshot) {
        h.snapshot.assign(ram, ram + size);
        h.snapshot_signal = signal;
        h.have_snapshot = true;
        return;
    }
    if (!h.armed) {
        h.candidates.clear();
        admit(h, ram, size, h.snapshot_signal, signal);
        h.armed = true;
        h.frames = 1u;
        // The snapshot has done its work; 64 MiB is worth giving back.
        h.snapshot.clear();
        h.snapshot.shrink_to_fit();
        h.have_snapshot = false;
        std::cout << "[find-camera] " << h.name << " attempt " << h.attempts << " (" << signal << " deg): "
                  << h.candidates.size() << " candidates\n";
        return;
    }

    std::vector<Candidate> kept;
    kept.reserve(h.candidates.size());
    const bool judge = std::fabs(signal) >= kWorthJudging;
    for (Candidate c : h.candidates) {
        const double now = read_as(ram, c.offset, c.kind);
        if (!judge) {
            c.previous = now;  // too gentle a move to tell anything from
            kept.push_back(c);
            continue;
        }
        const double expected = c.ratio * static_cast<double>(signal);
        // A rate may be read a frame before it reaches the matrix, so either
        // alignment counts as agreement.
        const bool agrees =
            c.shape == Shape::Angle
                ? std::fabs((now - c.previous) - expected) <= slack_of(c.kind, expected)
                : (std::fabs(now - expected) <= slack_of(c.kind, expected) ||
                   std::fabs(c.previous - expected) <= slack_of(c.kind, expected));
        if (!agrees) {
            if (++c.strikes >= kStrikes) continue;
        } else if (c.strikes != 0u) {
            --c.strikes;  // it came back into step, so forgive the earlier frame
        }
        c.previous = now;
        kept.push_back(c);
    }
    const bool thinned = kept.size() != h.candidates.size();
    h.candidates.swap(kept);
    h.candidates.shrink_to_fit();
    ++h.frames;
    if (h.frames >= kCollapseBy && h.candidates.size() > kCollapsedTo) {
        std::cout << "[find-camera] " << h.name << ": " << h.candidates.size() << " still standing after "
                  << h.frames << " frames, so that movement could not tell them apart; starting over\n";
        h.candidates.clear();
        h.candidates.shrink_to_fit();
    }

    // The point of narrowing is to get somewhere worth watching.
    if (!h.watching && !h.candidates.empty() && h.candidates.size() <= kWorthWatching) {
        const Candidate &best = h.candidates.front();
        std::cout << "[find-camera] " << h.name << ": watching guest writes to 0x" << std::hex
                  << (base + best.offset) << std::dec << " (" << name_of(best.kind) << " "
                  << shape_of(best.shape) << ")\n";
        psprecomp::set_write_watch(base + best.offset, best.kind == Kind::Int16 ? 2u : 4u);
        h.watching = true;
    }
    if (thinned) {
        std::cout << "[find-camera] " << h.name << " after " << h.frames << " moving frames (" << signal
                  << " deg): " << h.candidates.size() << " left\n";
        const std::streamsize precision = std::cout.precision();
        std::cout << std::setprecision(8);
        if (!h.candidates.empty() && h.candidates.size() <= kPrintable)
            for (const Candidate &c : h.candidates)
                std::cout << "[find-camera]   " << h.name << " " << name_of(c.kind) << " " << shape_of(c.shape)
                          << " at 0x" << std::hex << (base + c.offset) << std::dec << " value=" << c.previous
                          << " per-degree=" << c.ratio << "\n";
        std::cout.precision(precision);
    }
    if (h.candidates.empty() && h.attempts < 200) {
        h.armed = false;
        h.have_snapshot = false;
        h.cooldown = kCooldownFrames;
        ++h.attempts;
        std::cout << "[find-camera] " << h.name << ": that one said nothing; waiting for another\n";
    }
}

} // namespace

// MHP3RD_POKE_FLOAT=0xADDRESS:VALUE[,0xADDRESS:VALUE...] writes floats into
// guest memory once per frame. It is how a guess about a constant is turned
// into a measurement: write a value, watch the trace, see whether the camera
// obeys.
void poke_floats(psprecomp::Runtime &runtime) {
    static const char *text = std::getenv("MHP3RD_POKE_FLOAT");
    if (text == nullptr || *text == '\0') return;
    struct Poke {
        std::uint32_t address;
        float value;
    };
    static const std::vector<Poke> pokes = [] {
        std::vector<Poke> out;
        const char *at = text;
        while (*at != '\0') {
            char *end = nullptr;
            const unsigned long address = std::strtoul(at, &end, 0);
            if (end == at || *end != ':') break;
            at = end + 1;
            const float value = std::strtof(at, &end);
            if (end == at) break;
            out.push_back({static_cast<std::uint32_t>(address), value});
            std::cout << "[poke] 0x" << std::hex << address << std::dec << " <- " << value << " every frame\n";
            at = (*end == ',') ? end + 1 : end;
        }
        return out;
    }();
    for (const Poke &poke : pokes) {
        std::uint32_t bits = 0u;
        std::memcpy(&bits, &poke.value, sizeof(bits));
        runtime.memory().store32(poke.address, bits);
    }
}

// Writing to every address that merely held the right number is a blind write
// into whatever else happens to hold it -- live game state included -- and it
// cost a hung game and a pegged core to learn that. A poke now names which of
// the matches it means, and asking for all of them has to be said out loud.
// Returns the matches that may be written, empty if the caller has not chosen.
std::vector<std::uint32_t> chosen_places(const std::vector<std::uint32_t> &places, const char *what) {
    static const char *which = std::getenv("MHP3RD_POKE_WHICH");
    if (places.size() <= 1u) return places;
    if (which == nullptr || *which == '\0') {
        log_once(std::string("poke-which-") + what,
                 std::string("[poke] ") + what + " matches " + std::to_string(places.size()) +
                     " places; set MHP3RD_POKE_WHICH to an index (0-based) to write one of them, or to"
                     " 'all' to write every one, which will also write whatever else holds that value");
        return {};
    }
    if (std::strcmp(which, "all") == 0) {
        log_once(std::string("poke-all-") + what,
                 std::string("[poke] writing all ") + std::to_string(places.size()) + " places that match " +
                     what + "; anything else holding that value is being overwritten too");
        return places;
    }
    const std::size_t index = static_cast<std::size_t>(std::strtoul(which, nullptr, 0));
    if (index >= places.size()) {
        log_once(std::string("poke-range-") + what,
                 std::string("[poke] MHP3RD_POKE_WHICH=") + which + " is past the " +
                     std::to_string(places.size()) + " places that match " + what);
        return {};
    }
    return {places[index]};
}

// MHP3RD_FIND_FLOAT=VALUE lists every word in guest memory holding exactly that
// float, and MHP3RD_POKE_FOUND=VALUE then writes a different value into all of
// them every frame. A constant that the game copies out of its executable at
// start-up cannot be changed where it was found, only where it was copied to,
// and this finds the copies and tests them in one run.
void find_and_poke_copies(psprecomp::Runtime &runtime) {
    static const char *wanted_text = std::getenv("MHP3RD_FIND_FLOAT");
    if (wanted_text == nullptr || *wanted_text == '\0') return;
    static std::vector<std::uint32_t> copies;
    static std::uint64_t scans = 0u;
    static std::uint64_t frames = 0u;
    static const float wanted = std::strtof(wanted_text, nullptr);
    static const char *poke_text = std::getenv("MHP3RD_POKE_FOUND");

    psprecomp::GuestMemory &memory = runtime.memory();
    const std::uint32_t base = psprecomp::GuestMemory::kPhysicalBase;
    const std::uint32_t size = memory.size();
    // A constant may only be copied once the code that uses it is loaded, and
    // for this game that is an overlay swap away, so look again now and then
    // rather than once at the first frame.
    const bool rescan = (frames++ % 900u) == 0u;
    if (rescan) {
        const std::uint8_t *ram = memory.raw_pointer(base, size);
        if (ram == nullptr) return;
        std::uint32_t wanted_bits = 0u;
        std::memcpy(&wanted_bits, &wanted, sizeof(wanted_bits));
        copies.clear();
        for (std::uint32_t offset = 0; offset + 4u <= size; offset += 4u) {
            std::uint32_t bits = 0u;
            std::memcpy(&bits, ram + offset, sizeof(bits));
            if (bits == wanted_bits) copies.push_back(base + offset);
        }
        std::cout << "[find-float] scan " << scans++ << ": " << wanted << " appears at " << copies.size()
                  << " places";
        for (std::uint32_t address : copies) std::cout << " 0x" << std::hex << address << std::dec;
        std::cout << "\n";
    }
    if (poke_text == nullptr || *poke_text == '\0' || copies.empty()) return;
    static const float replacement = std::strtof(poke_text, nullptr);
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &replacement, sizeof(bits));
    for (std::uint32_t address : chosen_places(copies, "that float")) memory.store32(address, bits);
}

// MHP3RD_FIND_INT32=N lists every word holding exactly that whole number, and
// MHP3RD_POKE_INT32=M writes M into all of them each frame. A game that works
// in 16-bit angles keeps its camera steps as whole numbers, not floats, so the
// float tools above cannot see them.
void find_and_poke_int32(psprecomp::Runtime &runtime) {
    static const char *wanted_text = std::getenv("MHP3RD_FIND_INT32");
    static const bool half = std::getenv("MHP3RD_FIND_INT16_WIDE") != nullptr;
    if (wanted_text == nullptr || *wanted_text == '\0') return;
    static const std::int32_t wanted = static_cast<std::int32_t>(std::strtol(wanted_text, nullptr, 0));
    static const char *poke_text = std::getenv("MHP3RD_POKE_INT32");
    static std::vector<std::uint32_t> places;
    static std::uint64_t frames = 0u;
    static std::uint64_t scans = 0u;

    psprecomp::GuestMemory &memory = runtime.memory();
    const std::uint32_t base = psprecomp::GuestMemory::kPhysicalBase;
    const std::uint32_t size = memory.size();
    if ((frames++ % 900u) == 0u) {
        const std::uint8_t *ram = memory.raw_pointer(base, size);
        if (ram == nullptr) return;
        places.clear();
        // A game that works in 16-bit angles keeps its steps in 16-bit fields,
        // and a 32-bit scan only finds those whose upper half happens to be
        // zero -- which is a small and misleading subset.
        const std::uint32_t stride = half ? 2u : 4u;
        const std::uint32_t width = half ? 2u : 4u;
        for (std::uint32_t offset = 0; offset + width <= size; offset += stride) {
            std::int32_t value = 0;
            if (half) {
                std::int16_t narrow = 0;
                std::memcpy(&narrow, ram + offset, sizeof(narrow));
                value = narrow;
            } else {
                std::memcpy(&value, ram + offset, sizeof(value));
            }
            if (value == wanted) places.push_back(base + offset);
        }
        std::cout << "[find-int32] scan " << scans++ << ": " << wanted << " at " << places.size() << " places";
        for (std::size_t i = 0; i < places.size() && i < 24u; ++i)
            std::cout << " 0x" << std::hex << places[i] << std::dec;
        std::cout << "\n";
    }
    if (poke_text == nullptr || *poke_text == '\0' || places.empty()) return;
    static const std::int32_t replacement = static_cast<std::int32_t>(std::strtol(poke_text, nullptr, 0));
    for (std::uint32_t address : chosen_places(places, "that whole number")) {
        if (half)
            memory.store16(address, static_cast<std::uint16_t>(replacement));
        else
            memory.store32(address, static_cast<std::uint32_t>(replacement));
    }
}

// MHP3RD_FIND_STEP=N finds the camera's own yaw: the one 16-bit field whose
// value changes by exactly +N or -N from one frame to the next while the camera
// turns. Reading the game's code gives N as 1150, which is 1150/65536 of a turn
// -- 6.317139 degrees a frame, the rate measured from outside -- so this asks a
// question with a single right answer rather than a proportion that many words
// can satisfy by accident.
void find_step_field(psprecomp::Runtime &runtime, float turn) {
    static const char *step_text = std::getenv("MHP3RD_FIND_STEP");
    if (step_text == nullptr || *step_text == '\0') return;
    static const int step = static_cast<int>(std::strtol(step_text, nullptr, 0));
    static std::vector<std::uint8_t> before;   // held only until the first compare
    static std::vector<std::uint32_t> kept;
    static std::vector<std::int16_t> previous; // and only for the survivors after that
    static bool armed = false;
    static std::uint64_t rounds = 0u;


    psprecomp::GuestMemory &memory = runtime.memory();
    const std::uint32_t base = psprecomp::GuestMemory::kPhysicalBase;
    const std::uint32_t size = memory.size();
    const std::uint8_t *ram = memory.raw_pointer(base, size);
    if (ram == nullptr) return;
    const auto narrow = [](const std::uint8_t *at) {
        std::int16_t value = 0;
        std::memcpy(&value, at, sizeof(value));
        return value;
    };

    // The first full scan needs a real turn to compare across; the survivors
    // are then checked every frame, because the field advances a step per frame
    // and a skipped frame would make it look like it jumped two.
    if (!armed && std::fabs(turn) < 1.5f) return;
    if (kept.empty() && rounds == 0u && std::fabs(turn) < 1.5f) return;
    if (!armed) {
        // One copy of guest memory, once, and it is handed back below.
        before.assign(ram, ram + size);
        armed = true;
        return;
    }
    if (kept.empty() && rounds == 0u) {
        for (std::uint32_t offset = 0; offset + 2u <= size; offset += 2u) {
            // A 16-bit angle wraps, so the difference is taken as one too.
            const std::int16_t moved =
                static_cast<std::int16_t>(narrow(ram + offset) - narrow(before.data() + offset));
            if (moved == step || moved == -step) {
                kept.push_back(base + offset);
                previous.push_back(narrow(ram + offset));
            }
        }
        before.clear();
        before.shrink_to_fit();  // 64 MiB is worth giving back at once
        ++rounds;
        std::cout << "[find-step] " << kept.size() << " fields moved by exactly " << step;
        if (kept.size() <= 24u)
            for (std::uint32_t address : kept) std::cout << " 0x" << std::hex << address << std::dec;
        std::cout << "\n";
        return;
    }
    // From here only the survivors are touched, which costs nothing.
    std::vector<std::uint32_t> still;
    std::vector<std::int16_t> still_previous;
    for (std::size_t i = 0; i < kept.size(); ++i) {
        const std::int16_t now = narrow(ram + (kept[i] - base));
        const std::int16_t moved = static_cast<std::int16_t>(now - previous[i]);
        if (moved == step || moved == -step || moved == 0) {
            still.push_back(kept[i]);
            still_previous.push_back(now);
        }
    }
    if (still.size() != kept.size()) {
        kept.swap(still);
        previous.swap(still_previous);
        std::cout << "[find-step] " << kept.size() << " left after " << ++rounds << " turning frames";
        if (kept.size() <= 24u)
            for (std::uint32_t address : kept) std::cout << " 0x" << std::hex << address << std::dec;
        std::cout << "\n";
    } else {
        previous.swap(still_previous);
    }
}

void camera_frame(psprecomp::Runtime &runtime, std::uint32_t view_matrix_source) {
    poke_floats(runtime);

    find_and_poke_int32(runtime);
    find_and_poke_copies(runtime);
#if defined(MHP3RD_HAS_RENDERER)
    (void)view_matrix_source;
    const gpu::VulkanRenderer *renderer = active_renderer();
    if (renderer == nullptr || !renderer->available()) return;
    const gpu::CameraReading reading = renderer->camera();
    if (!reading.valid) return;
    // Each of these switches on independently: burying one behind another's
    // means a run quietly does nothing, which cost a whole trip to a quest.
    find_step_field(runtime, reading.turn);
    static const bool enabled = std::getenv("MHP3RD_FIND_CAMERA") != nullptr;
    if (!enabled) return;

    psprecomp::GuestMemory &memory = runtime.memory();
    const std::uint32_t base = psprecomp::GuestMemory::kPhysicalBase;
    const std::uint32_t size = memory.size();
    const std::uint8_t *ram = memory.raw_pointer(base, size);
    if (ram == nullptr) return;

    Probe &p = probe();
    if (!p.started) {
        std::cout << "[find-camera] watching " << (size / (1024u * 1024u)) << " MiB of guest RAM from 0x"
                  << std::hex << base << std::dec << ", as float, int32 and int16\n";
        p.started = true;
    }

    // Both halves of the camera get the same treatment, in one visit to a
    // quest, because getting into one is the expensive part.
    const float pitch_change = p.have_pitch ? reading.pitch - p.previous_pitch : 0.0f;
    p.previous_pitch = reading.pitch;
    p.have_pitch = true;

    // --- words that hold one of the camera's basis numbers -------------------
    // Only the nine rotation entries; the translation is a position, which the
    // earlier hunts already chased.
    static const int kRotation[9] = {0, 1, 2, 4, 5, 6, 8, 9, 10};
    const auto basis_of = [&](int which) { return reading.view[static_cast<std::size_t>(kRotation[which])]; };
    // A basis entry near zero is matched by every zero word in sixty-four
    // megabytes, and there are millions of those; only the entries with real
    // magnitude carry information.
    constexpr float kMeaningful = 0.05f;
    const auto holds = [&](std::uint32_t offset, int which) {
        const float wanted = basis_of(which);
        if (std::fabs(wanted) < kMeaningful) return false;
        std::uint32_t stored = 0u;
        std::memcpy(&stored, ram + offset, sizeof(stored));
        float value = 0.0f;
        std::memcpy(&value, &stored, sizeof(value));
        return std::isfinite(value) && std::fabs(value - wanted) <= std::fabs(wanted) * 1e-5f;
    };
    // A camera pointing along an axis makes every zero in memory look like a
    // basis entry, so only judge while it is turned well away from one.
    const bool basis_usable = std::fabs(reading.turn) >= kTurning && std::fabs(reading.turn) <= kCut;
    if (basis_usable && !p.basis_started) {
        for (std::uint32_t offset = 0; offset + 4u <= size; offset += 4u)
            for (int which = 0; which < 9; ++which)
                if (holds(offset, which)) {
                    p.basis.push_back(offset);
                    p.basis_entry.push_back(static_cast<std::uint8_t>(which));
                    p.basis_strikes.push_back(0u);
                    break;
                }
        p.basis_started = true;
        p.basis_frames = 1u;
        std::cout << "[find-camera] basis: " << p.basis.size() << " words hold one of the camera's own numbers\n";
    } else if (basis_usable && !p.basis.empty()) {
        // The game keeps no standing copy of its matrix: it writes one into a
        // display list it double-buffers, so a given address carries the camera
        // on alternate frames. Insisting on a match every frame threw away all
        // twenty-five survivors of one run at once.
        std::vector<std::uint32_t> kept;
        std::vector<std::uint8_t> kept_entry;
        std::vector<std::uint8_t> kept_strikes;
        for (std::size_t i = 0; i < p.basis.size(); ++i) {
            std::uint8_t strikes = p.basis_strikes[i];
            if (holds(p.basis[i], p.basis_entry[i])) {
                if (strikes != 0u) --strikes;
            } else if (++strikes >= 4u) {
                continue;
            }
            kept.push_back(p.basis[i]);
            kept_entry.push_back(p.basis_entry[i]);
            kept_strikes.push_back(strikes);
        }
        const bool thinned = kept.size() != p.basis.size();
        p.basis.swap(kept);
        p.basis_entry.swap(kept_entry);
        p.basis_strikes.swap(kept_strikes);
        ++p.basis_frames;
        if (thinned) {
            std::cout << "[find-camera] basis after " << p.basis_frames << " frames: " << p.basis.size()
                      << " left\n";
            if (p.basis.size() <= kPrintable)
                for (std::size_t i = 0; i < p.basis.size(); ++i)
                    std::cout << "[find-camera]   basis entry " << int(p.basis_entry[i]) << " at 0x" << std::hex
                              << (base + p.basis[i]) << std::dec << "\n";
        }
        if (!p.basis_watching && !p.basis.empty() && p.basis.size() <= kWorthWatching) {
            std::cout << "[find-camera] basis: watching guest writes to 0x" << std::hex << (base + p.basis[0])
                      << std::dec << "\n";
            psprecomp::set_write_watch(base + p.basis[0], 4u);
            p.basis_watching = true;
        }
    }

    step(p.yaw, ram, size, reading.turn, base);
    step(p.pitch, ram, size, pitch_change, base);
    if (p.yaw.frames % 200u == 0u || p.pitch.frames % 200u == 0u) write_lists(p, base);

#else
    (void)runtime;
    (void)view_matrix_source;
#endif
}

} // namespace mhp3rd::probe
