// Picks the installer's front end: the port's own setup screens when the
// game's window can be created, the SDL dialogs otherwise.

#include "install/installer.hpp"

#if defined(MHP3RD_HAS_RENDERER)
#include "ui/ui.hpp"
#endif

#include <iostream>

namespace mhp3rd::install {

std::unique_ptr<InstallerUi> make_installer_ui() {
#if defined(MHP3RD_HAS_RENDERER)
    if (auto screens = ui::make_setup_screens()) return screens;
#endif
    return make_dialog_ui();
}

bool report_problem(const std::string &title, const std::string &message, bool ask_setup) {
    std::cerr << title << ": " << message << "\n";
#if defined(MHP3RD_HAS_RENDERER)
    switch (ui::show_problem(title, message, ask_setup)) {
    case ui::ProblemAnswer::Quit: return false;
    case ui::ProblemAnswer::SetUpAgain: return true;
    case ui::ProblemAnswer::Unavailable: break;
    }
#endif
    return report_problem_in_dialog(title, message, ask_setup);
}

} // namespace mhp3rd::install
