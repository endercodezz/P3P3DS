// sceNet, sceNetAdhoc, sceNetAdhocctl, sceNetAdhocDiscover, sceWlanDrv and the
// network configuration dialog (sceUtilityNetconf): PSP ad hoc play through a
// PSP ad hoc server (see adhoc/client.hpp).
//
// The game joins a group through the network configuration dialog, finds the
// other players with sceNetAdhocctlGetPeerList and talks to them over PDP
// datagrams and PTP streams. Every call here answers from the client's
// in-memory state; a call that blocks on the PSP parks the calling guest
// thread in a host wait that the kernel re-checks while other threads run,
// and ends it with the PSP's own timeout error when its time runs out.
// Emulated time is held to real time, so PSP timeouts last as long as on
// hardware. While the in-game menu is open no guest code runs: waits and their
// timeouts stand still, and the network thread keeps the server connection
// alive and buffers what arrives.
//
// MHP3RD_TRACE_ADHOC=1 logs every call with its arguments and result, and
// every packet header the client sends or receives.
#include "hle_common.hpp"

#include "adhoc/client.hpp"
#include "adhoc/session.hpp"
#include "settings/settings.hpp"
#include "utility_dialog.hpp"

#include "psprecomp/common.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <string>

namespace mhp3rd {
namespace {

using adhoc::Client;
using adhoc::Mac;

namespace err {
// sceNetAdhoc
constexpr std::uint32_t kInvalidSocketId = 0x80410701u;
constexpr std::uint32_t kInvalidAddr = 0x80410702u;
constexpr std::uint32_t kInvalidBufLen = 0x80410704u;
constexpr std::uint32_t kInvalidDataLen = 0x80410705u;
constexpr std::uint32_t kNotEnoughSpace = 0x80400706u;
constexpr std::uint32_t kWouldBlock = 0x80410709u;
constexpr std::uint32_t kPortInUse = 0x8041070Au;
constexpr std::uint32_t kNotConnected = 0x8041070Bu;
constexpr std::uint32_t kDisconnected = 0x8041070Cu;
constexpr std::uint32_t kNotListened = 0x8040070Eu;
constexpr std::uint32_t kInvalidArg = 0x80410711u;
constexpr std::uint32_t kNotInitialized = 0x80410712u;
constexpr std::uint32_t kAlreadyInitialized = 0x80410713u;
constexpr std::uint32_t kTimeout = 0x80410715u;
constexpr std::uint32_t kConnectionRefused = 0x80410718u;
// sceNetAdhocctl
constexpr std::uint32_t kCtlInvalidArg = 0x80410B04u;
constexpr std::uint32_t kCtlTimeout = 0x80410B05u;
constexpr std::uint32_t kCtlIdNotFound = 0x80410B06u;
constexpr std::uint32_t kCtlAlreadyInitialized = 0x80410B07u;
constexpr std::uint32_t kCtlNotInitialized = 0x80410B08u;
constexpr std::uint32_t kCtlBeaconLost = 0x80410B0Eu;
constexpr std::uint32_t kCtlTooManyHandlers = 0x80410B12u;
} // namespace err

// sceNetAdhocctl handler events and states.
constexpr std::uint32_t kEventError = 0u;
constexpr std::uint32_t kEventConnect = 1u;
constexpr std::uint32_t kEventDisconnect = 2u;
constexpr std::uint32_t kEventScan = 3u;
constexpr std::uint32_t kMaxHandlers = 4u;

// PTP states as sceNetAdhocGetPtpStat reports them.
constexpr std::uint32_t kPtpClosed = 0u;
constexpr std::uint32_t kPtpListen = 1u;
constexpr std::uint32_t kPtpSynSent = 2u;
constexpr std::uint32_t kPtpEstablished = 4u;

// Guest structure sizes.
constexpr std::uint32_t kPeerInfoSize = 152u;  // next, nickname[128], mac[6], pad[2], flags, u64 last seen
constexpr std::uint32_t kScanInfoSize = 28u;   // next, channel, group[8], bssid[6], pad[2], mode
// next, id, mac[6], peer mac[6], port, peer port, bytes waiting to be sent,
// bytes waiting to be received, state
constexpr std::uint32_t kPtpStatSize = 36u;
constexpr std::uint32_t kAdhocChannel = 1u;

// Netconf: SceUtilityNetconfParam after the common dialog header.
constexpr std::uint32_t kNetconfAction = 0x30u;
constexpr std::uint32_t kNetconfAdhocParam = 0x34u;
constexpr std::uint32_t kNetconfActionConnectAdhoc = 2u;
constexpr std::uint32_t kNetconfResultCancelled = 1u;

constexpr const char *kDefaultProduct = "ULJM05800";
constexpr std::uint32_t kErrorWaitTimeout = error::kWaitTimeout;

bool trace_adhoc() { return Client::tracing(); }

std::string describe_args(const AllegrexContext &ctx, unsigned count) {
    std::ostringstream text;
    for (unsigned i = 0; i < count; ++i) text << (i != 0u ? ", " : "") << psprecomp::hex32(arg(ctx, i));
    return text.str();
}

void trace_line(const std::string &line) {
    if (!trace_adhoc()) return;
    const Thread *thread = kernel().current_thread();
    Client::log("[adhoc] " + line + " thread=" + (thread != nullptr ? thread->name : std::string("interrupt")) +
                    " t=" + std::to_string(kernel().now_us() / 1000u) + "ms",
                true);
}

// Finishes an import and logs it.
void done(AllegrexContext &ctx, const char *name, unsigned argc, std::uint32_t result, const std::string &note = {}) {
    if (trace_adhoc())
        trace_line(std::string(name) + "(" + describe_args(ctx, argc) + ") = " + psprecomp::hex32(result) +
                   (note.empty() ? "" : " " + note) + " ra=" + psprecomp::hex32(ctx.gpr[31]));
    kernel().finish(ctx, result);
}

// Blocks the calling thread until `poll` has an answer. `timeout_us` of 0
// means no timeout, as for the PSP's ad hoc calls.
void block(AllegrexContext &ctx, const char *name, unsigned argc, std::uint32_t timeout_us, HostWaitPoll poll) {
    if (trace_adhoc())
        trace_line(std::string(name) + "(" + describe_args(ctx, argc) + ") waits" +
                   (timeout_us != 0u ? " up to " + std::to_string(timeout_us / 1000u) + "ms" : "") +
                   " ra=" + psprecomp::hex32(ctx.gpr[31]));
    std::string label = name;
    kernel().wait_host(ctx, timeout_us != 0u ? std::optional<std::uint64_t>(timeout_us) : std::nullopt,
                       [label, poll = std::move(poll)](bool timed_out) -> std::optional<std::uint32_t> {
                           const auto result = poll(timed_out);
                           if (timed_out && result && (*result == err::kTimeout || *result == kErrorWaitTimeout))
                               Client::get().note_timeout();
                           if (result && trace_adhoc())
                               trace_line(label + " ends = " + psprecomp::hex32(*result) +
                                          (timed_out ? " (timed out)" : ""));
                           return result;
                       });
}

Mac read_mac(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    Mac mac{};
    for (std::uint32_t i = 0; i < 6u; ++i) mac[i] = memory.load8(address + i);
    return mac;
}

void write_mac(psprecomp::GuestMemory &memory, std::uint32_t address, const Mac &mac) {
    for (std::uint32_t i = 0; i < 6u; ++i) memory.store8(address + i, mac[i]);
}

std::optional<Mac> parse_mac(const std::string &text) {
    Mac mac{};
    unsigned values[6];
    if (std::sscanf(text.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4],
                    &values[5]) != 6)
        return std::nullopt;
    for (int i = 0; i < 6; ++i) {
        if (values[i] > 0xFFu) return std::nullopt;
        mac[i] = static_cast<std::uint8_t>(values[i]);
    }
    return mac;
}

// This player's virtual MAC: from settings, or made up once and kept there, so
// other players see the same address every session.
Mac own_mac() {
    static std::optional<Mac> cached;
    if (cached) return *cached;
    settings::Settings &s = settings::current();
    if (auto parsed = parse_mac(s.adhoc_mac); parsed && ((*parsed)[0] & 1u) == 0u) {
        cached = parsed;
        return *cached;
    }
    std::random_device random;
    Mac mac{};
    for (auto &byte : mac) byte = static_cast<std::uint8_t>(random());
    // Locally administered, unicast.
    mac[0] = static_cast<std::uint8_t>((mac[0] & 0xFCu) | 0x02u);
    if (mac[1] == 0u) mac[1] = 1u;
    cached = mac;
    if (settings::overridden_by("network.mac") == nullptr) {
        s.adhoc_mac = adhoc::format_mac(mac);
        settings::save();
    }
    return mac;
}

std::string nickname() {
    const settings::Settings &s = settings::current();
    std::string name = !s.adhoc_nickname.empty() ? s.adhoc_nickname : s.name;
    if (name.empty()) name = "Yakumo";
    return name.substr(0, 127);
}

struct Handler {
    std::uint32_t function{};
    std::uint32_t argument{};
};

struct PdpSocket {
    int handle{};
    std::uint16_t port{};
    std::uint32_t buffer{};
};

struct PtpSocket {
    int handle{};
    bool listener{};
};

struct NetconfState {
    DialogLifecycle dialog;
    std::uint32_t params{};
    bool joining{};
    std::uint32_t result{};
};

struct State {
    bool adhoc_initialized{};
    bool ctl_initialized{};
    std::string product{kDefaultProduct};
    std::map<std::uint32_t, Handler> handlers;
    std::uint32_t next_handler{1u};
    std::map<std::uint32_t, PdpSocket> pdp;
    std::map<std::uint32_t, PtpSocket> ptp;
    NetconfState netconf;
    bool hooked{};
};

State &state() {
    static State value;
    return value;
}

void start_client() {
    const settings::Settings &s = settings::current();
    adhoc::Identity identity;
    identity.server = s.adhoc ? adhoc_server_address() : std::string{};
    identity.nickname = nickname();
    identity.mac = own_mac();
    identity.product = state().product;
    Client::get().start(identity);
}

void notify_handlers(std::uint32_t event, std::uint32_t error) {
    for (const auto &[id, handler] : state().handlers) {
        if (trace_adhoc())
            trace_line("handler " + std::to_string(id) + " at " + psprecomp::hex32(handler.function) + " gets event " +
                       std::to_string(event) + " error " + psprecomp::hex32(error));
        InterruptCall call{};
        call.function = handler.function;
        call.arguments = {event, error, handler.argument, 0u};
        kernel().queue_interrupt(std::move(call));
    }
}

void finish_netconf(std::uint32_t result) {
    NetconfState &netconf = state().netconf;
    if (!netconf.joining) return;
    netconf.joining = false;
    netconf.result = result;
    trace_line("netconf finished, result " + psprecomp::hex32(result));
}

// Moves the client's events to the game: handler calls and the dialog.
void pump_events() {
    for (const adhoc::CtlEvent event : Client::get().take_events()) {
        switch (event) {
        case adhoc::CtlEvent::Connected:
            notify_handlers(kEventConnect, 0u);
            finish_netconf(0u);
            break;
        case adhoc::CtlEvent::Disconnected:
            notify_handlers(kEventDisconnect, 0u);
            break;
        case adhoc::CtlEvent::ScanComplete:
            notify_handlers(kEventScan, 0u);
            break;
        case adhoc::CtlEvent::Error:
            if (state().netconf.joining) {
                finish_netconf(err::kCtlTimeout);
                notify_handlers(kEventError, err::kCtlTimeout);
            } else {
                notify_handlers(kEventError, err::kCtlBeaconLost);
            }
            break;
        }
    }
}

void ensure_hooked() {
    if (state().hooked) return;
    state().hooked = true;
    kernel().add_vblank_hook(pump_events);
}

// sceNet and sceWlanDrv ------------------------------------------------------

void register_net(HleRegistrar &hle) {
    hle.add("sceNet", "sceNetInit", [](Runtime &, AllegrexContext &ctx) { done(ctx, "sceNetInit", 5, 0u); });
    hle.add("sceNet", "sceNetTerm", [](Runtime &, AllegrexContext &ctx) { done(ctx, "sceNetTerm", 0, 0u); });
    hle.add("sceNet", "sceNetFreeThreadinfo",
            [](Runtime &, AllegrexContext &ctx) { done(ctx, "sceNetFreeThreadinfo", 1, 0u); });
    hle.add("sceNet", "sceNetGetLocalEtherAddr", [](Runtime &rt, AllegrexContext &ctx) {
        if (arg(ctx, 0) != 0u) write_mac(rt.memory(), arg(ctx, 0), own_mac());
        done(ctx, "sceNetGetLocalEtherAddr", 1, 0u, adhoc::format_mac(own_mac()));
    });
    hle.add("sceWlanDrv", "sceWlanGetEtherAddr", [](Runtime &rt, AllegrexContext &ctx) {
        if (arg(ctx, 0) != 0u) write_mac(rt.memory(), arg(ctx, 0), own_mac());
        done(ctx, "sceWlanGetEtherAddr", 1, 0u, adhoc::format_mac(own_mac()));
    });
    // The wireless switch is the ad hoc setting: off, the game says so itself.
    hle.add("sceWlanDrv", "sceWlanGetSwitchState", [](Runtime &, AllegrexContext &ctx) {
        done(ctx, "sceWlanGetSwitchState", 0, settings::current().adhoc ? 1u : 0u);
    });
}

// sceNetAdhocctl ---------------------------------------------------------------

void register_adhocctl(HleRegistrar &hle) {
    hle.add("sceNetAdhocctl", "sceNetAdhocctlInit", [](Runtime &rt, AllegrexContext &ctx) {
        State &s = state();
        if (s.ctl_initialized) return done(ctx, "sceNetAdhocctlInit", 3, err::kCtlAlreadyInitialized);
        // SceNetAdhocctlAdhocId: i32 type, char product[9].
        if (arg(ctx, 2) != 0u) {
            const std::string product = read_cstring(rt.memory(), arg(ctx, 2) + 4u, 9u);
            if (product.size() == 9u) s.product = product;
        }
        s.ctl_initialized = true;
        ensure_hooked();
        start_client();
        done(ctx, "sceNetAdhocctlInit", 3, 0u, "product " + s.product);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlTerm", [](Runtime &, AllegrexContext &ctx) {
        State &s = state();
        if (s.ctl_initialized) Client::get().stop();
        s.ctl_initialized = false;
        s.handlers.clear();
        done(ctx, "sceNetAdhocctlTerm", 0, 0u);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlAddHandler", [](Runtime &, AllegrexContext &ctx) {
        State &s = state();
        if (arg(ctx, 0) == 0u) return done(ctx, "sceNetAdhocctlAddHandler", 2, err::kCtlInvalidArg);
        if (s.handlers.size() >= kMaxHandlers) return done(ctx, "sceNetAdhocctlAddHandler", 2, err::kCtlTooManyHandlers);
        const std::uint32_t id = s.next_handler++;
        s.handlers[id] = Handler{arg(ctx, 0), arg(ctx, 1)};
        done(ctx, "sceNetAdhocctlAddHandler", 2, id);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlDelHandler", [](Runtime &, AllegrexContext &ctx) {
        const bool found = state().handlers.erase(arg(ctx, 0)) != 0u;
        done(ctx, "sceNetAdhocctlDelHandler", 1, found ? 0u : err::kCtlIdNotFound);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlScan", [](Runtime &, AllegrexContext &ctx) {
        if (!state().ctl_initialized) return done(ctx, "sceNetAdhocctlScan", 0, err::kCtlNotInitialized);
        Client::get().scan();
        done(ctx, "sceNetAdhocctlScan", 0, 0u);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlGetScanInfo", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t length_address = arg(ctx, 0);
        const std::uint32_t buffer = arg(ctx, 1);
        if (!state().ctl_initialized) return done(ctx, "sceNetAdhocctlGetScanInfo", 2, err::kCtlNotInitialized);
        if (length_address == 0u) return done(ctx, "sceNetAdhocctlGetScanInfo", 2, err::kCtlInvalidArg);
        const auto groups = Client::get().scan_results();
        if (buffer == 0u) {
            memory.store32(length_address, static_cast<std::uint32_t>(groups.size()) * kScanInfoSize);
            return done(ctx, "sceNetAdhocctlGetScanInfo", 2, 0u, std::to_string(groups.size()) + " groups");
        }
        const std::uint32_t room = memory.load32(length_address) / kScanInfoSize;
        const auto count = std::min<std::uint32_t>(room, static_cast<std::uint32_t>(groups.size()));
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t entry = buffer + i * kScanInfoSize;
            memory.store32(entry, i + 1u < count ? entry + kScanInfoSize : 0u);
            memory.store32(entry + 4u, kAdhocChannel);
            for (std::uint32_t c = 0; c < 8u; ++c)
                memory.store8(entry + 8u + c, c < groups[i].name.size() ? static_cast<std::uint8_t>(groups[i].name[c]) : 0u);
            write_mac(memory, entry + 16u, groups[i].host);
            memory.store16(entry + 22u, 0u);
            memory.store32(entry + 24u, 1u);
        }
        memory.store32(length_address, count * kScanInfoSize);
        done(ctx, "sceNetAdhocctlGetScanInfo", 2, 0u, std::to_string(count) + " groups");
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlDisconnect", [](Runtime &, AllegrexContext &ctx) {
        if (!state().ctl_initialized) return done(ctx, "sceNetAdhocctlDisconnect", 0, err::kCtlNotInitialized);
        const bool was_connected = Client::get().in_group();
        Client::get().leave();
        pump_events();
        if (was_connected) notify_handlers(kEventDisconnect, 0u);
        done(ctx, "sceNetAdhocctlDisconnect", 0, 0u);
    });
    hle.add("sceNetAdhocctl", "sceNetAdhocctlGetPeerList", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t length_address = arg(ctx, 0);
        const std::uint32_t buffer = arg(ctx, 1);
        pump_events();
        if (!state().ctl_initialized) return done(ctx, "sceNetAdhocctlGetPeerList", 2, err::kCtlNotInitialized);
        if (length_address == 0u) return done(ctx, "sceNetAdhocctlGetPeerList", 2, err::kCtlInvalidArg);
        const auto peers = Client::get().peers();
        if (buffer == 0u) {
            memory.store32(length_address, static_cast<std::uint32_t>(peers.size()) * kPeerInfoSize);
            return done(ctx, "sceNetAdhocctlGetPeerList", 2, 0u, std::to_string(peers.size()) + " peers");
        }
        const std::uint32_t room = memory.load32(length_address) / kPeerInfoSize;
        const auto count = std::min<std::uint32_t>(room, static_cast<std::uint32_t>(peers.size()));
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint32_t entry = buffer + i * kPeerInfoSize;
            memory.store32(entry, i + 1u < count ? entry + kPeerInfoSize : 0u);
            write_cstring(memory, entry + 4u, peers[i].nickname, 128u);
            write_mac(memory, entry + 132u, peers[i].mac);
            memory.store16(entry + 138u, 0u);
            memory.store32(entry + 140u, 0u);
            // Last time the peer was heard from: the relay keeps it alive, so now.
            store64(memory, entry + 144u, kernel().now_us());
        }
        memory.store32(length_address, count * kPeerInfoSize);
        done(ctx, "sceNetAdhocctlGetPeerList", 2, 0u, std::to_string(count) + " peers");
    });
}

