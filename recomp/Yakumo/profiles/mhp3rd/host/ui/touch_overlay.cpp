#include "ui/touch_overlay.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace mhp3rd::ui {
namespace {

using input::touch::Circle;
using input::touch::Control;

ImU32 colour(int r, int g, int b, float alpha) {
    return IM_COL32(r, g, b, static_cast<int>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f));
}

// The PSP's symbols drawn as shapes, so no font has to carry them.
void draw_symbol(ImDrawList *list, Control control, ImVec2 c, float r, ImU32 ink, float thickness) {
    const float s = r * 0.42f;
    switch (control) {
    case Control::Triangle:
        list->AddTriangle({c.x, c.y - s}, {c.x + s * 0.95f, c.y + s * 0.65f}, {c.x - s * 0.95f, c.y + s * 0.65f}, ink,
                          thickness);
        break;
    case Control::Circle: list->AddCircle(c, s * 0.9f, ink, 0, thickness); break;
    case Control::Cross:
        list->AddLine({c.x - s * 0.8f, c.y - s * 0.8f}, {c.x + s * 0.8f, c.y + s * 0.8f}, ink, thickness);
        list->AddLine({c.x + s * 0.8f, c.y - s * 0.8f}, {c.x - s * 0.8f, c.y + s * 0.8f}, ink, thickness);
        break;
    case Control::Square:
        list->AddRect({c.x - s * 0.75f, c.y - s * 0.75f}, {c.x + s * 0.75f, c.y + s * 0.75f}, ink, 0.0f, 0, thickness);
        break;
    case Control::Menu:
        for (int i = -1; i <= 1; ++i)
            list->AddLine({c.x - s * 0.9f, c.y + i * s * 0.6f}, {c.x + s * 0.9f, c.y + i * s * 0.6f}, ink, thickness);
        break;
    default: break;
    }
}

const char *label(Control control) {
    switch (control) {
    case Control::L: return "L";
    case Control::R: return "R";
    case Control::Start: return "START";
    case Control::Select: return "SELECT";
    default: return nullptr;
    }
}

} // namespace

