#pragma once

// A PSP ad hoc server: the adhocctl matchmaking service and the relay that
// carries the game's datagrams and streams (see protocol.hpp), so one player
// can host a session for the others with nothing else installed. Other
// clients of these protocols, such as PPSSPP, can use it too.
//
// It was written for this project from the protocols' documented and observed
// behaviour:
//
// adhocctl. A client logs in with its MAC, nickname and product code, then
// joins one group at a time. Joining tells the newcomer about everyone already
// in the group and everyone about the newcomer, then confirms the join with
// the group's host MAC (whoever created the group). Leaving, or losing the
// connection, tells the others. A scan lists the groups of the client's game,
// then says it is complete. A client silent for 30 s is dropped; a new login
// with a MAC already logged in replaces the older connection.
//
// Relay. Each connection opens with an init record naming one ad hoc socket.
// A datagram socket's datagrams go to the datagram socket with the MAC and
// port they name, or, sent to the broadcast MAC, to that port on every other
// member of the sender's group. A stream connection waits up to 5 s for a
// listening socket on its destination, tells that socket who is connecting,
// and then waits up to 5 s more for the accepting connection. Once paired,
// both sides are told and their data is passed through; when either closes,
// the other is closed after what it was sent has been delivered.
//
// Threading. start() binds the listening sockets on the calling thread, which
// takes no time, and everything else runs on one network thread of the
// server's own. stop() closes every connection and joins that thread.
#include "adhoc/protocol.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mhp3rd::adhoc {

struct ServerConfig {
    std::uint16_t adhocctl_port{kAdhocctlPort};  // the relay listens on relay_port_for() this
    // Written to the network log as it happens; to the console as well when
    // set, as in the standalone server.
    bool print_events{};
};

struct ServerPlayer {
    std::string nickname;
    Mac mac{};
    std::string address;              // where the player connects from
    std::string product;
    std::optional<std::string> group;
    std::uint64_t online_ms{};
};

struct ServerStatus {
    bool running{};
    std::uint16_t adhocctl_port{};
    std::uint16_t relay_port{};
    std::string error;                 // why start() failed
    std::uint64_t uptime_ms{};
    std::vector<ServerPlayer> players; // logged in
    std::size_t connections{};         // adhocctl connections, logged in or not
    std::size_t groups{};
    std::size_t relay_sessions{};      // relay connections of every kind
    std::size_t streams{};             // paired stream connections, counted once per pair
    std::uint64_t relayed_packets{};
    std::uint64_t relayed_bytes{};
    std::uint64_t dropped{};           // datagrams with no receiver or no room
};

class Server {
public:
    Server();
    ~Server();
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    // Listens on every interface. False, with status().error set, when a
    // port cannot be used, for example because another server has it.
    bool start(const ServerConfig &config);
    // Disconnects everyone and stops. The players' clients see a lost server.
    void stop();
    [[nodiscard]] bool running() const;
    // A copy of the server's state, a few times a second fresh.
    [[nodiscard]] ServerStatus status() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mhp3rd::adhoc