// sceNetAdhoc: PDP ---------------------------------------------------------------

PdpSocket *find_pdp(std::uint32_t id) {
    const auto found = state().pdp.find(id);
    return found != state().pdp.end() ? &found->second : nullptr;
}

PtpSocket *find_ptp(std::uint32_t id) {
    const auto found = state().ptp.find(id);
    return found != state().ptp.end() ? &found->second : nullptr;
}

void close_all_sockets() {
    for (const auto &[id, socket] : state().pdp) Client::get().pdp_close(socket.handle);
    for (const auto &[id, socket] : state().ptp) Client::get().ptp_close(socket.handle);
    state().pdp.clear();
    state().ptp.clear();
}

// Delivers the next datagram of `socket` into the guest's buffers, or returns
// nothing when none is waiting.
std::optional<std::uint32_t> receive_datagram(psprecomp::GuestMemory &memory, const PdpSocket &socket,
                                              std::uint32_t mac_address, std::uint32_t port_address,
                                              std::uint32_t buffer, std::uint32_t length_address) {
    const auto size = Client::get().pdp_peek(socket.handle);
    if (!size) return std::nullopt;
    const std::uint32_t room = memory.load32(length_address);
    if (*size > room) {
        // The datagram stays queued; the caller learns how much room it needs.
        memory.store32(length_address, static_cast<std::uint32_t>(*size));
        return err::kNotEnoughSpace;
    }
    const auto datagram = Client::get().pdp_receive(socket.handle);
    if (!datagram) return std::nullopt;
    for (std::size_t i = 0; i < datagram->data.size(); ++i)
        memory.store8(buffer + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>(datagram->data[i]));
    memory.store32(length_address, static_cast<std::uint32_t>(datagram->data.size()));
    if (mac_address != 0u) write_mac(memory, mac_address, datagram->source);
    if (port_address != 0u) memory.store16(port_address, datagram->port);
    return 0u;
}

