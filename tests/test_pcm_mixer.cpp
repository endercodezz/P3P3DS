// PcmMixer: buffers placed by virtual start time, overlaps summed and
// clamped, late buffers counted, WAV header sizes patched on finish.
#include "../platform/pc/pcm_mixer.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)
std::uint32_t u32(const std::vector<std::uint8_t> &b, std::size_t at) { return b[at] | (b[at + 1] << 8) | (b[at + 2] << 16) | (static_cast<std::uint32_t>(b[at + 3]) << 24); }
std::int16_t s16(const std::vector<std::uint8_t> &b, std::size_t at) { return static_cast<std::int16_t>(b[at] | (b[at + 1] << 8)); }
}

int main(int, char **argv) {
    const std::string path = std::string(argv[0]) + ".wav";
    {
        p3p3ds::PcmMixer mixer(path);
        mixer.add(0u, std::vector<std::int16_t>(200, 1000));                     // frames 0..99
        mixer.add(1000000u * 50u / 44100u + 1u, std::vector<std::int16_t>(200, 32000)); // starts at frame 50
        mixer.add(10u * 1000000u, {7, -7});                                      // frame 441000, flushes the first second
        mixer.add(0u, {1, 1});                                                   // already written: late
        CHECK(mixer.late_frames() == 1u);
        mixer.finish();
        CHECK(mixer.frames() == 441001u);
        CHECK(mixer.peak() == 32767u);
        CHECK(mixer.clipped() == 100u);  // frames 50..99, both channels, 1000+32000 clamps
        CHECK(mixer.nonzero_frames() == 151u);
    }
    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> wav((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    CHECK(wav.size() == 44u + 441001u * 4u);
    CHECK(u32(wav, 4) == 36u + 441001u * 4u && u32(wav, 40) == 441001u * 4u && u32(wav, 24) == 44100u);
    CHECK(s16(wav, 44) == 1000 && s16(wav, 44 + 49 * 4) == 1000 && s16(wav, 44 + 50 * 4) == 32767 && s16(wav, 44 + 100 * 4) == 32000);
    CHECK(s16(wav, 44 + 441000u * 4u) == 7 && s16(wav, 44 + 441000u * 4u + 2u) == -7);
    in.close();
    std::remove(path.c_str());
    std::cout << (failures ? "FAILED" : "PASS") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
