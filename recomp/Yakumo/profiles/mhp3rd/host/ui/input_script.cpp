#include "ui/input_script.hpp"

#include "ui/layer.hpp"

#include "gpu/vulkan_renderer.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace mhp3rd::ui::script {
namespace {

// Frames a scripted button stays down: ImGui samples the pad once per frame.
constexpr std::uint64_t kHoldFrames = 4u;

struct Step {
    std::uint64_t frame{};
    std::string action;
    std::string argument;
};

struct State {
    bool attached{};
    std::deque<Step> steps;
    std::uint64_t frame{};
    SDL_Joystick *pad{};
    // Releases due later: frame, key or pad buttons.
    struct Release {
        std::uint64_t frame{};
        SDL_Keycode key{};
        std::vector<SDL_GamepadButton> buttons;
        std::uint8_t mouse_button{};
    };
    std::vector<Release> releases;
    // Strings handed to SDL events must outlive them.
    std::deque<std::string> strings;
    // MHP3RD_INPUT_LIVE: a file whose appended lines are read as they come.
    std::string live_path;
    std::streamoff live_offset{};
};

State &state() {
    static State value;
    return value;
}

std::string trim(const std::string &text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t") - first + 1u);
}

void push_key(SDL_Keycode key, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.timestamp = SDL_GetTicksNS();
    event.key.windowID = SDL_GetWindowID(Layer::get().renderer().window());
    event.key.key = key;
    event.key.scancode = SDL_GetScancodeFromKey(key, nullptr);
    event.key.down = down;
    SDL_PushEvent(&event);
    // A pushed event does not change SDL's keyboard snapshot, which the game's
    // bindings read, so the key is handed to the renderer as well.
    Layer::get().renderer().set_scripted_key(static_cast<int>(event.key.scancode), down);
}

void push_mouse_button(std::uint8_t button, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.timestamp = SDL_GetTicksNS();
    event.button.windowID = SDL_GetWindowID(Layer::get().renderer().window());
    event.button.which = gpu::kScriptedMouse;
    event.button.button = button;
    event.button.down = down;
    event.button.clicks = 1u;
    SDL_PushEvent(&event);
}

// "NAME N": the name, and N frames if the last word is a number.
std::pair<std::string, std::uint64_t> name_and_frames(const std::string &argument, std::uint64_t fallback) {
    const auto space = argument.find_last_of(' ');
    if (space == std::string::npos) return {argument, fallback};
    const std::string last = argument.substr(space + 1u);
    if (last.empty() || last.find_first_not_of("0123456789") != std::string::npos) return {argument, fallback};
    return {argument.substr(0, space), std::max<std::uint64_t>(1u, std::strtoull(last.c_str(), nullptr, 10))};
}

bool mouse_step(const std::string &action) { return action == "mouse" || action == "click"; }

void run(const Step &step) {
    State &s = state();
    std::cout << "[script] frame " << s.frame << ": " << step.action << " " << step.argument << std::endl;
    if (step.action == "key") {
        const auto [name, frames] = name_and_frames(step.argument, kHoldFrames);
        const SDL_Keycode key = SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN) {
            std::cout << "[script] unknown key " << step.argument << std::endl;
            return;
        }
        push_key(key, true);
        s.releases.push_back({s.frame + frames, key, {}});
    } else if (step.action == "mouse") {
        float dx = 0.0f;
        float dy = 0.0f;
        std::stringstream(step.argument) >> dx >> dy;
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.timestamp = SDL_GetTicksNS();
        event.motion.windowID = SDL_GetWindowID(Layer::get().renderer().window());
        event.motion.which = gpu::kScriptedMouse;
        event.motion.xrel = dx;
        event.motion.yrel = dy;
        SDL_PushEvent(&event);
    } else if (step.action == "click") {
        const auto [name, frames] = name_and_frames(step.argument, kHoldFrames);
        static const std::pair<const char *, std::uint8_t> kButtons[] = {
            {"left", SDL_BUTTON_LEFT}, {"middle", SDL_BUTTON_MIDDLE}, {"right", SDL_BUTTON_RIGHT},
            {"x1", SDL_BUTTON_X1},     {"x2", SDL_BUTTON_X2}};
        std::uint8_t button = 0u;
        for (const auto &[button_name, value] : kButtons)
            if (name == button_name) button = value;
        if (button == 0u) {
            std::cout << "[script] unknown mouse button " << step.argument << std::endl;
            return;
        }
        push_mouse_button(button, true);
        s.releases.push_back({s.frame + frames, SDLK_UNKNOWN, {}, button});
    } else if (step.action == "pad") {
        if (s.pad == nullptr) return;
        std::vector<SDL_GamepadButton> buttons;
        std::stringstream names(step.argument);
        std::string name;
        while (std::getline(names, name, '+')) {
            const SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name.c_str());
            if (button == SDL_GAMEPAD_BUTTON_INVALID) {
                std::cout << "[script] unknown button " << name << std::endl;
                continue;
            }
            SDL_SetJoystickVirtualButton(s.pad, button, true);
            buttons.push_back(button);
        }
        s.releases.push_back({s.frame + kHoldFrames, SDLK_UNKNOWN, buttons});
    } else if (step.action == "axis") {
        if (s.pad == nullptr) return;
        const auto space = step.argument.find(' ');
        const SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(step.argument.substr(0, space).c_str());
        const float value = space == std::string::npos ? 0.0f : std::strtof(step.argument.c_str() + space + 1u, nullptr);
        if (axis == SDL_GAMEPAD_AXIS_INVALID) {
            std::cout << "[script] unknown axis " << step.argument << std::endl;
            return;
        }
        SDL_SetJoystickVirtualAxis(s.pad, axis, static_cast<Sint16>(std::clamp(value, -1.0f, 1.0f) * 32767.0f));
    } else if (step.action == "text") {
        SDL_Event event{};
        event.type = SDL_EVENT_TEXT_INPUT;
        event.text.windowID = SDL_GetWindowID(Layer::get().renderer().window());
        event.text.text = s.strings.emplace_back(step.argument).c_str();
        SDL_PushEvent(&event);
    } else if (step.action == "drop") {
        SDL_Event event{};
        event.type = SDL_EVENT_DROP_FILE;
        event.drop.windowID = SDL_GetWindowID(Layer::get().renderer().window());
        event.drop.data = s.strings.emplace_back(step.argument).c_str();
        SDL_PushEvent(&event);
    } else if (step.action == "shot") {
        const char *dir = std::getenv("MHP3RD_SCREENSHOT_DIR");
        const std::string name = step.argument.empty() ? "frame_" + std::to_string(s.frame) : step.argument;
        Layer::get().renderer().capture_window((dir != nullptr ? std::string(dir) : std::string(".")) + "/" + name +
                                               ".bmp");
    } else if (step.action == "quit") {
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    } else {
        std::cout << "[script] unknown action " << step.action << std::endl;
    }
}