void register_pdp(HleRegistrar &hle) {
    hle.add("sceNetAdhoc", "sceNetAdhocInit", [](Runtime &, AllegrexContext &ctx) {
        if (state().adhoc_initialized) return done(ctx, "sceNetAdhocInit", 0, err::kAlreadyInitialized);
        state().adhoc_initialized = true;
        ensure_hooked();
        done(ctx, "sceNetAdhocInit", 0, 0u);
    });
    hle.add("sceNetAdhoc", "sceNetAdhocTerm", [](Runtime &, AllegrexContext &ctx) {
        close_all_sockets();
        state().adhoc_initialized = false;
        done(ctx, "sceNetAdhocTerm", 0, 0u);
    });
    hle.add("sceNetAdhoc", "sceNetAdhocPdpCreate", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPdpCreate";
        if (!state().adhoc_initialized) return done(ctx, name, 4, err::kNotInitialized);
        const std::uint32_t mac_address = arg(ctx, 0);
        const auto port = static_cast<std::uint16_t>(arg(ctx, 1));
        const std::uint32_t buffer = arg(ctx, 2);
        if (mac_address == 0u) return done(ctx, name, 4, err::kInvalidAddr);
        if (buffer == 0u) return done(ctx, name, 4, err::kInvalidBufLen);
        const Mac mac = read_mac(rt.memory(), mac_address);
        if (mac != own_mac()) return done(ctx, name, 4, err::kInvalidAddr, adhoc::format_mac(mac) + " is not ours");
        const int handle = Client::get().pdp_open(port, buffer);
        if (handle == 0) return done(ctx, name, 4, err::kPortInUse);
        const auto id = static_cast<std::uint32_t>(handle);
        state().pdp[id] = PdpSocket{handle, port, buffer};
        done(ctx, name, 4, id);
    });
    hle.add("sceNetAdhoc", "sceNetAdhocPdpDelete", [](Runtime &, AllegrexContext &ctx) {
        PdpSocket *socket = find_pdp(arg(ctx, 0));
        if (socket == nullptr) return done(ctx, "sceNetAdhocPdpDelete", 2, err::kInvalidSocketId);
        Client::get().pdp_close(socket->handle);
        state().pdp.erase(arg(ctx, 0));
        done(ctx, "sceNetAdhocPdpDelete", 2, 0u);
    });
    // (id, mac*, port, data*, length, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPdpSend", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPdpSend";
        PdpSocket *socket = find_pdp(arg(ctx, 0));
        if (socket == nullptr) return done(ctx, name, 7, err::kInvalidSocketId);
        const std::uint32_t mac_address = arg(ctx, 1);
        const auto port = static_cast<std::uint16_t>(arg(ctx, 2));
        const std::uint32_t data = arg(ctx, 3);
        const std::uint32_t length = arg(ctx, 4);
        if (mac_address == 0u) return done(ctx, name, 7, err::kInvalidAddr);
        if (length > adhoc::relay::kPdpBlockMax || (length != 0u && data == 0u))
            return done(ctx, name, 7, err::kInvalidDataLen);
        const Mac destination = read_mac(rt.memory(), mac_address);
        std::string bytes(length, '\0');
        for (std::uint32_t i = 0; i < length; ++i) bytes[i] = static_cast<char>(rt.memory().load8(data + i));
        // The relay queues the datagram at once, so a send never has to wait.
        Client::get().pdp_send(socket->handle, destination, port, bytes.data(), bytes.size());
        done(ctx, name, 7, 0u, "to " + adhoc::format_mac(destination));
    });
    // (id, mac*, port*, data*, length*, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPdpRecv", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPdpRecv";
        const std::uint32_t id = arg(ctx, 0);
        const PdpSocket *socket = find_pdp(id);
        if (socket == nullptr) return done(ctx, name, 7, err::kInvalidSocketId);
        const std::uint32_t mac_address = arg(ctx, 1);
        const std::uint32_t port_address = arg(ctx, 2);
        const std::uint32_t buffer = arg(ctx, 3);
        const std::uint32_t length_address = arg(ctx, 4);
        const std::uint32_t timeout = arg(ctx, 5);
        const bool nonblock = arg(ctx, 6) != 0u;
        if (buffer == 0u || length_address == 0u) return done(ctx, name, 7, err::kInvalidArg);
        auto &memory = rt.memory();
        if (const auto result = receive_datagram(memory, *socket, mac_address, port_address, buffer, length_address))
            return done(ctx, name, 7, *result);
        if (nonblock) return done(ctx, name, 7, err::kWouldBlock);
        block(ctx, name, 7, timeout,
              [&memory, id, mac_address, port_address, buffer, length_address](bool timed_out)
                  -> std::optional<std::uint32_t> {
                  const PdpSocket *socket = find_pdp(id);
                  if (socket == nullptr) return err::kInvalidSocketId;
                  if (const auto result =
                          receive_datagram(memory, *socket, mac_address, port_address, buffer, length_address))
                      return result;
                  if (timed_out) return err::kTimeout;
                  return std::nullopt;
              });
    });
}

