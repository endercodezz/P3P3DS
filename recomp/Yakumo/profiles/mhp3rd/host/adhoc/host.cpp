// Hosting an ad hoc session from the game: the built-in server, announced on
// the local network, with this instance's own client connected to it over
// the loopback address.
#include "adhoc/client.hpp"
#include "adhoc/discovery.hpp"
#include "adhoc/server.hpp"
#include "adhoc/session.hpp"
#include "settings/settings.hpp"

#include <algorithm>
#include <mutex>

namespace mhp3rd {
namespace {

// The product code other players see in the announcement. Players of the
// game's PSP release log in with the same one.
constexpr const char *kProduct = "ULJM05800";
constexpr std::size_t kRecentAddresses = 5u;

struct HostState {
    std::mutex mutex;
    adhoc::Server server;
    std::string error;
    std::uint16_t port{};
};

// Never destroyed: the discovery thread asks the server for its player count
// until the process ends, and a server still running at exit simply ends
// with it, which its players see as a lost connection.
HostState &host() {
    static HostState *state = new HostState;
    return *state;
}

void remember(const std::string &address) {
    settings::Settings &s = settings::current();
    auto &recent = s.adhoc_recent;
    recent.erase(std::remove(recent.begin(), recent.end(), address), recent.end());
    recent.insert(recent.begin(), address);
    if (recent.size() > kRecentAddresses) recent.resize(kRecentAddresses);
}

} // namespace

std::string adhoc_server_address() {
    if (adhoc_hosting()) {
        const std::uint16_t port = host().port;
        return port == adhoc::kAdhocctlPort ? std::string("127.0.0.1") : "127.0.0.1:" + std::to_string(port);
    }
    return settings::current().adhoc_server;
}

bool adhoc_host_start() {
    HostState &state = host();
    {
        std::lock_guard lock(state.mutex);
        if (state.server.running()) return true;
        adhoc::ServerConfig config;
        config.adhocctl_port = static_cast<std::uint16_t>(settings::current().adhoc_host_port);
        if (!state.server.start(config)) {
            state.error = state.server.status().error;
            return false;
        }
        state.error.clear();
        state.port = config.adhocctl_port;
        adhoc::Server *server = &state.server;
        adhoc::Discovery::get().start_announcing(config.adhocctl_port, [server] {
            adhoc::Announcement info;
            info.name = adhoc_player_name();
            info.product = kProduct;
            info.players = static_cast<unsigned>(server->status().players.size());
            return info;
        });
    }
    settings::Settings &s = settings::current();
    if (!s.adhoc && settings::overridden_by("network.adhoc") == nullptr) {
        s.adhoc = true;
        settings::save();
    }
    adhoc::Client::log("[adhoc] hosting a session", true);
    adhoc_apply_settings(true);
    return true;
}

void adhoc_host_stop() {
    HostState &state = host();
    {
        std::lock_guard lock(state.mutex);
        if (!state.server.running()) return;
        adhoc::Discovery::get().stop_announcing();
        state.server.stop();
        state.port = 0u;
    }
    adhoc::Client::log("[adhoc] stopped hosting", true);
    adhoc_apply_settings(true);
}

bool adhoc_hosting() { return host().server.running(); }

std::string adhoc_host_error() {
    std::lock_guard lock(host().mutex);
    return host().error;
}

adhoc::ServerStatus adhoc_host_status() { return host().server.status(); }

void adhoc_shutdown() noexcept {
    try {
        HostState &state = host();
        std::lock_guard lock(state.mutex);
        adhoc::Discovery::get().shutdown();
        state.server.stop();
        state.port = 0u;
    } catch (...) {
    }
    adhoc::Discovery::get().shutdown();
    adhoc::Client::get().shutdown();
}

void adhoc_join(const std::string &address) {
    if (address.empty()) return;
    if (adhoc_hosting()) {
        HostState &state = host();
        std::lock_guard lock(state.mutex);
        adhoc::Discovery::get().stop_announcing();
        state.server.stop();
        state.port = 0u;
    }
    settings::Settings &s = settings::current();
    if (settings::overridden_by("network.server") == nullptr) s.adhoc_server = address;
    if (settings::overridden_by("network.adhoc") == nullptr) s.adhoc = true;
    remember(address);
    settings::save();
    adhoc::Client::log("[adhoc] joining the session at " + address, true);
    adhoc_apply_settings(true);
}

} // namespace mhp3rd
