#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

// On-screen controls for a touch screen (#17, #103), drawn by the host over
// the game, never into its picture. This part is the pure logic: where the
// controls go on a screen of a given shape, which finger holds what, and the
// pad state and camera turn that come out. The renderer feeds it fingers in
// window pixels and draws it; nothing here needs SDL or ImGui.
//
// The left of the screen is the movement stick, a floating one: it appears
// where the thumb lands and follows the thumb when it is dragged beyond its
// reach. A fixed D-pad sits at the left edge, half way down, for the game's
// menus: above where a thumb rests to walk, below L. The right holds the four face buttons in the PSP's diamond, L and R
// sit at the top corners, Start, Select and the menu button small at the top
// centre. A drag anywhere else on the right half turns the camera, as the
// mouse does. Every finger is independent, so moving, turning and pressing
// work together.
namespace mhp3rd::input::touch {

enum class Control : std::uint8_t { Triangle, Circle, Cross, Square, L, R, Start, Select, Menu, Count };
inline constexpr std::size_t kControls = static_cast<std::size_t>(Control::Count);

// The PSP button (SceCtrlButtons) a control presses; 0 for the menu button.
[[nodiscard]] std::uint16_t psp_button(Control control);

struct Point {
    float x{};
    float y{};
};
struct Circle {
    Point centre;
    float radius{};
    [[nodiscard]] bool contains(Point p, float slack = 1.0f) const;
};
// Edges the controls keep clear of (a display cutout, rounded corners), px.
struct Insets {
    float left{};
    float top{};
    float right{};
    float bottom{};
};

struct Layout {
    std::array<Circle, kControls> controls{};
    // The D-pad, when shown: its centre and the reach of its arms.
    bool dpad_shown{};
    Circle dpad{};
    float stick_radius{};    // the stick's reach from where the thumb landed
    float stick_split{};     // x: fingers landing left of it are for the stick
    float width{};
    float height{};
};
// A layout for a width x height screen. `size` scales every control (1 is the
// default, sized for thumbs on a phone held sideways); `dpad` shows the D-pad.
[[nodiscard]] Layout make_layout(float width, float height, Insets insets, float size, bool dpad = true);

// The PSP D-pad bits (up 0x10, right 0x20, down 0x40, left 0x80) for a finger
// at `offset` from the D-pad's centre: one direction within 22.5 degrees of
// it, two adjacent ones between, as a PSP D-pad can press them; nothing in
// the middle (a fifth of the reach).
[[nodiscard]] std::uint16_t dpad_buttons(Point offset, float reach);

// Stick deflection for a thumb at `offset` from the stick's centre, each axis
// -1..1 (right and down positive), with a dead zone as a fraction of `reach`.
[[nodiscard]] Point stick_deflection(Point offset, float reach, float dead_zone = 0.12f);

struct Stick {
    bool active{};
    Point origin;   // where the stick is centred now
    Point thumb;    // where the thumb is
};

class Controls {
public:
    void set_layout(const Layout &layout) { layout_ = layout; }
    [[nodiscard]] const Layout &layout() const { return layout_; }

    void finger_down(std::uint64_t id, Point at);
    void finger_move(std::uint64_t id, Point at);
    void finger_up(std::uint64_t id);
    // Lets go of everything, for when the game loses input (a menu opens).
    void release_all();

    // PSP buttons held now.
    [[nodiscard]] std::uint16_t buttons() const;
    // The stick, each axis -1..1, right and down positive.
    [[nodiscard]] Point stick() const;
    [[nodiscard]] const Stick &stick_state() const { return stick_; }
    // Camera drag since the last take, in pixels.
    [[nodiscard]] Point take_camera_drag();
    // The menu button was tapped since the last take.
    [[nodiscard]] bool take_menu();
    [[nodiscard]] bool held(Control control) const;
    // The D-pad directions held now (PSP bits), for drawing.
    [[nodiscard]] std::uint16_t dpad_held() const;
    [[nodiscard]] bool any_finger() const;

private:
    enum class Role : std::uint8_t { None, Stick, Control, Camera, DPad };
    struct Finger {
        std::uint64_t id{};
        bool used{};
        Role role{};
        Control control{};
        bool over{};    // the finger is still over its control
        Point last;
    };
    static constexpr std::size_t kFingers = 10u;
    [[nodiscard]] std::optional<Control> control_at(Point at, bool face_only) const;
    Finger *find(std::uint64_t id);
    [[nodiscard]] std::uint16_t dpad_held_by(const Finger &finger) const;

    Layout layout_{};
    std::array<Finger, kFingers> fingers_{};
    Stick stick_{};
    Point camera_drag_{};
    bool menu_tapped_{};
};

} // namespace mhp3rd::input::touch