// sceNetAdhoc: PTP ---------------------------------------------------------------

std::uint32_t stream_error(const adhoc::StreamInfo &info) {
    switch (info.state) {
    case adhoc::StreamState::Disconnected: return err::kDisconnected;
    case adhoc::StreamState::Failed: return err::kConnectionRefused;
    default: return err::kNotConnected;
    }
}

std::optional<std::uint32_t> try_stream_receive(psprecomp::GuestMemory &memory, int handle, std::uint32_t buffer,
                                                std::uint32_t length_address) {
    const adhoc::StreamInfo info = Client::get().ptp_info(handle);
    const std::uint32_t room = memory.load32(length_address);
    if (info.readable != 0u) {
        std::string bytes(std::min<std::size_t>(room, info.readable), '\0');
        const std::size_t count = Client::get().ptp_receive(handle, bytes.data(), bytes.size());
        for (std::size_t i = 0; i < count; ++i)
            memory.store8(buffer + static_cast<std::uint32_t>(i), static_cast<std::uint8_t>(bytes[i]));
        memory.store32(length_address, static_cast<std::uint32_t>(count));
        return 0u;
    }
    if (info.state != adhoc::StreamState::Established) return stream_error(info);
    return std::nullopt;
}

std::optional<std::uint32_t> try_stream_send(psprecomp::GuestMemory &memory, int handle, std::uint32_t data,
                                             std::uint32_t length_address) {
    const adhoc::StreamInfo info = Client::get().ptp_info(handle);
    if (info.state != adhoc::StreamState::Established) return stream_error(info);
    const std::uint32_t length = memory.load32(length_address);
    if (length == 0u) return 0u;
    const std::size_t room = info.unsent >= info.capacity ? 0u : info.capacity - info.unsent;
    if (room == 0u) return std::nullopt;
    std::string bytes(std::min<std::size_t>(room, length), '\0');
    for (std::size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = static_cast<char>(memory.load8(data + static_cast<std::uint32_t>(i)));
    const std::size_t sent = Client::get().ptp_send(handle, bytes.data(), bytes.size());
    if (sent == 0u) return std::nullopt;
    memory.store32(length_address, static_cast<std::uint32_t>(sent));
    return 0u;
}

void register_ptp(HleRegistrar &hle) {
    // (srcmac*, srcport, dstmac*, dstport, buffer, retry interval, retry count, unused)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpOpen", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpOpen";
        if (!state().adhoc_initialized) return done(ctx, name, 8, err::kNotInitialized);
        if (arg(ctx, 0) == 0u || arg(ctx, 2) == 0u) return done(ctx, name, 8, err::kInvalidAddr);
        if (read_mac(rt.memory(), arg(ctx, 0)) != own_mac()) return done(ctx, name, 8, err::kInvalidAddr);
        const Mac peer = read_mac(rt.memory(), arg(ctx, 2));
        if (peer == adhoc::kBroadcastMac || peer == own_mac()) return done(ctx, name, 8, err::kInvalidAddr);
        if (arg(ctx, 4) == 0u) return done(ctx, name, 8, err::kInvalidBufLen);
        const int handle = Client::get().ptp_open(static_cast<std::uint16_t>(arg(ctx, 1)), peer,
                                                  static_cast<std::uint16_t>(arg(ctx, 3)), arg(ctx, 4), arg(ctx, 5),
                                                  arg(ctx, 6));
        if (handle == 0) return done(ctx, name, 8, err::kPortInUse);
        const auto id = static_cast<std::uint32_t>(handle);
        state().ptp[id] = PtpSocket{handle, false};
        done(ctx, name, 8, id, "to " + adhoc::format_mac(peer));
    });
    // (id, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpConnect", [](Runtime &, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpConnect";
        const std::uint32_t id = arg(ctx, 0);
        const PtpSocket *socket = find_ptp(id);
        if (socket == nullptr || socket->listener) return done(ctx, name, 3, err::kInvalidSocketId);
        const auto check = [id](bool timed_out) -> std::optional<std::uint32_t> {
            const PtpSocket *socket = find_ptp(id);
            if (socket == nullptr) return err::kInvalidSocketId;
            const adhoc::StreamInfo info = Client::get().ptp_info(socket->handle);
            switch (info.state) {
            case adhoc::StreamState::Established: return 0u;
            case adhoc::StreamState::Opening: break;
            default: return stream_error(info);
            }
            if (timed_out) return err::kTimeout;
            return std::nullopt;
        };
        if (const auto result = check(false)) return done(ctx, name, 3, *result);
        if (arg(ctx, 2) != 0u) return done(ctx, name, 3, err::kWouldBlock);
        block(ctx, name, 3, arg(ctx, 1), check);
    });
    // (srcmac*, srcport, buffer, retry interval, retry count, backlog, unused)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpListen", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpListen";
        if (!state().adhoc_initialized) return done(ctx, name, 7, err::kNotInitialized);
        if (arg(ctx, 0) == 0u || read_mac(rt.memory(), arg(ctx, 0)) != own_mac())
            return done(ctx, name, 7, err::kInvalidAddr);
        if (arg(ctx, 2) == 0u) return done(ctx, name, 7, err::kInvalidBufLen);
        const int handle =
            Client::get().ptp_listen(static_cast<std::uint16_t>(arg(ctx, 1)), arg(ctx, 2), std::max(arg(ctx, 5), 1u));
        if (handle == 0) return done(ctx, name, 7, err::kPortInUse);
        const auto id = static_cast<std::uint32_t>(handle);
        state().ptp[id] = PtpSocket{handle, true};
        done(ctx, name, 7, id);
    });
    // (id, mac*, port*, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpAccept", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpAccept";
        const std::uint32_t id = arg(ctx, 0);
        const PtpSocket *socket = find_ptp(id);
        if (socket == nullptr) return done(ctx, name, 5, err::kInvalidSocketId);
        if (!socket->listener) return done(ctx, name, 5, err::kNotListened);
        auto &memory = rt.memory();
        const std::uint32_t mac_address = arg(ctx, 1);
        const std::uint32_t port_address = arg(ctx, 2);
        const auto check = [&memory, id, mac_address, port_address](bool timed_out) -> std::optional<std::uint32_t> {
            const PtpSocket *socket = find_ptp(id);
            if (socket == nullptr) return err::kInvalidSocketId;
            const int accepted = Client::get().ptp_accept(socket->handle);
            if (accepted != 0) {
                const adhoc::StreamInfo info = Client::get().ptp_info(accepted);
                const auto new_id = static_cast<std::uint32_t>(accepted);
                state().ptp[new_id] = PtpSocket{accepted, false};
                if (mac_address != 0u) write_mac(memory, mac_address, info.peer);
                if (port_address != 0u) memory.store16(port_address, info.peer_port);
                return new_id;
            }
            if (timed_out) return err::kTimeout;
            return std::nullopt;
        };
        if (const auto result = check(false)) return done(ctx, name, 5, *result);
        if (arg(ctx, 4) != 0u) return done(ctx, name, 5, err::kWouldBlock);
        block(ctx, name, 5, arg(ctx, 3), check);
    });
    // (id, data*, length*, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpSend", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpSend";
        const std::uint32_t id = arg(ctx, 0);
        const PtpSocket *socket = find_ptp(id);
        if (socket == nullptr || socket->listener) return done(ctx, name, 5, err::kInvalidSocketId);
        if (arg(ctx, 1) == 0u || arg(ctx, 2) == 0u) return done(ctx, name, 5, err::kInvalidArg);
        auto &memory = rt.memory();
        const std::uint32_t data = arg(ctx, 1);
        const std::uint32_t length_address = arg(ctx, 2);
        if (const auto result = try_stream_send(memory, socket->handle, data, length_address))
            return done(ctx, name, 5, *result, "len " + std::to_string(memory.load32(length_address)));
        if (arg(ctx, 4) != 0u) return done(ctx, name, 5, err::kWouldBlock);
        block(ctx, name, 5, arg(ctx, 3),
              [&memory, id, data, length_address](bool timed_out) -> std::optional<std::uint32_t> {
                  const PtpSocket *socket = find_ptp(id);
                  if (socket == nullptr) return err::kInvalidSocketId;
                  if (const auto result = try_stream_send(memory, socket->handle, data, length_address)) return result;
                  if (timed_out) return err::kTimeout;
                  return std::nullopt;
              });
    });
    // (id, data*, length*, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpRecv", [](Runtime &rt, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpRecv";
        const std::uint32_t id = arg(ctx, 0);
        const PtpSocket *socket = find_ptp(id);
        if (socket == nullptr || socket->listener) return done(ctx, name, 5, err::kInvalidSocketId);
        if (arg(ctx, 1) == 0u || arg(ctx, 2) == 0u) return done(ctx, name, 5, err::kInvalidArg);
        auto &memory = rt.memory();
        const std::uint32_t buffer = arg(ctx, 1);
        const std::uint32_t length_address = arg(ctx, 2);
        const std::uint32_t asked = memory.load32(length_address);
        if (const auto result = try_stream_receive(memory, socket->handle, buffer, length_address))
            return done(ctx, name, 5, *result,
                        "asked " + std::to_string(asked) + " got " + std::to_string(memory.load32(length_address)));
        if (arg(ctx, 4) != 0u) return done(ctx, name, 5, err::kWouldBlock);
        block(ctx, name, 5, arg(ctx, 3),
              [&memory, id, buffer, length_address](bool timed_out) -> std::optional<std::uint32_t> {
                  const PtpSocket *socket = find_ptp(id);
                  if (socket == nullptr) return err::kInvalidSocketId;
                  if (const auto result = try_stream_receive(memory, socket->handle, buffer, length_address))
                      return result;
                  if (timed_out) return err::kTimeout;
                  return std::nullopt;
              });
    });
    // (id, timeout, nonblock)
    hle.add("sceNetAdhoc", "sceNetAdhocPtpFlush", [](Runtime &, AllegrexContext &ctx) {
        const char *name = "sceNetAdhocPtpFlush";
        const std::uint32_t id = arg(ctx, 0);
        const PtpSocket *socket = find_ptp(id);
        if (socket == nullptr || socket->listener) return done(ctx, name, 3, err::kInvalidSocketId);
        const auto check = [id](bool timed_out) -> std::optional<std::uint32_t> {
            const PtpSocket *socket = find_ptp(id);
            if (socket == nullptr) return err::kInvalidSocketId;
            const adhoc::StreamInfo info = Client::get().ptp_info(socket->handle);
            if (info.state != adhoc::StreamState::Established) return stream_error(info);
            if (info.unsent == 0u) return 0u;
            if (timed_out) return err::kTimeout;
            return std::nullopt;
        };
        if (const auto result = check(false)) return done(ctx, name, 3, *result);
        if (arg(ctx, 2) != 0u) return done(ctx, name, 3, err::kWouldBlock);
        block(ctx, name, 3, arg(ctx, 1), check);
    });
    hle.add("sceNetAdhoc", "sceNetAdhocPtpClose", [](Runtime &, AllegrexContext &ctx) {
        const PtpSocket *socket = find_ptp(arg(ctx, 0));
        if (socket == nullptr) return done(ctx, "sceNetAdhocPtpClose", 2, err::kInvalidSocketId);
        Client::get().ptp_close(socket->handle);
        state().ptp.erase(arg(ctx, 0));
        done(ctx, "sceNetAdhocPtpClose", 2, 0u);
    });
    // (size*, buffer*)
    hle.add("sceNetAdhoc", "sceNetAdhocGetPtpStat", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t size_address = arg(ctx, 0);
        const std::uint32_t buffer = arg(ctx, 1);
        if (size_address == 0u) return done(ctx, "sceNetAdhocGetPtpStat", 2, err::kInvalidArg);
        const auto count_all = static_cast<std::uint32_t>(state().ptp.size());
        if (buffer == 0u) {
            memory.store32(size_address, count_all * kPtpStatSize);
            return done(ctx, "sceNetAdhocGetPtpStat", 2, 0u);
        }
        const std::uint32_t room = memory.load32(size_address) / kPtpStatSize;
        std::uint32_t index = 0;
        const Mac mac = own_mac();
        for (const auto &[id, socket] : state().ptp) {
            if (index >= room) break;
            const adhoc::StreamInfo info = Client::get().ptp_info(socket.handle);
            const std::uint32_t entry = buffer + index * kPtpStatSize;
            const bool last = index + 1u >= std::min(room, count_all);
            memory.store32(entry, last ? 0u : entry + kPtpStatSize);
            memory.store32(entry + 4u, id);
            write_mac(memory, entry + 8u, mac);
            write_mac(memory, entry + 14u, info.peer);
            memory.store16(entry + 20u, info.local_port);
            memory.store16(entry + 22u, info.peer_port);
            // What the socket buffers hold now, not running totals: the game
            // reads these as buffer occupancy, and totals that only grow
            // made the quest host close the stream after about 1 KB.
            memory.store32(entry + 24u, static_cast<std::uint32_t>(info.unsent_data));
            memory.store32(entry + 28u, static_cast<std::uint32_t>(socket.listener ? 0u : info.readable));
            std::uint32_t status = kPtpClosed;
            if (info.state == adhoc::StreamState::Listening) status = kPtpListen;
            else if (info.state == adhoc::StreamState::Opening) status = kPtpSynSent;
            else if (info.state == adhoc::StreamState::Established) status = kPtpEstablished;
            memory.store32(entry + 32u, status);
            ++index;
        }
        memory.store32(size_address, index * kPtpStatSize);
        done(ctx, "sceNetAdhocGetPtpStat", 2, 0u, std::to_string(index) + " sockets");
    });
}

