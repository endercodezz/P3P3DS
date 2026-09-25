// sceUtility dialogs: the on-screen keyboard and the message dialog. The
// save-data dialog is in hle_savedata.cpp. The keyboard opens the port's own
// (host/ui/text_input.hpp); the message dialog draws nothing yet and answers
// the way a player confirming it would.
#include "hle_common.hpp"
#include "utility_dialog.hpp"

#include "settings/settings.hpp"

#if defined(MHP3RD_HAS_RENDERER)
#include "ui/text_input.hpp"
#endif

#include "psprecomp/common.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace mhp3rd {
namespace {

// The on-screen keyboard's parameters, as the game's requests show them
// (MHP3RD_TRACE_OSK): the common dialog header, then at 0x30 the number of
// fields, at 0x34 the address of the first, and at 0x38 a state word the
// keyboard keeps. Each field describes one text box.
namespace osk {
constexpr std::uint32_t kFieldCount = 0x30u;
constexpr std::uint32_t kFields = 0x34u;
constexpr std::uint32_t kState = 0x38u;
// Field offsets. The game asks for its hunter name with input type 0xDF0F,
// one line, an empty description, a 13-unit output buffer and no limit.
constexpr std::uint32_t kInputType = 0x10u;
constexpr std::uint32_t kLines = 0x14u;
constexpr std::uint32_t kDescription = 0x1Cu;
constexpr std::uint32_t kInitialText = 0x20u;
constexpr std::uint32_t kOutputLength = 0x24u;  // UTF-16 units, the terminator included
constexpr std::uint32_t kOutputText = 0x28u;
constexpr std::uint32_t kResult = 0x2Cu;
constexpr std::uint32_t kOutputLimit = 0x30u;   // characters; 0: the buffer decides

constexpr std::uint32_t kResultCancelled = 1u;
constexpr std::uint32_t kResultChanged = 2u;
// The common header's result after a cancel.
constexpr std::uint32_t kDialogCancelled = 1u;
} // namespace osk

// Where the keyboard is in its life cycle (see utility_dialog.hpp):
//
//   InitStart -> INIT -> VISIBLE ... answer ... QUIT -> ShutdownStart -> FINISHED -> NONE
//
// The game polls GetStatus, and calls Update while the status is VISIBLE.
// Each poll moves INIT and FINISHED one step on; VISIBLE lasts until the
// answer is there: at once for a fixed name, when the player closes the
// keyboard otherwise.
struct OskAnswer {
    bool cancelled{};
    std::string text;  // UTF-8
};

struct OskState {
    std::uint32_t params{};
    std::uint32_t status{dialog_status::kNone};
    std::optional<OskAnswer> answer;
};

OskState &osk_state() {
    static OskState state;
    return state;
}

// MHP3RD_TRACE_OSK: every call of the keyboard utility, with the words of
// its parameter block and of the first field, so their layout is read off the
// game rather than recalled.
bool trace_osk() {
    static const bool trace = std::getenv("MHP3RD_TRACE_OSK") != nullptr;
    return trace;
}

void dump_words(const psprecomp::GuestMemory &memory, const char *what, std::uint32_t address, std::uint32_t bytes) {
    if (address == 0u) return;
    for (std::uint32_t offset = 0; offset < bytes; offset += 16u) {
        std::cout << "[osk-trace]   " << what << " +" << std::hex << offset << ":";
        for (std::uint32_t i = offset; i < std::min(bytes, offset + 16u); i += 4u)
            std::cout << " " << psprecomp::hex32(memory.load32(address + i));
        std::cout << std::dec << "\n";
    }
}

void dump_utf16(const psprecomp::GuestMemory &memory, const char *what, std::uint32_t address) {
    if (address == 0u) return;
    std::cout << "[osk-trace]   " << what << " @" << psprecomp::hex32(address) << ":";
    for (std::uint32_t i = 0; i < 40u; ++i) {
        const std::uint16_t unit = memory.load16(address + i * 2u);
        std::cout << " " << std::hex << unit << std::dec;
        if (unit == 0u) break;
    }
    std::cout << "\n";
}

void trace_block(const psprecomp::GuestMemory &memory, std::uint32_t params) {
    if (params == 0u) return;
    const std::uint32_t size = std::min<std::uint32_t>(memory.load32(params), 0x100u);
    dump_words(memory, "params", params, size);
    const std::uint32_t count = memory.load32(params + osk::kFieldCount);
    const std::uint32_t field = memory.load32(params + osk::kFields);
    if (count == 0u || field == 0u) return;
    dump_words(memory, "field", field, 0x40u);
    for (std::uint32_t offset = 0; offset < 0x40u; offset += 4u) {
        const std::uint32_t word = memory.load32(field + offset);
        if ((word & 0x0F000000u) != 0x08000000u) continue;
        const std::string label = "field+" + std::to_string(offset) + " ->";
        dump_utf16(memory, label.c_str(), word);
    }
}

std::string read_utf16(const psprecomp::GuestMemory &memory, std::uint32_t address, std::size_t max_units = 512u) {
    std::string text;
    if (address == 0u) return text;
    for (std::size_t i = 0; i < max_units; ++i) {
        char32_t c = memory.load16(address + static_cast<std::uint32_t>(i) * 2u);
        if (c == 0u) break;
        if (c >= 0xD800u && c < 0xDC00u && i + 1u < max_units) {
            const char32_t low = memory.load16(address + static_cast<std::uint32_t>(i + 1u) * 2u);
            if (low >= 0xDC00u && low < 0xE000u) {
                c = 0x10000u + ((c - 0xD800u) << 10u) + (low - 0xDC00u);
                ++i;
            }
        }
        if (c < 0x80u) {
            text.push_back(static_cast<char>(c));
        } else if (c < 0x800u) {
            text.push_back(static_cast<char>(0xC0u | (c >> 6u)));
            text.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        } else if (c < 0x10000u) {
            text.push_back(static_cast<char>(0xE0u | (c >> 12u)));
            text.push_back(static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu)));
            text.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        } else {
            text.push_back(static_cast<char>(0xF0u | (c >> 18u)));
            text.push_back(static_cast<char>(0x80u | ((c >> 12u) & 0x3Fu)));
            text.push_back(static_cast<char>(0x80u | ((c >> 6u) & 0x3Fu)));
            text.push_back(static_cast<char>(0x80u | (c & 0x3Fu)));
        }
    }
    return text;
}

