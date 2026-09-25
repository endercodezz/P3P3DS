// The first-run setup as screens in the game's window: a welcome, the file
// browser, the choice between copying the image and using it in place,
// progress, and plain errors. It implements install::InstallerUi, so the
// installer's own logic decides the order.

#include "ui/ui.hpp"

#include "ui/file_browser.hpp"
#include "ui/layer.hpp"
#include "ui/widgets.hpp"

#include "hle/hle_common.hpp"
#include "install/game_identity.hpp"
#include "install/installer.hpp"
#include "install/user_data.hpp"
#include "settings/settings.hpp"
#if defined(MHP3RD_ANDROID_APP)
#include "platform/android_jni.hpp"
#endif

#include "imgui.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iostream>
#include <mutex>
#include <thread>

namespace mhp3rd::ui {
namespace {

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

const char *const kSubtitle = "Setup";

float font() { return Layer::get().font_size(); }

// Two buttons side by side, the first one primary. Returns 1 or 2 when one
// was activated. The first is focused when the screen appears.
int button_pair(const char *first, const char *second, bool focus_first, bool first_disabled = false) {
    const float gap = font() * 0.6f;
    const float width = std::min(font() * 13.0f, (ImGui::GetContentRegionAvail().x - gap) * 0.5f);
    ImGui::Dummy({0.0f, font() * 0.4f});
    if (focus_first) focus_next_row();
    int pressed = 0;
    if (big_button(first, width, true, first_disabled)) pressed = 1;
    ImGui::SameLine(0.0f, gap);
    if (big_button(second, width)) pressed = 2;
    return pressed;
}

std::string megabytes(std::uint64_t bytes) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.0f MB", static_cast<double>(bytes) / 1e6);
    return text;
}

class SetupScreens final : public install::InstallerUi {
public:
    bool introduce(const fs::path &data_dir) override;
    std::optional<fs::path> choose_image() override;
    std::optional<install::ImageStorage> choose_storage(const fs::path &image, const install::ImageInfo &info,
                                                        const fs::path &data_dir) override;
    void run_task(const std::string &title, const std::function<void()> &work) override;
    void progress(const std::string &stage, std::uint64_t done, std::uint64_t total) override;
    bool offer_retry(const std::string &message) override;
    void finished(const fs::path &data_dir) override;

private:
    // A file dropped onto the welcome screen skips the browser.
    std::optional<fs::path> dropped_;
    fs::path data_dir_;

