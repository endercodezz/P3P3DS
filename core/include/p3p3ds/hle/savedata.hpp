#pragma once
// sceUtilitySavedata: the PSP system save/load utility.
//
// Parameter block: psp/pspsdk/src/utility/psputility_savedata.h
// (SceUtilitySavedataParam, 0x600 bytes for firmware >= 2.00). Status
// sequence 1 -> 2 (Update) -> 3 -> ShutdownStart -> 4 -> 0 and result codes:
// hardware transcripts in references/pspautotests/tests/utility/savedata.
//
// Saves live on the host under `root` (ms0:/PSP/SAVEDATA), one directory per
// <gameName><saveName>, with the data file stored unencrypted, PARAM.SFO and
// the icon/picture/sound files the game supplies. [INFERRED] P3P passes a
// key (secure save), which the PSP would use to encrypt the data file; saves
// are therefore not interchangeable with PSP/PPSSPP saves yet.
//
// List modes (LISTLOAD/LISTSAVE/LISTDELETE) need a choice made by the
// player in the system UI. The core exposes it as pending_dialog(); the
// platform draws its own UI (3DS bottom screen, PC policy) and calls resolve().
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace psprecomp { class Runtime; }

namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

struct SavedataSlot {
    std::string name;            // saveName, e.g. "DATA00"
    bool exists{};
    std::string title, savedata_title, detail; // from PARAM.SFO
    std::filesystem::file_time_type modified{};
};

struct SavedataDialog {
    enum class Kind { Load, Save, Delete } kind{Kind::Load};
    std::string game;
    std::vector<SavedataSlot> slots;
    int focus{};
};

class SavedataUtility {
public:
    std::filesystem::path root; // host directory backing ms0:/PSP/SAVEDATA
    // Optional immediate choice (PC runner): returns a slot index or -1 to cancel.
    std::function<int(const SavedataDialog &)> auto_choice;

    // The list dialog waiting for the player, or null.
    [[nodiscard]] const SavedataDialog *pending_dialog() const { return waiting_ ? &*dialog_ : nullptr; }
    // Choice for the pending dialog: slot index, or -1 to cancel.
    void resolve(int slot_index) { choice_ = slot_index; waiting_ = false; }

    [[nodiscard]] int status() const { return status_; }
    [[nodiscard]] std::uint64_t operations() const { return operations_; }

private:
    friend void register_savedata_module(psprecomp::Runtime &, KernelState &);
    int status_{};                 // 0 none, 1 init, 2 visible, 3 quit, 4 finished
    std::uint32_t param_{};
    std::uint32_t mode_{};
    bool init_reported_{}, shutdown_reported_{};
    bool waiting_{};
    int choice_{-2};
    std::optional<SavedataDialog> dialog_;
    std::uint64_t operations_{};
};

void register_savedata_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