// Writes UTF-8 text as UTF-16 with a terminator, in at most `capacity`
// units; characters that do not fit are dropped.
void write_utf16(psprecomp::GuestMemory &memory, std::uint32_t address, const std::string &text,
                 std::uint32_t capacity) {
    if (address == 0u || capacity == 0u) return;
    std::vector<std::uint16_t> units;
    for (std::size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        int extra = byte >= 0xF0u ? 3 : byte >= 0xE0u ? 2 : byte >= 0xC0u ? 1 : 0;
        char32_t c = extra == 3 ? byte & 0x07u : extra == 2 ? byte & 0x0Fu : extra == 1 ? byte & 0x1Fu : byte;
        ++i;
        for (; extra > 0 && i < text.size(); --extra, ++i) c = (c << 6u) | (static_cast<unsigned char>(text[i]) & 0x3Fu);
        const std::size_t needed = c >= 0x10000u ? 2u : 1u;
        if (units.size() + needed > capacity - 1u) break;
        if (c >= 0x10000u) {
            units.push_back(static_cast<std::uint16_t>(0xD800u + ((c - 0x10000u) >> 10u)));
            units.push_back(static_cast<std::uint16_t>(0xDC00u + ((c - 0x10000u) & 0x3FFu)));
        } else {
            units.push_back(static_cast<std::uint16_t>(c));
        }
    }
    units.push_back(0u);
    for (std::size_t i = 0; i < units.size(); ++i)
        memory.store16(address + static_cast<std::uint32_t>(i) * 2u, units[i]);
}

std::uint32_t field_address(const psprecomp::GuestMemory &memory, std::uint32_t params) {
    if (params == 0u || memory.load32(params + osk::kFieldCount) == 0u) return 0u;
    return memory.load32(params + osk::kFields);
}

// The most characters the field takes: its limit, or what its buffer holds
// besides the terminator.
std::size_t field_max_length(const psprecomp::GuestMemory &memory, std::uint32_t field) {
    const std::uint32_t capacity = memory.load32(field + osk::kOutputLength);
    const std::uint32_t limit = memory.load32(field + osk::kOutputLimit);
    const std::uint32_t room = capacity > 0u ? capacity - 1u : 0u;
    return limit != 0u ? std::min(limit, room) : room;
}