// Parses `frame:action argument`; `base` is added to the frame.
bool parse_step(std::string item, std::uint64_t base, Step &step) {
    item = trim(item);
    const auto colon = item.find(':');
    if (item.empty() || colon == std::string::npos) return false;
    step.frame = base + std::strtoull(item.substr(0, colon).c_str(), nullptr, 10);
    const std::string rest = trim(item.substr(colon + 1u));
    const auto space = rest.find(' ');
    step.action = rest.substr(0, space);
    step.argument = space == std::string::npos ? std::string{} : trim(rest.substr(space + 1u));
    return true;
}

void sort_steps(State &s) {
    std::stable_sort(s.steps.begin(), s.steps.end(),
                     [](const Step &a, const Step &b) { return a.frame < b.frame; });
}

// Reads the lines appended to the live file since the last call. Their frames
// count from now, so a line `30:pad a` presses ○ half a second after it is read.
void read_live(State &s) {
    std::ifstream file(s.live_path, std::ios::binary);
    if (!file) return;
    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size < s.live_offset) s.live_offset = 0;  // the file was replaced
    if (size == s.live_offset) return;
    file.seekg(s.live_offset);
    std::string line;
    bool added = false;
    while (std::getline(file, line)) {
        if (file.eof()) break;  // an incomplete last line: read it next time
        s.live_offset = file.tellg();
        Step step;
        if (parse_step(line, s.frame, step)) {
            s.steps.push_back(step);
            added = true;
        }
    }
    if (added) sort_steps(s);
}

void attach_pad(State &s);

} // namespace

void attach() {
    State &s = state();
    if (s.attached) return;
    s.attached = true;
    bool uses_pad = false;
    bool uses_mouse = false;
    if (const char *live = std::getenv("MHP3RD_INPUT_LIVE"); live != nullptr && *live != '\0') {
        s.live_path = live;
        // Only what is appended after start-up counts.
        std::ifstream file(s.live_path, std::ios::binary | std::ios::ate);
        if (file) s.live_offset = file.tellg();
        uses_pad = true;
        uses_mouse = true;
        std::cout << "[script] reading live input from " << s.live_path << std::endl;
    }
    if (const char *text = std::getenv("MHP3RD_INPUT_SCRIPT"); text != nullptr) {
        std::stringstream list(text);
        std::string item;
        while (std::getline(list, item, ';')) {
            Step step;
            if (!parse_step(item, 0u, step)) continue;
            uses_pad = uses_pad || step.action == "pad" || step.action == "axis";
            uses_mouse = uses_mouse || mouse_step(step.action);
            s.steps.push_back(step);
        }
        sort_steps(s);
        std::cout << "[script] " << s.steps.size() << " steps" << std::endl;
    }
    if (uses_pad) attach_pad(s);
    if (uses_mouse) Layer::get().renderer().set_scripted_input(true);
}

namespace {

void attach_pad(State &s) {
    // Scripted runs usually go on in the background, where SDL would
    // otherwise ignore the pad.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1u;
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1u;
    // The renderer gives the game this pad over a real one by its name.
    desc.name = "Yakumo input script";
    const SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    if (id == 0) {
        std::cout << "[script] cannot attach a virtual gamepad: " << SDL_GetError() << std::endl;
        return;
    }
    s.pad = SDL_OpenJoystick(id);
}

} // namespace

void tick() {
    State &s = state();
    if (s.steps.empty() && s.releases.empty() && s.live_path.empty()) return;
    ++s.frame;
    if (!s.live_path.empty() && s.frame % 10u == 0u) read_live(s);
    for (auto it = s.releases.begin(); it != s.releases.end();) {
        if (it->frame > s.frame) {
            ++it;
            continue;
        }
        if (it->key != SDLK_UNKNOWN) push_key(it->key, false);
        if (it->mouse_button != 0u) push_mouse_button(it->mouse_button, false);
        for (SDL_GamepadButton button : it->buttons) SDL_SetJoystickVirtualButton(s.pad, button, false);
        it = s.releases.erase(it);
    }
    while (!s.steps.empty() && s.steps.front().frame <= s.frame) {
        run(s.steps.front());
        s.steps.pop_front();
    }
}

} // namespace mhp3rd::ui::script
