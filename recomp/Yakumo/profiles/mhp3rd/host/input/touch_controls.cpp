#include "touch_controls.hpp"

#include <algorithm>
#include <cmath>

namespace mhp3rd::input::touch {

std::uint16_t psp_button(Control control) {
    switch (control) {
    case Control::Triangle: return 0x1000u;
    case Control::Circle: return 0x2000u;
    case Control::Cross: return 0x4000u;
    case Control::Square: return 0x8000u;
    case Control::L: return 0x0100u;
    case Control::R: return 0x0200u;
    case Control::Start: return 0x0008u;
    case Control::Select: return 0x0001u;
    default: return 0u;
    }
}

bool Circle::contains(Point p, float slack) const {
    const float dx = p.x - centre.x;
    const float dy = p.y - centre.y;
    const float r = radius * slack;
    return dx * dx + dy * dy <= r * r;
}

Layout make_layout(float width, float height, Insets insets, float size, bool dpad) {
    Layout layout;
    layout.width = width;
    layout.height = height;
    const float left = insets.left;
    const float top = insets.top;
    const float right = width - insets.right;
    const float bottom = height - insets.bottom;
    // A face button about 9 mm across on a phone held sideways: the short side
    // is what the thumbs span, whatever the screen's shape.
    const float unit = std::min(height, width * 9.0f / 16.0f) * std::clamp(size, 0.5f, 2.0f);
    const float face = unit * 0.075f;
    const float spread = face * 1.45f;
    const Point diamond{right - spread - face * 1.6f, bottom - spread - face * 1.5f};
    auto &c = layout.controls;
    c[static_cast<std::size_t>(Control::Triangle)] = {{diamond.x, diamond.y - spread}, face};
    c[static_cast<std::size_t>(Control::Circle)] = {{diamond.x + spread, diamond.y}, face};
    c[static_cast<std::size_t>(Control::Cross)] = {{diamond.x, diamond.y + spread}, face};
    c[static_cast<std::size_t>(Control::Square)] = {{diamond.x - spread, diamond.y}, face};
    const float shoulder = face * 1.05f;
    c[static_cast<std::size_t>(Control::L)] = {{left + shoulder * 1.6f, top + shoulder * 1.4f}, shoulder};
    c[static_cast<std::size_t>(Control::R)] = {{right - shoulder * 1.6f, top + shoulder * 1.4f}, shoulder};
    const float small = face * 0.55f;
    const float middle = (left + right) * 0.5f;
    c[static_cast<std::size_t>(Control::Select)] = {{middle - small * 3.0f, top + small * 1.6f}, small};
    c[static_cast<std::size_t>(Control::Menu)] = {{middle, top + small * 1.6f}, small};
    c[static_cast<std::size_t>(Control::Start)] = {{middle + small * 3.0f, top + small * 1.6f}, small};
    // The D-pad: at the left edge, half way down, clear of L above it.
    const float reach = face * 2.3f;
    layout.dpad_shown = dpad;
    layout.dpad = {{left + reach + face * 0.5f,
                    std::max(top + (bottom - top) * 0.47f, c[static_cast<std::size_t>(Control::L)].centre.y +
                                                               shoulder + reach + face * 0.4f)},
                   reach};
    layout.stick_radius = unit * 0.13f;
    layout.stick_split = left + (right - left) * 0.45f;
    return layout;
}

std::uint16_t dpad_buttons(Point offset, float reach) {
    const float length = std::hypot(offset.x, offset.y);
    if (!(reach > 0.0f) || length < reach * 0.2f) return 0u;
    // Eight sectors of 45 degrees, the first centred on right; y grows down.
    constexpr float kPi = 3.14159265f;
    float angle = std::atan2(-offset.y, offset.x);
    if (angle < 0.0f) angle += 2.0f * kPi;
    const int sector = static_cast<int>(std::floor(angle / (kPi / 4.0f) + 0.5f)) % 8;
    constexpr std::uint16_t kUp = 0x10u, kRight = 0x20u, kDown = 0x40u, kLeft = 0x80u;
    constexpr std::uint16_t kSectors[8] = {kRight,        kRight | kUp,  kUp,           kUp | kLeft,
                                           kLeft,         kLeft | kDown, kDown,         kDown | kRight};
    return kSectors[sector];
}

Point stick_deflection(Point offset, float reach, float dead_zone) {
    const float length = std::hypot(offset.x, offset.y);
    if (!(reach > 0.0f) || length <= reach * dead_zone) return {};
    const float amount = std::min((length / reach - dead_zone) / (1.0f - dead_zone), 1.0f);
    return {offset.x / length * amount, offset.y / length * amount};
}

std::optional<Control> Controls::control_at(Point at, bool face_only) const {
    // A little slack around each control: a thumb is wider than its aim.
    const std::size_t last = face_only ? static_cast<std::size_t>(Control::Square) : kControls - 1u;
    std::optional<Control> best;
    float best_distance = 0.0f;
    for (std::size_t i = 0; i <= last; ++i) {
        const Circle &circle = layout_.controls[i];
        if (!circle.contains(at, 1.2f)) continue;
        const float distance = std::hypot(at.x - circle.centre.x, at.y - circle.centre.y) / circle.radius;
        if (!best || distance < best_distance) {
            best = static_cast<Control>(i);
            best_distance = distance;
        }
    }
    return best;
}

Controls::Finger *Controls::find(std::uint64_t id) {
    for (Finger &finger : fingers_)
        if (finger.used && finger.id == id) return &finger;
    return nullptr;
}

void Controls::finger_down(std::uint64_t id, Point at) {
    if (find(id) != nullptr) finger_up(id);
    Finger *slot = nullptr;
    for (Finger &finger : fingers_)
        if (!finger.used) {
            slot = &finger;
            break;
        }
    if (slot == nullptr) return;
    *slot = Finger{id, true, Role::None, Control::Count, false, at};
    if (const std::optional<Control> control = control_at(at, false)) {
        slot->role = Role::Control;
        slot->control = *control;
        slot->over = true;
        if (*control == Control::Menu) menu_tapped_ = true;
    } else if (layout_.dpad_shown && layout_.dpad.contains(at, 1.15f)) {
        slot->role = Role::DPad;
    } else if (at.x < layout_.stick_split && !stick_.active) {
        slot->role = Role::Stick;
        stick_ = {true, at, at};
    } else if (at.x >= layout_.stick_split) {
        slot->role = Role::Camera;
    }
}

void Controls::finger_move(std::uint64_t id, Point at) {
    Finger *finger = find(id);
    if (finger == nullptr) return;
    switch (finger->role) {
    case Role::Stick: {
        stick_.thumb = at;
        // Beyond its reach the stick follows the thumb, so a long drag keeps
        // pushing the way the thumb went instead of losing the centre.
        const float dx = at.x - stick_.origin.x;
        const float dy = at.y - stick_.origin.y;
        const float length = std::hypot(dx, dy);
        if (length > layout_.stick_radius && length > 0.0f) {
            const float excess = (length - layout_.stick_radius) / length;
            stick_.origin.x += dx * excess;
            stick_.origin.y += dy * excess;
        }
        break;
    }
    case Role::Control: {
        // A thumb may slide from one face button to the next.
        const bool face = static_cast<std::size_t>(finger->control) <= static_cast<std::size_t>(Control::Square);
        if (face) {
            if (const std::optional<Control> control = control_at(at, true)) {
                finger->control = *control;
                finger->over = true;
            } else {
                finger->over = false;
            }
        } else {
            finger->over = layout_.controls[static_cast<std::size_t>(finger->control)].contains(at, 1.6f);
        }
        break;
    }
    case Role::Camera:
        camera_drag_.x += at.x - finger->last.x;
        camera_drag_.y += at.y - finger->last.y;
        break;
    case Role::DPad:  // the direction follows the finger; see buttons()
    case Role::None: break;
    }
    finger->last = at;
}

void Controls::finger_up(std::uint64_t id) {
    Finger *finger = find(id);
    if (finger == nullptr) return;
    if (finger->role == Role::Stick) stick_ = {};
    *finger = Finger{};
}

void Controls::release_all() {
    fingers_ = {};
    stick_ = {};
    camera_drag_ = {};
    menu_tapped_ = false;
}

std::uint16_t Controls::buttons() const {
    std::uint16_t buttons = 0u;
    for (const Finger &finger : fingers_) {
        if (!finger.used) continue;
        if (finger.role == Role::Control && finger.over) buttons |= psp_button(finger.control);
        if (finger.role == Role::DPad) buttons |= dpad_held_by(finger);
    }
    return buttons;
}

std::uint16_t Controls::dpad_held_by(const Finger &finger) const {
    return dpad_buttons({finger.last.x - layout_.dpad.centre.x, finger.last.y - layout_.dpad.centre.y},
                        layout_.dpad.radius);
}

std::uint16_t Controls::dpad_held() const {
    std::uint16_t held = 0u;
    for (const Finger &finger : fingers_)
        if (finger.used && finger.role == Role::DPad) held |= dpad_held_by(finger);
    return held;
}

bool Controls::held(Control control) const {
    for (const Finger &finger : fingers_)
        if (finger.used && finger.role == Role::Control && finger.over && finger.control == control) return true;
    return false;
}

bool Controls::any_finger() const {
    for (const Finger &finger : fingers_)
        if (finger.used) return true;
    return false;
}

Point Controls::stick() const {
    if (!stick_.active) return {};
    return stick_deflection({stick_.thumb.x - stick_.origin.x, stick_.thumb.y - stick_.origin.y},
                            layout_.stick_radius);
}

Point Controls::take_camera_drag() {
    const Point drag = camera_drag_;
    camera_drag_ = {};
    return drag;
}

bool Controls::take_menu() {
    const bool tapped = menu_tapped_;
    menu_tapped_ = false;
    return tapped;
}

} // namespace mhp3rd::input::touch