void set_status(psprecomp::GuestMemory &memory, std::uint32_t status) {
    osk_state().status = status;
    if (osk_state().params != 0u) memory.store32(osk_state().params + osk::kState, status);
}

// Hands the answer to the game: the text in the field's buffer and the
// field's result, then QUIT.
void deliver(psprecomp::GuestMemory &memory) {
    OskState &state = osk_state();
    const OskAnswer answer = std::move(*state.answer);
    state.answer.reset();
    const std::uint32_t field = field_address(memory, state.params);
    if (field != 0u) {
        if (!answer.cancelled) {
            const auto units = static_cast<std::uint32_t>(field_max_length(memory, field) + 1u);
            write_utf16(memory, memory.load32(field + osk::kOutputText), answer.text, units);
        }
        memory.store32(field + osk::kResult, answer.cancelled ? osk::kResultCancelled : osk::kResultChanged);
    }
    memory.store32(state.params + dialog_common::kResultOffset, answer.cancelled ? osk::kDialogCancelled : 0u);
    set_status(memory, dialog_status::kQuit);
    std::cout << "[osk] " << (answer.cancelled ? "cancelled" : "entered \"" + answer.text + "\"") << std::endl;
}

// One poll: returns the status to report and moves the life cycle on.
std::uint32_t poll(psprecomp::GuestMemory &memory) {
    OskState &state = osk_state();
    const std::uint32_t reported = state.status;
    switch (state.status) {
    case dialog_status::kInit: set_status(memory, dialog_status::kVisible); break;
    case dialog_status::kVisible:
        if (state.answer) deliver(memory);
        break;
    case dialog_status::kFinished:
        set_status(memory, dialog_status::kNone);
        state.params = 0u;
        break;
    default: break;
    }
    return reported;
}

// Opens the port's keyboard for the request, or answers at once with the
// fixed name when that is chosen or there is no window to type in.
void answer_request(psprecomp::GuestMemory &memory, std::uint32_t field) {
    OskState &state = osk_state();
    const settings::Settings &s = settings::current();
    const std::string initial = field != 0u ? read_utf16(memory, memory.load32(field + osk::kInitialText)) : "";
    const std::size_t max_length = field != 0u ? field_max_length(memory, field) : 0u;
#if defined(MHP3RD_HAS_RENDERER)
    if (s.name_entry == settings::NameEntry::Keyboard && field != 0u && max_length > 0u) {
        ui::TextInputRequest request;
        request.prompt = read_utf16(memory, memory.load32(field + osk::kDescription));
        // The game passes no description; its own screen behind says what
        // the text is for.
        request.title = request.prompt.empty() ? "Enter a name" : request.prompt;
        if (request.prompt == request.title) request.prompt.clear();
        request.initial = initial;
        request.max_length = max_length;
        request.allowed = ui::hunter_name_character;
        const bool opened = ui::open_game_text_input(std::move(request), [](std::optional<std::string> text) {
            OskState &keyboard = osk_state();
            keyboard.answer = OskAnswer{!text.has_value(), text.value_or(std::string{})};
        });
        if (opened) {
            std::cout << "[osk] on-screen keyboard open, up to " << max_length << " characters" << std::endl;
            return;
        }
    }
#endif
    state.answer = OskAnswer{false, initial.empty() ? s.name : initial};
}

void register_osk(HleRegistrar &hle) {
    hle.add("sceUtility", "sceUtilityOskInitStart", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        OskState &state = osk_state();
        const std::uint32_t params = arg(ctx, 0);
        if (trace_osk()) {
            std::cout << "[osk-trace] InitStart " << psprecomp::hex32(params) << ", status " << state.status << "\n";
            trace_block(memory, params);
        }
        if (state.status != dialog_status::kNone) {
            kernel().finish(ctx, kErrorUtilityInvalidStatus);
            return;
        }
        state.params = params;
        state.answer.reset();
        set_status(memory, dialog_status::kInit);
        answer_request(memory, field_address(memory, params));
        kernel().finish(ctx, 0u);
    });

    hle.add("sceUtility", "sceUtilityOskUpdate", [](Runtime &rt, AllegrexContext &ctx) {
        if (trace_osk()) std::cout << "[osk-trace] Update(" << arg(ctx, 0) << "), status " << osk_state().status << "\n";
        if (osk_state().status == dialog_status::kVisible && osk_state().answer) deliver(rt.memory());
        kernel().finish(ctx, 0u);
    });

    hle.add("sceUtility", "sceUtilityOskGetStatus", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t status = poll(rt.memory());
        if (trace_osk()) std::cout << "[osk-trace] GetStatus -> " << status << "\n";
        kernel().finish(ctx, status);
    });

    hle.add("sceUtility", "sceUtilityOskShutdownStart", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        OskState &state = osk_state();
        if (trace_osk()) {
            std::cout << "[osk-trace] ShutdownStart, status " << state.status << "\n";
            trace_block(memory, state.params);
        }
        if (state.status != dialog_status::kQuit) {
            kernel().finish(ctx, kErrorUtilityInvalidStatus);
            return;
        }
        set_status(memory, dialog_status::kFinished);
        kernel().finish(ctx, 0u);
    });
}

