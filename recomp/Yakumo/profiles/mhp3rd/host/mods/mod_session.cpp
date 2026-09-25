#include "mods/mod_session.hpp"

#include <iostream>

namespace mhp3rd::mods {

ModSession::ModSession(const ModFormat &format, Paths paths, Game game)
    : library_(format), paths_(std::move(paths)), game_(std::move(game)) {}

Resolution ModSession::resolve() const {
    if (paths_.disabled_by != nullptr) return {};
    return library_.resolve();
}

void ModSession::start() {
    library_.load_choices(paths_.choices);
    library_.scan(paths_.folder);
    wanted_ = resolve();
    active_ = wanted_;
    game_.activate(active_);
}

void ModSession::rescan() {
    library_.scan(paths_.folder);
    commit();
}

void ModSession::commit() {
    error_.clear();
    if (!library_.save_choices(paths_.choices, error_)) std::cerr << "[mods] " << error_ << "\n";
    wanted_ = resolve();
    if (wanted_.same_files(active_)) {
        active_ = wanted_;  // the conflicts may read differently
        return;
    }
    if (game_.can_switch(wanted_)) {
        active_ = wanted_;
        game_.activate(active_);
    }
}

} // namespace mhp3rd::mods