// sceNetAdhocDiscover ----------------------------------------------------------
//
// Discovery finds a nearby console for a one-to-one session with no group; it
// is not part of joining a gathering hall. Starting it succeeds, and it ends at
// once with nothing found.
void register_discover(HleRegistrar &hle) {
    constexpr std::uint32_t kDiscoverNone = 0u;
    constexpr std::uint32_t kDiscoverCompleted = 2u;
    static std::uint32_t status = kDiscoverNone;
    hle.add("sceNetAdhocDiscover", "sceNetAdhocDiscoverInitStart", [](Runtime &, AllegrexContext &ctx) {
        status = kDiscoverCompleted;
        done(ctx, "sceNetAdhocDiscoverInitStart", 1, 0u);
    });
    hle.add("sceNetAdhocDiscover", "sceNetAdhocDiscoverUpdate",
            [](Runtime &, AllegrexContext &ctx) { done(ctx, "sceNetAdhocDiscoverUpdate", 0, 0u); });
    hle.add("sceNetAdhocDiscover", "sceNetAdhocDiscoverGetStatus",
            [](Runtime &, AllegrexContext &ctx) { done(ctx, "sceNetAdhocDiscoverGetStatus", 0, status); });
    hle.add("sceNetAdhocDiscover", "sceNetAdhocDiscoverStop", [](Runtime &, AllegrexContext &ctx) {
        status = kDiscoverNone;
        done(ctx, "sceNetAdhocDiscoverStop", 0, 0u);
    });
    hle.add("sceNetAdhocDiscover", "sceNetAdhocDiscoverTerm", [](Runtime &, AllegrexContext &ctx) {
        status = kDiscoverNone;
        done(ctx, "sceNetAdhocDiscoverTerm", 0, 0u);
    });
}