// SceUtilityMsgDialogParams, after the common dialog header.
namespace msg {
constexpr std::uint32_t kMode = 0x34u;         // 0: error code, 1: text
constexpr std::uint32_t kErrorValue = 0x38u;
constexpr std::uint32_t kMessage = 0x3Cu;      // char[512], UTF-8
constexpr std::uint32_t kOptions = 0x23Cu;
constexpr std::uint32_t kButtonPressed = 0x240u;
constexpr std::uint32_t kMinimumSize = 0x244u;

constexpr std::uint32_t kModeError = 0u;
constexpr std::uint32_t kOptionYesNo = 0x10u;
constexpr std::uint32_t kOptionDefaultNo = 0x100u;
constexpr std::uint32_t kPressedYes = 1u;
} // namespace msg

DialogLifecycle &msg_dialog() {
    static DialogLifecycle dialog;
    return dialog;
}

// With no dialog UI, every message is answered at once as if the player
// pressed confirm: "OK" for a notice, "Yes" for a question. The text is
// logged so the conversation can be followed.
void register_msg_dialog(HleRegistrar &hle) {
    hle.add("sceUtility", "sceUtilityMsgDialogInitStart", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t params = arg(ctx, 0);
        const std::uint32_t size = memory.load32(params + dialog_common::kSizeOffset);
        const std::uint32_t mode = memory.load32(params + msg::kMode);
        const std::uint32_t options = size >= msg::kMinimumSize ? memory.load32(params + msg::kOptions) : 0u;
        if (mode == msg::kModeError) {
            std::cerr << "[msgdialog] error " << psprecomp::hex32(memory.load32(params + msg::kErrorValue)) << "\n";
        } else {
            std::cerr << "[msgdialog] \"" << read_cstring(memory, params + msg::kMessage, 512u) << "\""
                      << ((options & msg::kOptionYesNo) != 0u ? " [yes/no]" : "")
                      << ((options & msg::kOptionDefaultNo) != 0u ? " [default no]" : "") << " -> "
                      << ((options & msg::kOptionYesNo) != 0u ? "yes" : "ok") << "\n";
        }
        if (size >= msg::kMinimumSize) memory.store32(params + msg::kButtonPressed, msg::kPressedYes);
        memory.store32(params + dialog_common::kResultOffset, 0u);
        msg_dialog().start();
        kernel().finish(ctx, 0u);
    });

    hle.add("sceUtility", "sceUtilityMsgDialogUpdate", [](Runtime &, AllegrexContext &ctx) {
        (void)msg_dialog().poll();
        kernel().finish(ctx, 0u);
    });

    hle.add("sceUtility", "sceUtilityMsgDialogGetStatus", [](Runtime &, AllegrexContext &ctx) {
        kernel().finish(ctx, msg_dialog().poll());
    });

    hle.add("sceUtility", "sceUtilityMsgDialogShutdownStart", [](Runtime &, AllegrexContext &ctx) {
        if (!msg_dialog().active()) {
            kernel().finish(ctx, kErrorUtilityInvalidStatus);
            return;
        }
        if (!msg_dialog().shutdown()) std::cerr << "[msgdialog] ShutdownStart before the dialog finished\n";
        kernel().finish(ctx, 0u);
    });
}

} // namespace

void register_utility(HleRegistrar &hle, const std::filesystem::path &memory_stick) {
    register_osk(hle);
    register_msg_dialog(hle);
    register_savedata(hle, memory_stick);
}

} // namespace mhp3rd
