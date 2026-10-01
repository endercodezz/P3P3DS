// Host input: script parsing/sampling, XInput mapping, and sceCtrl
// ReadBufferPositive returning the scripted buttons at the scripted vblank.
#include "../platform/pc/host_input.hpp"
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/input.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <iostream>
#include <stdexcept>

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)
using namespace p3p3ds::input;

bool throws(const char *text) {
    try { (void)InputScript::parse(text); } catch (const std::runtime_error &) { return true; }
    return false;
}
} // namespace

int main() {
    // Names and masks (pspctrl.h values).
    CHECK(button_from_name("start") == 0x8u && button_from_name("CROSS") == 0x4000u && !button_from_name("HOME"));
    CHECK(buttons_from_text("UP+CROSS") == 0x4010u && buttons_from_text("none") == 0u && buttons_from_text("-") == 0u);
    CHECK(!buttons_from_text("UP+FOO"));

    // Holds, explicit states, sticks, comments.
    auto script = InputScript::parse(
        "# title\n"
        "100 START 6      # tap\n"
        "200 UP+CROSS\n"
        "250 none\n"
        "300 stick 255 128\n"
        "400 CIRCLE 2\n"
        "402 SQUARE\n");
    CHECK(script.sample(99).buttons == 0u);
    CHECK(script.sample(100).buttons == button::Start && script.sample(105).buttons == button::Start);
    CHECK(script.sample(106).buttons == 0u);
    CHECK(script.sample(200).buttons == (button::Up | button::Cross) && script.sample(249).buttons == (button::Up | button::Cross));
    CHECK(script.sample(250).buttons == 0u);
    CHECK(script.sample(299).analog_x == 128 && script.sample(300).analog_x == 255 && script.sample(300).analog_y == 128);
    CHECK(script.sample(401).buttons == button::Circle);
    CHECK(script.sample(402).buttons == button::Square); // explicit state wins over the release at 402
    CHECK(throws("10 START\n5 CROSS\n"));      // out of order
    CHECK(throws("10 JUMP\n"));                // unknown button
    CHECK(throws("x START\n"));                // bad vblank
    CHECK(throws("10 stick 300 0\n"));         // range
    CHECK(throws("10 START 0\n"));             // zero duration
    CHECK(throws("10 START 2 extra\n"));       // trailing token

    // XInput -> PSP.
    p3p3ds::pc::xinput::Gamepad g{};
    g.buttons = p3p3ds::pc::xinput::A | p3p3ds::pc::xinput::Start | p3p3ds::pc::xinput::DpadLeft;
    g.thumb_lx = 32767; g.thumb_ly = 32767;
    auto pad = p3p3ds::pc::map_xinput(g);
    CHECK(pad.buttons == (button::Cross | button::Start | button::Left));
    CHECK(pad.analog_x == 255 && pad.analog_y == 0);
    g = {}; g.right_trigger = 255; g.thumb_ly = -32768;
    pad = p3p3ds::pc::map_xinput(g);
    CHECK(pad.buttons == button::RTrigger && pad.analog_x == 128 && pad.analog_y == 255);

    // sceCtrlReadBufferPositive through the HLE: blocks to the next vblank,
    // then reports the script state for that vblank.
    {
        psprecomp::Runtime rt;
        p3p3ds::KernelState k;
        p3p3ds::hle::register_all_hle_modules(rt, k);
        k.threads().set_import_stubs({0x08A00000u});
        k.threads().init_root_thread("root", 0x08804000u, 0x09FFFF00u, 0u);
        k.input().source = std::make_shared<InputScript>(InputScript::parse("1 START 1\n3 CROSS\n"));
        constexpr std::uint32_t kData = 0x08900000u;
        std::vector<std::uint32_t> seen;
        for (int read = 0; read < 4; ++read) {
            // The first call blocks until the next vblank (the lone thread
            // idles the virtual clock to the deadline); the retry at the stub
            // then returns the sample.
            for (int attempt = 0; attempt < 4; ++attempt) {
                auto &ctx = rt.cpu();
                ctx.gpr[4] = kData; ctx.gpr[5] = 1u; ctx.gpr[31] = 0x08A00100u; ctx.pc = 0x08A00000u;
                rt.invoke_import("sceCtrl", 0x1F803938u, ctx);
                if (k.input().pending_reads.empty()) break;
            }
            seen.push_back(rt.memory().load32(kData + 4u));
        }
        CHECK(seen.size() == 4u);
        CHECK(seen[0] == button::Start);   // vblank 1
        CHECK(seen[1] == 0u);              // vblank 2 (hold of 1 released)
        CHECK(seen[2] == button::Cross);   // vblank 3
        CHECK(seen[3] == button::Cross);   // vblank 4
    }
    std::cout << (failures ? "FAILED" : "PASS") << " (" << failures << " failures)\n";
    return failures ? 1 : 0;
}