// sceUtilityNetconf --------------------------------------------------------------
//
// The dialog a PSP shows while it connects. For ad hoc it joins the group the
// game names, and stays visible until the server confirms the join or the
// client gives up; its result is 0 on success. Nothing is drawn: the game
// keeps drawing its own screen behind it.
void register_netconf(HleRegistrar &hle) {
    hle.add("sceUtility", "sceUtilityNetconfInitStart", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        NetconfState &netconf = state().netconf;
        const std::uint32_t params = arg(ctx, 0);
        if (params == 0u) return done(ctx, "sceUtilityNetconfInitStart", 1, err::kInvalidArg);
        if (netconf.dialog.active()) return done(ctx, "sceUtilityNetconfInitStart", 1, kErrorUtilityInvalidStatus);
        netconf.params = params;
        netconf.dialog.start();
        netconf.result = kNetconfResultCancelled;
        netconf.joining = false;
        const std::uint32_t action = memory.load32(params + kNetconfAction);
        const std::uint32_t adhoc_param = memory.load32(params + kNetconfAdhocParam);
        std::string note = "action " + std::to_string(action);
        if (action == kNetconfActionConnectAdhoc && adhoc_param != 0u) {
            const std::string group = read_cstring(memory, adhoc_param, 8u);
            note += " group \"" + group + "\" timeout " + std::to_string(memory.load32(adhoc_param + 8u));
            ensure_hooked();
            if (!settings::current().adhoc || adhoc_server_address().empty()) {
                netconf.result = err::kCtlTimeout;
                Client::log("[adhoc] no session to join and no server is set up (menu: Network); the connection "
                            "fails",
                            true);
            } else {
                start_client();
                (void)Client::get().take_events();
                netconf.joining = true;
                Client::get().join(group);
            }
        }
        done(ctx, "sceUtilityNetconfInitStart", 1, 0u, note);
    });
    const auto status = [](psprecomp::GuestMemory &memory) {
        NetconfState &netconf = state().netconf;
        pump_events();
        // Visible until the join is over, then the usual steps to QUIT.
        if (netconf.joining && netconf.dialog.status() == dialog_status::kVisible) return dialog_status::kVisible;
        const std::uint32_t reported = netconf.dialog.poll();
        if (reported == dialog_status::kQuit && netconf.params != 0u)
            memory.store32(netconf.params + dialog_common::kResultOffset, netconf.result);
        return reported;
    };
    hle.add("sceUtility", "sceUtilityNetconfUpdate", [status](Runtime &rt, AllegrexContext &ctx) {
        (void)status(rt.memory());
        if (trace_adhoc() && !state().netconf.joining) done(ctx, "sceUtilityNetconfUpdate", 1, 0u);
        else kernel().finish(ctx, 0u);
    });
    hle.add("sceUtility", "sceUtilityNetconfGetStatus", [status](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t reported = status(rt.memory());
        if (state().netconf.joining) kernel().finish(ctx, reported);
        else done(ctx, "sceUtilityNetconfGetStatus", 0, reported);
    });
    hle.add("sceUtility", "sceUtilityNetconfShutdownStart", [](Runtime &, AllegrexContext &ctx) {
        NetconfState &netconf = state().netconf;
        if (!netconf.dialog.active()) return done(ctx, "sceUtilityNetconfShutdownStart", 0, kErrorUtilityInvalidStatus);
        if (netconf.joining) {
            // Cancelled while joining.
            netconf.joining = false;
            Client::get().leave();
        }
        (void)netconf.dialog.shutdown();
        done(ctx, "sceUtilityNetconfShutdownStart", 0, 0u);
    });
}

} // namespace

void adhoc_apply_settings(bool switch_now) {
    if (!state().ctl_initialized) return;
    if (!settings::current().adhoc || adhoc_server_address().empty()) {
        // Off line now: the client reports the lost group to the game.
        start_client();
        return;
    }
    // A running session keeps its server and name until the game goes on line
    // again, unless the player picked another session to be in.
    if (switch_now || Client::get().server_state() == adhoc::ServerState::Off) start_client();
}

std::string adhoc_player_name() { return nickname(); }

bool adhoc_networking_on() { return state().ctl_initialized || state().adhoc_initialized; }

bool adhoc_session_active() {
    const adhoc::Diagnostics d = Client::get().diagnostics();
    return adhoc_hosting() || Client::get().in_group() || d.joining.has_value() || d.rejoin_ms.has_value();
}

void register_adhoc(HleRegistrar &hle) {
    register_net(hle);
    register_adhocctl(hle);
    register_pdp(hle);
    register_ptp(hle);
    register_discover(hle);
    register_netconf(hle);
}

} // namespace mhp3rd