void draw_touch_controls(const input::touch::Controls &controls, float opacity) {
    ImDrawList *list = ImGui::GetBackgroundDrawList();
    const input::touch::Layout &layout = controls.layout();
    const float base = std::clamp(opacity, 0.1f, 1.0f);
    // PSP colours for the symbols.
    const ImU32 symbol_colours[] = {colour(64, 224, 176, base), colour(240, 96, 112, base),
                                    colour(128, 160, 255, base), colour(232, 144, 208, base)};
    for (std::size_t i = 0; i < input::touch::kControls; ++i) {
        const auto control = static_cast<Control>(i);
        const Circle &circle = layout.controls[i];
        const ImVec2 c{circle.centre.x, circle.centre.y};
        const bool held = controls.held(control);
        const float thickness = std::max(1.5f, circle.radius * 0.08f);
        list->AddCircleFilled(c, circle.radius, colour(0, 0, 0, base * (held ? 0.55f : 0.3f)));
        // A dark edge under the light one keeps the outline visible on a bright picture.
        list->AddCircle(c, circle.radius + thickness, colour(0, 0, 0, base * 0.6f), 0, thickness);
        list->AddCircle(c, circle.radius, colour(255, 255, 255, base * (held ? 1.0f : 0.8f)), 0, thickness);
        if (held) list->AddCircleFilled(c, circle.radius * 0.92f, colour(255, 255, 255, base * 0.35f));
        const ImU32 ink = i < 4u ? symbol_colours[i] : colour(255, 255, 255, base);
        if (const char *text = label(control)) {
            ImFont *font = ImGui::GetFont();
            const float size = circle.radius * (text[1] == '\0' ? 0.9f : 0.42f);
            const ImVec2 extent = font->CalcTextSizeA(size, 1e9f, 0.0f, text);
            list->AddText(font, size, {c.x - extent.x * 0.5f, c.y - extent.y * 0.5f}, ink, text);
        } else {
            draw_symbol(list, control, c, circle.radius, ink, thickness * 1.3f);
        }
    }
    // The D-pad: a cross of four arms, each lit while held.
    if (layout.dpad_shown) {
        const ImVec2 c{layout.dpad.centre.x, layout.dpad.centre.y};
        const float reach = layout.dpad.radius;
        const float half = reach * 0.31f;
        const float thickness = std::max(1.5f, reach * 0.035f);
        const std::uint16_t held = controls.dpad_held();
        struct Arm {
            std::uint16_t bit;
            float dx, dy;
        };
        const Arm arms[] = {{0x10u, 0.0f, -1.0f}, {0x20u, 1.0f, 0.0f}, {0x40u, 0.0f, 1.0f}, {0x80u, -1.0f, 0.0f}};
        list->AddRectFilled({c.x - half, c.y - half}, {c.x + half, c.y + half}, colour(0, 0, 0, base * 0.3f));
        for (const Arm &arm : arms) {
            const bool on = (held & arm.bit) != 0u;
            // The arm's rectangle from the centre square out to the reach.
            const ImVec2 a{c.x + (arm.dx != 0.0f ? arm.dx * half : -half), c.y + (arm.dy != 0.0f ? arm.dy * half : -half)};
            const ImVec2 b{c.x + (arm.dx != 0.0f ? arm.dx * reach : half), c.y + (arm.dy != 0.0f ? arm.dy * reach : half)};
            const ImVec2 low{std::min(a.x, b.x), std::min(a.y, b.y)};
            const ImVec2 high{std::max(a.x, b.x), std::max(a.y, b.y)};
            list->AddRectFilled(low, high, colour(0, 0, 0, base * (on ? 0.55f : 0.3f)), half * 0.25f);
            if (on) list->AddRectFilled(low, high, colour(255, 255, 255, base * 0.8f), half * 0.25f);
            list->AddRect({low.x - thickness, low.y - thickness}, {high.x + thickness, high.y + thickness},
                          colour(0, 0, 0, base * 0.6f), half * 0.25f, 0, thickness);
            list->AddRect(low, high, colour(255, 255, 255, base * (on ? 1.0f : 0.8f)), half * 0.25f, 0, thickness);
            // A small triangle pointing out along the arm.
            const float t = reach * 0.66f;
            const float w = half * 0.55f;
            const ImVec2 tip{c.x + arm.dx * (t + w * 0.6f), c.y + arm.dy * (t + w * 0.6f)};
            const ImVec2 side1{c.x + arm.dx * (t - w * 0.4f) - arm.dy * w, c.y + arm.dy * (t - w * 0.4f) + arm.dx * w};
            const ImVec2 side2{c.x + arm.dx * (t - w * 0.4f) + arm.dy * w, c.y + arm.dy * (t - w * 0.4f) - arm.dx * w};
            list->AddTriangleFilled(tip, side1, side2, colour(255, 255, 255, base * (on ? 1.0f : 0.8f)));
        }
    }
    // The floating stick while the thumb is down: its reach and the knob.
    const input::touch::Stick &stick = controls.stick_state();
    if (stick.active) {
        const ImVec2 origin{stick.origin.x, stick.origin.y};
        list->AddCircleFilled(origin, layout.stick_radius, colour(0, 0, 0, base * 0.3f));
        list->AddCircle(origin, layout.stick_radius, colour(255, 255, 255, base * 0.7f), 0,
                        std::max(1.5f, layout.stick_radius * 0.04f));
        const input::touch::Point push = controls.stick();
        const ImVec2 knob{origin.x + push.x * layout.stick_radius, origin.y + push.y * layout.stick_radius};
        list->AddCircleFilled(knob, layout.stick_radius * 0.45f, colour(255, 255, 255, base * 0.6f));
    }
}

} // namespace mhp3rd::ui
