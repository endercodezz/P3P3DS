#pragma once

// Life cycle shared by the sceUtility dialogs.
//
//   InitStart -> INIT -> VISIBLE -> QUIT -> ShutdownStart -> FINISHED -> NONE
//
// The guest polls GetStatus (and, for some dialogs, Update) until the dialog
// reports QUIT, then calls ShutdownStart and waits for NONE. A dialog that
// reports itself running and never reaches QUIT stalls the guest for good, so
// the host does its work in InitStart and each poll moves the status one step
// on: every path, errors included, reaches QUIT within a few polls.
#include <cstdint>

namespace mhp3rd {

namespace dialog_status {
inline constexpr std::uint32_t kNone = 0u;
inline constexpr std::uint32_t kInit = 1u;
inline constexpr std::uint32_t kVisible = 2u;
inline constexpr std::uint32_t kQuit = 3u;
inline constexpr std::uint32_t kFinished = 4u;
} // namespace dialog_status

// Common header at the start of every dialog's parameter block.
namespace dialog_common {
inline constexpr std::uint32_t kSizeOffset = 0x00u;
inline constexpr std::uint32_t kResultOffset = 0x1Cu;
} // namespace dialog_common

// sceUtility*InitStart while another dialog of the same kind is active.
inline constexpr std::uint32_t kErrorUtilityInvalidStatus = 0x80110001u;

class DialogLifecycle {
public:
    [[nodiscard]] bool active() const noexcept { return status_ != dialog_status::kNone; }
    [[nodiscard]] std::uint32_t status() const noexcept { return status_; }

    void start() noexcept { status_ = dialog_status::kInit; }

    // Returns the status to report for this poll, then advances it.
    std::uint32_t poll() noexcept {
        const std::uint32_t reported = status_;
        if (status_ == dialog_status::kInit) status_ = dialog_status::kVisible;
        else if (status_ == dialog_status::kVisible) status_ = dialog_status::kQuit;
        else if (status_ == dialog_status::kFinished) status_ = dialog_status::kNone;
        return reported;
    }

    // ShutdownStart. Returns false when the dialog had not quit yet; it is
    // shut down anyway, since its work is already done and refusing would
    // leave the guest waiting.
    bool shutdown() noexcept {
        const bool quit = status_ == dialog_status::kQuit;
        if (status_ != dialog_status::kNone) status_ = dialog_status::kFinished;
        return quit;
    }

private:
    std::uint32_t status_{dialog_status::kNone};
};

} // namespace mhp3rd