    // Progress, written by the worker thread in progress().
    std::mutex mutex_;
    std::string stage_;
    std::uint64_t done_{};
    std::uint64_t total_{};
    Clock::time_point stage_started_{};
    std::atomic<bool> cancel_{false};
};

bool SetupScreens::introduce(const fs::path &data_dir) {
    data_dir_ = data_dir;
    Layer &layer = Layer::get();
    layer.set_interactive(true);
    bool first = true;
    int answer = 0;
    const bool window_open = layer.run(
        [&] {
            if (auto dropped = layer.take_dropped_file()) {
                dropped_ = std::move(dropped);
                answer = 1;
                return false;
            }
            if (layer.take_back()) {
                answer = 2;
                return false;
            }
            begin_panel("##welcome", "Welcome to Yakumo", kSubtitle, false);
            begin_content();
            ImGui::Dummy({0.0f, font() * 0.3f});
            paragraph(std::string("Yakumo plays ") + install::kGameTitle +
                      " from your own copy of the game. It needs the disc image of the game's PSP disc, " +
                      install::kDiscIdDisplay + ", as an .iso file: the PlayStation 3 release carries it.");
            ImGui::Dummy({0.0f, font() * 0.4f});
#if defined(MHP3RD_ANDROID_APP)
            paragraph("Choose the image next, in Android's file picker. Yakumo copies it into its own storage (an "
                      "app cannot keep reading a file elsewhere), checks that it is the right release and prepares "
                      "the game from it. The copy needs about 1.3 GB of free space, besides the 0.8 GB the app takes; "
                      "once it is made, the original can be deleted. Nothing is downloaded.",
                      colors::kTextDim);
#else
            paragraph("Choose the image next. Yakumo checks that it is the right release, prepares the game from it "
                      "and, unless you choose otherwise, copies it into its data folder so the game keeps working if "
                      "the original is moved or deleted. Nothing is downloaded.",
                      colors::kTextDim);
#endif
            ImGui::Dummy({0.0f, font() * 0.4f});
            section("Data folder");
            ImGui::Indent(std::round(16.0f * layer.scale()));
            paragraph(install::path_to_utf8(data_dir), colors::kTextDim);
            ImGui::Unindent(std::round(16.0f * layer.scale()));
            ImGui::Dummy({0.0f, font() * 0.6f});
            answer = button_pair("Choose disc image", "Quit", first);
            first = false;
#if !defined(MHP3RD_ANDROID_APP)
            ImGui::Dummy({0.0f, font() * 0.4f});
            paragraph("You can also drop the .iso file onto this window.", colors::kTextDim);
#endif
            layer.set_description("Monster Hunter Portable 3rd HD Ver. (NPJB-40001) is the only release supported.");
            begin_footer();
            hints({{Control::Confirm, "Select"}, {Control::Back, "Quit"}});
            end_panel();
            return answer == 0;
        },
        false);
    return window_open && answer == 1;
}

#if defined(MHP3RD_ANDROID_APP)
std::optional<fs::path> SetupScreens::choose_image() {
    for (;;) {
        const std::optional<std::string> uri = android::pick_document();
        if (!uri) return std::nullopt;
        std::cout << "[setup] picked " << *uri << std::endl;
        // The picked image is a content:// document, readable only through a
        // file descriptor, so it is copied into the data folder, with
        // progress, before it is checked. It cannot be used where it is.
        try {
            fs::path copied;
            run_task("Copying the disc image", [&] {
                copied = install::copy_image_document(*uri, data_dir_,
                                                      [this](const std::string &stage, std::uint64_t done,
                                                             std::uint64_t total) { progress(stage, done, total); });
            });
            return copied;
        } catch (const install::InstallCancelled &) {
            return std::nullopt;
        } catch (const install::InstallError &e) {
            std::cerr << "Setup: " << e.what() << "\n";
            if (!offer_retry(e.what())) return std::nullopt;
        }
    }
}
#else
std::optional<fs::path> SetupScreens::choose_image() {
    if (dropped_) return std::exchange(dropped_, std::nullopt);
    Layer &layer = Layer::get();
    layer.set_interactive(true);
    FileBrowser browser(settings::current().last_folder.empty()
                            ? FileBrowser::home()
                            : install::path_from_utf8(settings::current().last_folder));
    std::optional<fs::path> chosen;
    layer.run(
        [&] {
            if (auto dropped = layer.take_dropped_file()) {
                chosen = std::move(dropped);
                return false;
            }
            const bool back = layer.take_back();
            const ImGuiKey cancel = layer.confirm_south() ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
            const bool pad_back = ImGui::IsKeyPressed(cancel, false);
            begin_panel("##browser", "Choose the disc image", kSubtitle, false);
            begin_content();
            const FileBrowser::Result result = browser.frame(back || pad_back);
            layer.set_description(std::string("Look for the .iso image of ") + install::kDiscIdDisplay +
                                  ". Removable drives and SD cards are listed next to Home.");
            begin_footer();
            if (layer.input_device() == InputDevice::Gamepad)
                hints({{Control::Confirm, "Open"}, {Control::Back, "Up a folder"}, {Control::Toggle, "All files"}});
            else
                hints({{Control::Confirm, "Open"}, {Control::Back, "Up a folder"}});
            end_panel();
            if (result == FileBrowser::Result::Chosen) chosen = browser.chosen();
            return result == FileBrowser::Result::Browsing;
        },
        false);
    if (chosen) {
        // The browser opens where the player found the image next time.
        settings::current().last_folder = install::path_to_utf8(chosen->parent_path());
        settings::save();
    }
    return chosen;
}
#endif

std::optional<install::ImageStorage> SetupScreens::choose_storage(const fs::path &image,
                                                                  const install::ImageInfo &info,
                                                                  const fs::path &data_dir) {
#if defined(MHP3RD_ANDROID_APP)
    // Already copied into the data folder by choose_image().
    (void)image;
    (void)info;
    (void)data_dir;
    return install::ImageStorage::Copy;
#endif
    Layer &layer = Layer::get();
    const std::uint64_t needed = install::space_needed_to_copy(info);
    const std::optional<std::uint64_t> space = install::available_space(data_dir);
    const bool room = !space || *space >= needed;
    std::optional<install::ImageStorage> choice;
    bool first = true;
    layer.run(
        [&] {
            if (layer.take_back()) return false;
            const ImGuiKey cancel = layer.confirm_south() ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
            if (ImGui::IsKeyPressed(cancel, false)) return false;
            begin_panel("##storage", "Disc image found", kSubtitle, false);
            begin_content();
            paragraph(std::string(install::kGameTitle) + " (" + install::kDiscIdDisplay + "), " +
                          human_size(info.size_bytes) + ". It passed its checks.",
                      colors::kGood);
            ImGui::Dummy({0.0f, font() * 0.2f});
            paragraph(install::path_to_utf8(image), colors::kTextDim);
            ImGui::Dummy({0.0f, font() * 0.6f});
            heading("Keep a copy, or use the image where it is?");

            std::string copy_note = "Recommended. The game keeps working if the original is moved or deleted. Needs " +
                                    human_size(needed);
            if (space) copy_note += "; " + human_size(*space) + " free";
            copy_note += ".";
            if (!room) copy_note = "Not enough free space: the copy needs " + human_size(needed) + " and " +
                                   human_size(*space) + " is free in the data folder.";
            RowOptions copy_options{!room, {}, {}};
            if (first && room) focus_next_row();
            if (button_row("Copy it into Yakumo's data folder", copy_options)) choice = install::ImageStorage::Copy;
            ImGui::Indent(std::round(16.0f * layer.scale()));
            paragraph(copy_note, room ? colors::kTextDim : colors::kDanger);
            ImGui::Unindent(std::round(16.0f * layer.scale()));
            ImGui::Dummy({0.0f, font() * 0.4f});
            const std::string place_note =
                "Saves " + human_size(info.size_bytes) + ". The image must then stay where it is; if it moves, "
                "Yakumo asks you to set up again.";
            if (first && !room) focus_next_row();
            if (button_row("Use it where it is")) choice = install::ImageStorage::InPlace;
            ImGui::Indent(std::round(16.0f * layer.scale()));
            paragraph(place_note, colors::kTextDim);
            ImGui::Unindent(std::round(16.0f * layer.scale()));
            first = false;
            layer.set_description("Either way, the game's executable is prepared from the image into Yakumo's data "
                                  "folder.");
            begin_footer();
            hints({{Control::Confirm, "Continue"}, {Control::Back, "Choose another file"}});
            end_panel();
            return !choice;
        },
        false);
    return choice;
}

void SetupScreens::run_task(const std::string &title, const std::function<void()> &work) {
    Layer &layer = Layer::get();
    {
        std::lock_guard lock(mutex_);
        stage_.clear();
        done_ = total_ = 0u;
        stage_started_ = Clock::now();
    }
    cancel_ = false;
    std::atomic<bool> finished{false};
    std::exception_ptr failure;
    std::thread worker([&] {
        try {
            work();
        } catch (...) {
            failure = std::current_exception();
        }
        finished = true;
    });
    bool first = true;
    const Clock::time_point started = Clock::now();
    const bool window_open = layer.run(
        [&] {
            if (finished) return false;
            const ImGuiKey cancel_key = layer.confirm_south() ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
            if (layer.take_back() || ImGui::IsKeyPressed(cancel_key, false)) cancel_ = true;
            std::string stage;
            std::uint64_t done = 0;
            std::uint64_t total = 0;
            Clock::time_point stage_started;
            {
                std::lock_guard lock(mutex_);
                stage = stage_;
                done = done_;
                total = total_;
                stage_started = stage_started_;
            }
            begin_panel("##progress", title, kSubtitle, false);
            begin_content();
            ImGui::Dummy({0.0f, font() * 1.5f});
            heading(stage.empty() ? title + "…" : stage);
            ImGui::Dummy({0.0f, font() * 0.3f});
            if (total > 1u) {
                const float fraction = static_cast<float>(static_cast<double>(done) / static_cast<double>(total));
                char text[96];
                if (total >= 1'000'000u)
                    std::snprintf(text, sizeof(text), "%d%%   %s of %s", static_cast<int>(fraction * 100.0f),
                                  megabytes(done).c_str(), megabytes(total).c_str());
                else std::snprintf(text, sizeof(text), "%d%%", static_cast<int>(fraction * 100.0f));
                progress_bar(fraction, text);
                const double seconds = std::chrono::duration<double>(Clock::now() - stage_started).count();
                if (fraction > 0.02f && seconds > 2.0) {
                    const double left = seconds * (1.0 - fraction) / fraction;
                    char eta[64];
                    std::snprintf(eta, sizeof(eta), "About %d s left", static_cast<int>(std::ceil(left)));
                    paragraph(eta, colors::kTextDim);
                }
            } else {
                // No size to measure against (the checks): a moving bar.
                const double t = std::chrono::duration<double>(Clock::now() - started).count();
                progress_bar(static_cast<float>(0.5 + 0.5 * std::sin(t * 3.0)), "");
            }
            ImGui::Dummy({0.0f, font() * 1.0f});
            const float width = std::min(font() * 13.0f, ImGui::GetContentRegionAvail().x);
            if (first) focus_next_row();
            first = false;
            if (big_button(cancel_ ? "Cancelling…" : "Cancel", width, false, cancel_)) cancel_ = true;
            layer.set_description("Cancelling removes everything this step has written so far.");
            begin_footer();
            hints({{Control::Back, "Cancel"}});
            end_panel();
            return true;
        },
        false);
    if (!window_open) cancel_ = true;
    worker.join();
    if (failure) std::rethrow_exception(failure);
}

void SetupScreens::progress(const std::string &stage, std::uint64_t done, std::uint64_t total) {
    install::print_progress(stage, done, total);
    {
        std::lock_guard lock(mutex_);
        if (stage != stage_) stage_started_ = Clock::now();
        stage_ = stage;
        done_ = done;
        total_ = total;
    }
    if (cancel_) throw install::InstallCancelled();
}

bool SetupScreens::offer_retry(const std::string &message) {
    Layer &layer = Layer::get();
    const bool space = message.find("free space") != std::string::npos;
    int answer = 0;
    bool first = true;
    const bool window_open = layer.run(
        [&] {
            if (auto dropped = layer.take_dropped_file()) {
                dropped_ = std::move(dropped);
                answer = 1;
                return false;
            }
            if (layer.take_back()) {
                answer = 2;
                return false;
            }
            begin_panel("##error", space ? "Not enough free space" : "This file can't be used", kSubtitle, false);
            begin_content();
            ImGui::Dummy({0.0f, font() * 0.5f});
            paragraph(message);
            ImGui::Dummy({0.0f, font() * 0.8f});
            answer = button_pair("Choose another file", "Quit", first);
            first = false;
            layer.set_description(std::string("Yakumo supports only ") + install::kGameTitle + ", " +
                                  install::kDiscIdDisplay + ", as an unmodified, uncompressed .iso image.");
            begin_footer();
            hints({{Control::Confirm, "Select"}, {Control::Back, "Quit"}});
            end_panel();
            return answer == 0;
        },
        false);
    return window_open && answer == 1;
}

void SetupScreens::finished(const fs::path &data_dir) {
    Layer &layer = Layer::get();
    bool first = true;
    layer.run(
        [&] {
            begin_panel("##done", "All set", kSubtitle, false);
            begin_content();
            ImGui::Dummy({0.0f, font() * 0.5f});
            paragraph("The game is ready. Later starts go straight to it.", colors::kGood);
            ImGui::Dummy({0.0f, font() * 0.3f});
#if defined(MHP3RD_ANDROID_APP)
            paragraph("In the game, Back, the menu button at the top of the touch controls, or L3+R3 on a gamepad "
                      "(both sticks pressed) opens Yakumo's menu, with the settings and the way back to this setup. "
                      "Touch the screen to show the touch controls.",
                      colors::kTextDim);
#else
            paragraph("In the game, Esc or L3+R3 (both sticks pressed) opens Yakumo's menu, with the settings and the "
                      "way back to this setup.",
                      colors::kTextDim);
#endif
            ImGui::Dummy({0.0f, font() * 0.3f});
            section("Data folder");
            ImGui::Indent(std::round(16.0f * layer.scale()));
            paragraph(install::path_to_utf8(data_dir), colors::kTextDim);
            ImGui::Unindent(std::round(16.0f * layer.scale()));
            ImGui::Dummy({0.0f, font() * 0.8f});
            if (first) focus_next_row();
            first = false;
            const bool play = big_button("Play", std::min(font() * 13.0f, ImGui::GetContentRegionAvail().x), true);
            begin_footer();
            hints({{Control::Confirm, "Play"}});
            end_panel();
            return !play;
        },
        false);
    layer.set_interactive(false);
}

// The screen for a problem found before the game starts.
ProblemAnswer run_problem(const std::string &title, const std::string &message, bool ask_setup) {
    Layer &layer = Layer::get();
    layer.set_interactive(true);
    int answer = 0;
    bool first = true;
    const bool window_open = layer.run(
        [&] {
            if (layer.take_back()) {
                answer = ask_setup ? 2 : 1;
                return false;
            }
            begin_panel("##problem", title, "", false);
            begin_content();
            ImGui::Dummy({0.0f, font() * 0.5f});
            paragraph(message);
            ImGui::Dummy({0.0f, font() * 0.8f});
            if (ask_setup) {
                answer = button_pair("Set up again", "Quit", first);
            } else {
                if (first) focus_next_row();
                if (big_button("Quit", std::min(font() * 13.0f, ImGui::GetContentRegionAvail().x), true)) answer = 1;
            }
            first = false;
            begin_footer();
            hints({{Control::Confirm, "Select"}, {Control::Back, "Quit"}});
            end_panel();
            return answer == 0;
        },
        false);
    layer.set_interactive(false);
    if (!window_open) return ProblemAnswer::Quit;
    return ask_setup && answer == 1 ? ProblemAnswer::SetUpAgain : ProblemAnswer::Quit;
}

} // namespace

std::unique_ptr<install::InstallerUi> make_setup_screens() {
    if (ensure_renderer() == nullptr || !Layer::get().attached()) return nullptr;
    return std::make_unique<SetupScreens>();
}

ProblemAnswer show_problem(const std::string &title, const std::string &message, bool ask_setup) {
    if (ensure_renderer() == nullptr || !Layer::get().attached()) return ProblemAnswer::Unavailable;
    return run_problem(title, message, ask_setup);
}

} // namespace mhp3rd::ui
