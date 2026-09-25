#include "adhoc/client.hpp"

#include "adhoc/sockets.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>


namespace mhp3rd::adhoc {

std::string format_mac(const Mac &mac) {
    char text[18];
    std::snprintf(text, sizeof(text), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4],
                  mac[5]);
    return text;
}

namespace {

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;

// How long the network thread sleeps in poll(). Short, because it is also how
// long queued data can wait before it is written.
constexpr int kPollIntervalMs = 2;
// adhocctl keeps a client listed for 30 s without traffic.
constexpr auto kPingInterval = milliseconds(2000);
constexpr auto kConnectTimeout = milliseconds(8000);
// Data that could not be written for this long means the connection is dead,
// whatever TCP itself thinks.
constexpr auto kStallTimeout = milliseconds(15000);
// A join or scan that the server does not complete in this time fails.
constexpr auto kRequestTimeout = milliseconds(15000);
// A group the server connection lost is kept, and rejoined on reconnection,
// for this long before the game is told it is disconnected.
constexpr auto kGroupGrace = milliseconds(10000);
// The relay drops a stream connection that is not accepted within 5 s, so an
// older pending connection is not worth accepting.
constexpr auto kPendingConnectionLife = milliseconds(4500);
constexpr std::size_t kMaxQueuedOutput = 512u * 1024u;
constexpr std::size_t kReadChunk = 16u * 1024u;

using net::Address;
using net::close_socket;
using net::connect_pending;
using net::kNoSocket;
using net::kSendFlags;
using net::PollEntry;
using net::poll_sockets;
using net::set_nonblocking;
using net::Socket;
using net::socket_error;
using net::would_block;

// "host", "host:port", "[v6]" or "[v6]:port".
void split_server(const std::string &server, std::string &host, std::uint16_t &port) {
    host = server;
    port = kAdhocctlPort;
    std::string port_text;
    if (!server.empty() && server.front() == '[') {
        const auto close = server.find(']');
        if (close != std::string::npos) {
            host = server.substr(1u, close - 1u);
            if (close + 1u < server.size() && server[close + 1u] == ':') port_text = server.substr(close + 2u);
        }
    } else if (std::count(server.begin(), server.end(), ':') == 1) {
        const auto colon = server.find(':');
        host = server.substr(0, colon);
        port_text = server.substr(colon + 1u);
    }
    if (!port_text.empty()) {
        const unsigned long value = std::strtoul(port_text.c_str(), nullptr, 10);
        if (value > 0u && value < 65536u) port = static_cast<std::uint16_t>(value);
    }
}

std::vector<Address> resolve(const std::string &host, std::uint16_t port) {
    std::vector<Address> result;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo *list = nullptr;
    const std::string service = std::to_string(port);
    if (getaddrinfo(host.c_str(), service.c_str(), &hints, &list) != 0) return result;
    for (addrinfo *entry = list; entry != nullptr; entry = entry->ai_next) {
        if (entry->ai_addrlen > sizeof(sockaddr_storage)) continue;
        Address address;
        std::memcpy(&address.storage, entry->ai_addr, entry->ai_addrlen);
        address.length = static_cast<socklen_t>(entry->ai_addrlen);
        result.push_back(address);
    }
    freeaddrinfo(list);
    // IPv4 first: the relay pairs its connections with the adhocctl login by
    // address, and a v4 path is the one every server has.
    std::stable_sort(result.begin(), result.end(), [](const Address &a, const Address &b) {
        return a.storage.ss_family == AF_INET && b.storage.ss_family != AF_INET;
    });
    return result;
}

void tune_socket(Socket s) {
    int enabled = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&enabled), sizeof(enabled));
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>(&enabled), sizeof(enabled));
#if defined(SO_NOSIGPIPE)
    setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
#endif
#if defined(__APPLE__)
    int idle = 5;
    setsockopt(s, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle));
#elif defined(TCP_KEEPIDLE) && !defined(_WIN32)
    int idle = 5;
    int interval = 2;
    int count = 4;
    setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
#endif
#if defined(TCP_USER_TIMEOUT)
    unsigned int user_timeout_ms = 15000u;
    setsockopt(s, IPPROTO_TCP, TCP_USER_TIMEOUT, &user_timeout_ms, sizeof(user_timeout_ms));
#endif
}

// The last lines logged, for "Save network log".
constexpr std::size_t kLogLines = 6000;

struct LogBuffer {
    std::mutex mutex;
    std::deque<std::string> lines;
};

// Never destroyed: the client's, server's and discovery's threads log until
// their singletons are torn down at exit, in whatever order that happens.
LogBuffer &log_buffer() {
    static LogBuffer *buffer = new LogBuffer;
    return *buffer;
}

std::atomic<bool> &tracing_flag() {
    static std::atomic<bool> flag{[] {
        const char *text = std::getenv("MHP3RD_TRACE_ADHOC");
        return text != nullptr && *text != '\0' && std::strcmp(text, "0") != 0;
    }()};
    return flag;
}

std::string wall_clock() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &seconds);
#else
    localtime_r(&seconds, &tm);
#endif
    char text[32];
    std::snprintf(text, sizeof(text), "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec,
                  static_cast<int>(millis));
    return text;
}

// Network events always go to the log buffer; to the console only while
// tracing, unless they are reports a player should see.
void trace(const std::string &line) { Client::log("[adhoc-net] " + line, false); }
void report(const std::string &line) { Client::log("[adhoc] " + line, true); }

// Retry delays that double from `first` up to `limit`.
struct Backoff {
    milliseconds first{500};
    milliseconds limit{8000};
    milliseconds current{0};

    milliseconds next() {
        current = current.count() == 0 ? first : std::min(current * 2, limit);
        return current;
    }
    void reset() { current = milliseconds(0); }
};

// One TCP connection to the server.
struct Link {
    Socket socket{kNoSocket};
    bool open{};         // the TCP connection is up
    bool failed{};       // closed by the server, refused, or broken
    std::string input;
    std::string output;
    Clock::time_point started{};
    Clock::time_point last_write{};  // last time output shrank or was empty
    bool pause_reading{};

    [[nodiscard]] bool active() const { return socket != kNoSocket; }

    bool start(const Address &address) {
        close();
        failed = false;
        socket = ::socket(address.storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
        if (socket == kNoSocket || !set_nonblocking(socket)) {
            close();
            failed = true;
            return false;
        }
        tune_socket(socket);
        started = Clock::now();
        last_write = started;
        if (::connect(socket, reinterpret_cast<const sockaddr *>(&address.storage), address.length) == 0) {
            open = true;
            return true;
        }
        if (!connect_pending(socket_error())) {
            close();
            failed = true;
            return false;
        }
        return true;
    }

    void close() {
        if (socket != kNoSocket) close_socket(socket);
        socket = kNoSocket;
        open = false;
        input.clear();
        output.clear();
        pause_reading = false;
    }

    void fail() {
        close();
        failed = true;
    }

    // After poll(): completes a pending connect, reads and writes.
    void service(short revents) {
        if (socket == kNoSocket) return;
        const auto now = Clock::now();
        if (!open) {
            if ((revents & (POLLOUT | POLLERR | POLLHUP)) != 0) {
                int error = 0;
                socklen_t length = sizeof(error);
                getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &length);
                if (error != 0) {
                    fail();
                    return;
                }
                open = true;
                last_write = now;
            } else {
                if (now - started > kConnectTimeout) fail();
                return;
            }
        }
        if ((revents & POLLIN) != 0 || (revents & (POLLHUP | POLLERR)) != 0) {
            char buffer[kReadChunk];
            for (int round = 0; round < 8 && !pause_reading; ++round) {
                const auto count = ::recv(socket, buffer, static_cast<int>(sizeof(buffer)), 0);
                if (count > 0) {
                    input.append(buffer, static_cast<std::size_t>(count));
                    if (static_cast<std::size_t>(count) < sizeof(buffer)) break;
                    continue;
                }
                if (count == 0 || !would_block(socket_error())) {
                    // Keep what arrived before the close for the owner to parse.
                    std::string remaining = std::move(input);
                    fail();
                    input = std::move(remaining);
                    return;
                }
                break;
            }
        }
        flush();
    }

    void flush() {
        if (socket == kNoSocket || !open) return;
        const auto now = Clock::now();
        while (!output.empty()) {
            const auto count = ::send(socket, output.data(), static_cast<int>(output.size()), kSendFlags);
            if (count > 0) {
                output.erase(0, static_cast<std::size_t>(count));
                last_write = now;
                continue;
            }
            if (count < 0 && would_block(socket_error())) break;
            fail();
            return;
        }
        if (output.empty()) last_write = now;
        else if (now - last_write > kStallTimeout) fail();
    }
};

struct PendingConnection {
    Mac mac{};
    std::uint16_t port{};
    Clock::time_point arrived{};
};

struct DatagramSocket {
    std::uint16_t port{};
    std::size_t capacity{};
    Link link;
    Backoff backoff{milliseconds(250), milliseconds(4000)};
    Clock::time_point retry_at{};
    std::deque<Datagram> received;
    std::size_t received_bytes{};
    bool closing{};
    // Header of the datagram being read, once complete.
    std::optional<Datagram> partial;
    std::size_t partial_size{};
};

enum class StreamKind { Listen, Connect, Accept };

struct StreamSocket {
    StreamKind kind{};
    StreamState state{StreamState::Closed};
    std::uint16_t local_port{};
    Mac peer{};
    std::uint16_t peer_port{};
    std::size_t capacity{};
    Link link;
    bool notice_seen{};       // the relay's "established" notice
    std::string received;
    std::uint64_t sent_total{};
    std::uint64_t received_total{};
    // Listen
    std::size_t backlog_limit{};
    std::deque<PendingConnection> backlog;
    Backoff backoff{milliseconds(250), milliseconds(4000)};
    Clock::time_point retry_at{};
    // Connect
    std::uint64_t retry_us{};
    std::uint32_t retries_left{};
    // Accept
    bool accept_pending{};
    bool closing{};
};

} // namespace

struct Client::Impl {
    mutable std::mutex mutex;
    std::thread thread;
    std::atomic<bool> quit{false};
    net::WinsockSession winsock;

    // What the game asked for.
    bool active{};
    Identity identity;
    std::uint64_t generation{};  // bumps on start() so a stale resolution is ignored

    // Server connection.
    std::vector<Address> addresses;
    std::size_t address_index{};
    bool resolving{};
    std::optional<Address> server;  // the adhocctl address that last worked
    std::uint16_t relay_port{kRelayPort};
    Link ctl;
    bool logged_in{};
    bool stop_ctl{};  // close `ctl` on the network thread
    Backoff ctl_backoff{milliseconds(500), milliseconds(8000)};
    Clock::time_point ctl_retry_at{};
    Clock::time_point last_ping{};
    std::uint32_t failed_attempts{};

    // Matchmaking.
    std::optional<std::string> wanted_group;
    bool joined{};                  // the server confirmed the join in this login
    bool game_connected{};          // the game was told it is connected
    std::optional<GroupInfo> current_group;
    std::optional<Clock::time_point> join_started;
    std::optional<Clock::time_point> lost_since;
    bool scan_wanted{};
    bool scan_sent{};
    std::optional<Clock::time_point> scan_started;
    std::vector<GroupInfo> scan_building;
    std::vector<GroupInfo> scan_done;
    struct PeerEntry {
        Peer peer;
        std::uint32_t id{};
    };
    std::map<Mac, PeerEntry> peers;
    std::vector<CtlEvent> events;

    // Diagnostics.
    std::string last_error;
    std::uint64_t reconnects{};
    std::optional<Clock::time_point> online_since;
    bool reconnect_requested{};
    Traffic traffic;
    std::uint64_t dropped{};
    std::uint64_t timeouts{};
    std::map<Mac, Clock::time_point> heard;
    mutable std::mutex snapshot_mutex;
    Diagnostics snapshot;
    Clock::time_point snapshot_at{};
    Traffic snapshot_traffic;

    // Sockets.
    int next_handle{1};
    std::map<int, DatagramSocket> datagrams;
    std::map<int, StreamSocket> streams;

    Impl() { thread = std::thread([this] { run(); }); }

    ~Impl() {
        quit = true;
        if (thread.joinable()) thread.join();
    }

    // Everything below runs with `mutex` held unless it says otherwise.

    bool port_taken(std::uint16_t port, bool stream) const {
        if (stream) {
            for (const auto &[handle, socket] : streams)
                if (socket.local_port == port && !socket.closing && socket.kind == StreamKind::Listen) return true;
            return false;
        }
        for (const auto &[handle, socket] : datagrams)
            if (socket.port == port && !socket.closing) return true;
        return false;
    }

    std::uint16_t ephemeral_port(bool stream) const {
        for (std::uint32_t port = kFirstEphemeralPort; port < 0xFFFFu; ++port) {
            bool used = port_taken(static_cast<std::uint16_t>(port), stream);
            if (stream) {
                for (const auto &[handle, socket] : streams)
                    if (socket.local_port == port && !socket.closing) used = true;
            }
            if (!used) return static_cast<std::uint16_t>(port);
        }
        return 0u;
    }

    void send_ctl(const std::string &packet, const std::string &what) {
        if (!ctl.active()) return;
        ctl.output += packet;
        trace("ctl > " + what);
    }

    void clear_group_state() {
        joined = false;
        current_group.reset();
        peers.clear();
    }

    // Relay links only exist while the server has us in a group.
    void close_relay_links(bool streams_too) {
        for (auto &[handle, socket] : datagrams) {
            socket.link.close();
            socket.partial.reset();
        }
        if (!streams_too) return;
        for (auto &[handle, socket] : streams) {
            if (socket.kind == StreamKind::Listen) {
                socket.link.close();
                socket.backlog.clear();
                continue;
            }
            if (socket.link.active()) socket.link.close();
            if (socket.state == StreamState::Established) {
                socket.state = StreamState::Disconnected;
                trace("stream " + std::to_string(handle) + " lost with the group");
            } else if (socket.state == StreamState::Opening) {
                socket.state = StreamState::Failed;
            }
        }
    }

    void on_ctl_lost(const char *why) {
        last_error = why;
        online_since.reset();
        if (logged_in) ++reconnects;
        if (logged_in || ctl.active()) report(std::string("lost the server connection: ") + why);
        ctl.close();
        logged_in = false;
        scan_sent = false;
        const auto delay = ctl_backoff.next();
        ctl_retry_at = Clock::now() + delay;
        trace("reconnecting in " + std::to_string(delay.count()) + " ms");
        if (joined || game_connected) {
            if (!lost_since) lost_since = Clock::now();
        }
        joined = false;
        close_relay_links(true);
    }

    void lose_group_for_good() {
        report("the group is lost; reporting a disconnect to the game");
        wanted_group.reset();
        join_started.reset();
        lost_since.reset();
        if (game_connected) events.push_back(CtlEvent::Disconnected);
        game_connected = false;
        clear_group_state();
        close_relay_links(true);
    }

    void handle_ctl_input() {
        std::string &in = ctl.input;
        std::size_t offset = 0;
        while (offset < in.size()) {
            const auto opcode = static_cast<std::uint8_t>(in[offset]);
            const std::size_t size = ctl::server_packet_size(opcode);
            if (size == 0u) {
                report("the server sent an unknown adhocctl opcode " + std::to_string(opcode) + "; reconnecting");
                in.clear();
                on_ctl_lost("protocol error");
                return;
            }
            if (in.size() - offset < size) break;
            const char *packet = in.data() + offset;
            handle_ctl_packet(opcode, packet);
            offset += size;
        }
        in.erase(0, offset);
    }

    void handle_ctl_packet(std::uint8_t opcode, const char *packet) {
        switch (opcode) {
        case ctl::kPing:
            break;
        case ctl::kConnect: {
            PeerEntry entry;
            entry.peer.nickname = wire::get_fixed(packet + 1, ctl::kNicknameLength);
            entry.peer.mac = wire::get_mac(packet + 1 + ctl::kNicknameLength);
            entry.id = wire::get32(packet + 1 + ctl::kNicknameLength + 6);
            entry.peer.joined_ms = static_cast<std::uint64_t>(
                std::chrono::duration_cast<milliseconds>(Clock::now().time_since_epoch()).count());
            trace("ctl < connect peer " + format_mac(entry.peer.mac) + " \"" + entry.peer.nickname + "\" id " +
                  std::to_string(entry.id));
            if (entry.peer.mac != identity.mac) {
                if (!peers.contains(entry.peer.mac)) report("peer joined: " + entry.peer.nickname);
                peers[entry.peer.mac] = entry;
            }
            break;
        }
        case ctl::kDisconnect: {
            const std::uint32_t id = wire::get32(packet + 1);
            trace("ctl < disconnect peer id " + std::to_string(id));
            for (auto it = peers.begin(); it != peers.end(); ++it) {
                if (it->second.id != id) continue;
                report("peer left: " + it->second.peer.nickname);
                peers.erase(it);
                break;
            }
            break;
        }
        case ctl::kScan: {
            GroupInfo info;
            info.name = wire::get_fixed(packet + 1, ctl::kGroupNameLength);
            info.host = wire::get_mac(packet + 1 + ctl::kGroupNameLength);
            trace("ctl < scan result \"" + info.name + "\" host " + format_mac(info.host));
            scan_building.push_back(info);
            break;
        }
        case ctl::kScanComplete:
            trace("ctl < scan complete, " + std::to_string(scan_building.size()) + " groups");
            scan_done = std::move(scan_building);
            scan_building.clear();
            if (scan_wanted) events.push_back(CtlEvent::ScanComplete);
            scan_wanted = false;
            scan_sent = false;
            scan_started.reset();
            break;
        case ctl::kConnectBssid: {
            const Mac host = wire::get_mac(packet + 1);
            trace("ctl < connect bssid " + format_mac(host));
            if (!wanted_group) break;
            joined = true;
            current_group = GroupInfo{*wanted_group, host};
            join_started.reset();
            if (lost_since) report("rejoined group " + *wanted_group + " after reconnecting");
            lost_since.reset();
            if (!game_connected) {
                report("joined group " + *wanted_group);
                game_connected = true;
                events.push_back(CtlEvent::Connected);
            }
            break;
        }
        case ctl::kChat:
            trace("ctl < chat (ignored)");
            break;
        default:
            break;
        }
    }

    void on_logged_in() {
        logged_in = true;
        online_since = Clock::now();
        ctl_backoff.reset();
        failed_attempts = 0;
        last_ping = Clock::now();
        report("logged in to " + (server ? server->describe() : std::string("?")) + " as \"" + identity.nickname +
               "\" " + format_mac(identity.mac));
        if (wanted_group) {
            clear_group_state();
            send_ctl(ctl::connect(*wanted_group), "connect \"" + *wanted_group + "\"");
        }
        if (scan_wanted) {
            scan_building.clear();
            send_ctl(ctl::opcode_only(ctl::kScan), "scan");
            scan_sent = true;
        }
    }

    // Matchmaking upkeep, once per loop.
    void maintain_ctl(Clock::time_point now) {
        if (stop_ctl) {
            stop_ctl = false;
            ctl.close();
        }
        if (!active || identity.server.empty()) return;
        if (reconnect_requested) {
            reconnect_requested = false;
            if (ctl.active()) {
                report("reconnecting now, as asked");
                on_ctl_lost("reconnect requested");
            }
            ctl_backoff.reset();
            ctl_retry_at = now;
            addresses.clear();  // resolve again: the address may have changed
        }
        if (join_started && !game_connected && now - *join_started > kRequestTimeout) {
            report("could not join group " + wanted_group.value_or("?") + ": the server did not answer");
            events.push_back(CtlEvent::Error);
            if (logged_in && wanted_group) send_ctl(ctl::opcode_only(ctl::kDisconnect), "disconnect");
            wanted_group.reset();
            join_started.reset();
            clear_group_state();
        }
        if (scan_wanted && scan_started && now - *scan_started > kRequestTimeout) {
            report("scan failed: the server did not answer");
            events.push_back(CtlEvent::Error);
            scan_wanted = false;
            scan_sent = false;
            scan_started.reset();
        }
        if (lost_since && now - *lost_since > kGroupGrace) lose_group_for_good();

        if (ctl.failed) {
            ctl.failed = false;
            if (!ctl.input.empty()) handle_ctl_input();
            ++failed_attempts;
            address_index = addresses.empty() ? 0u : (address_index + 1u) % addresses.size();
            on_ctl_lost(logged_in ? "closed" : "could not connect");
            if (failed_attempts == 1u || failed_attempts % 10u == 0u)
                report("cannot reach the ad hoc server " + identity.server + " (attempt " +
                       std::to_string(failed_attempts) + ")");
            return;
        }
        if (!ctl.active()) {
            if (resolving || now < ctl_retry_at) return;
            if (addresses.empty()) return;  // run() resolves outside the lock
            const Address address = addresses[address_index % addresses.size()];
            trace("connecting to " + address.describe());
            if (!ctl.start(address)) return;
            server = address;
            return;
        }
        if (ctl.open && !logged_in) {
            send_ctl(ctl::login(identity.mac, identity.nickname, identity.product),
                     "login " + format_mac(identity.mac) + " \"" + identity.nickname + "\" " + identity.product);
            on_logged_in();
        }
        if (logged_in && now - last_ping >= kPingInterval) {
            last_ping = now;
            ctl.output += ctl::opcode_only(ctl::kPing);
        }
        if (logged_in && scan_wanted && !scan_sent) {
            scan_building.clear();
            send_ctl(ctl::opcode_only(ctl::kScan), "scan");
            scan_sent = true;
        }
    }

    Address relay_address() const { return server->with_port(relay_port); }

    void start_relay(Link &link, const std::string &init, const std::string &what) {
        if (!link.start(relay_address())) return;
        link.output = init;
        trace("relay > init " + what);
    }

    void maintain_datagrams(Clock::time_point now) {
        for (auto it = datagrams.begin(); it != datagrams.end();) {
            DatagramSocket &socket = it->second;
            if (socket.closing) {
                socket.link.close();
                it = datagrams.erase(it);
                continue;
            }
            if (socket.link.failed) {
                socket.link.failed = false;
                parse_datagrams(it->first, socket);
                socket.link.close();
                socket.partial.reset();
                const auto delay = socket.backoff.next();
                socket.retry_at = now + delay;
                trace("pdp " + std::to_string(it->first) + " relay link closed; retry in " +
                      std::to_string(delay.count()) + " ms");
            }
            if (!socket.link.active() && joined && server && now >= socket.retry_at) {
                start_relay(socket.link,
                            relay::init(relay::kInitPdp, identity.mac, socket.port, Mac{}, 0u),
                            "pdp " + format_mac(identity.mac) + " port " + std::to_string(socket.port));
            }
            if (socket.link.open && socket.link.input.empty() && now - socket.link.started > milliseconds(3000))
                socket.backoff.reset();
            ++it;
        }
    }

    void parse_datagrams(int handle, DatagramSocket &socket) {
        std::string &in = socket.link.input;
        std::size_t offset = 0;
        for (;;) {
            if (!socket.partial) {
                if (in.size() - offset < relay::kPdpHeaderSize) break;
                Datagram datagram;
                datagram.source = wire::get_mac(in.data() + offset);
                datagram.port = wire::get16(in.data() + offset + 8);
                socket.partial_size = wire::get32(in.data() + offset + 10);
                socket.partial = std::move(datagram);
                offset += relay::kPdpHeaderSize;
                if (socket.partial_size > relay::kPdpBlockMax * 2u) {
                    report("the relay sent an oversized datagram; reconnecting the socket");
                    socket.partial.reset();
                    in.clear();
                    socket.link.fail();
                    return;
                }
            }
            if (in.size() - offset < socket.partial_size) break;
            Datagram datagram = std::move(*socket.partial);
            socket.partial.reset();
            datagram.data.assign(in.data() + offset, socket.partial_size);
            offset += socket.partial_size;
            trace("relay < pdp " + std::to_string(handle) + " from " + format_mac(datagram.source) + " port " +
                  std::to_string(datagram.port) + " size " + std::to_string(datagram.data.size()));
            ++traffic.packets_in;
            traffic.bytes_in += datagram.data.size();
            heard[datagram.source] = Clock::now();
            if (socket.received_bytes + datagram.data.size() > socket.capacity) {
                trace("pdp " + std::to_string(handle) + " buffer full; datagram dropped");
                ++dropped;
                continue;
            }
            socket.received_bytes += datagram.data.size();
            socket.received.push_back(std::move(datagram));
        }
        in.erase(0, offset);
    }

    void maintain_streams(Clock::time_point now) {
        for (auto it = streams.begin(); it != streams.end();) {
            const int handle = it->first;
            StreamSocket &socket = it->second;
            if (socket.closing) {
                socket.link.close();
                it = streams.erase(it);
                continue;
            }
            switch (socket.kind) {
            case StreamKind::Listen:
                if (socket.link.failed) {
                    socket.link.failed = false;
                    parse_stream(handle, socket);
                    socket.link.close();
                    const auto delay = socket.backoff.next();
                    socket.retry_at = now + delay;
                    trace("ptp " + std::to_string(handle) + " listen link closed; retry in " +
                          std::to_string(delay.count()) + " ms");
                }
                if (!socket.link.active() && joined && server && now >= socket.retry_at) {
                    start_relay(socket.link,
                                relay::init(relay::kInitPtpListen, identity.mac, socket.local_port, Mac{}, 0u),
                                "ptp listen port " + std::to_string(socket.local_port));
                }
                while (!socket.backlog.empty() && now - socket.backlog.front().arrived > kPendingConnectionLife)
                    socket.backlog.pop_front();
                break;
            case StreamKind::Connect:
                if (socket.state == StreamState::Opening) {
                    if (socket.link.failed) {
                        socket.link.failed = false;
                        parse_stream(handle, socket);
                        socket.link.close();
                        if (socket.state == StreamState::Opening) {
                            if (socket.retries_left == 0u) {
                                trace("ptp " + std::to_string(handle) + " refused");
                                socket.state = StreamState::Failed;
                            } else {
                                --socket.retries_left;
                                socket.retry_at = now + std::chrono::microseconds(
                                                            std::max<std::uint64_t>(socket.retry_us, 100'000u));
                                trace("ptp " + std::to_string(handle) + " not accepted; retrying");
                            }
                        }
                    }
                    if (socket.state == StreamState::Opening && !socket.link.active() && joined && server &&
                        now >= socket.retry_at) {
                        socket.notice_seen = false;
                        start_relay(socket.link,
                                    relay::init(relay::kInitPtpConnect, identity.mac, socket.local_port, socket.peer,
                                                socket.peer_port),
                                    "ptp connect " + std::to_string(socket.local_port) + " -> " +
                                        format_mac(socket.peer) + " port " + std::to_string(socket.peer_port));
                    }
                    break;
                }
                [[fallthrough]];
            case StreamKind::Accept:
                if (socket.accept_pending) {
                    socket.accept_pending = false;
                    if (server)
                        start_relay(socket.link,
                                    relay::init(relay::kInitPtpAccept, identity.mac, socket.local_port, socket.peer,
                                                socket.peer_port),
                                    "ptp accept " + std::to_string(socket.local_port) + " <- " +
                                        format_mac(socket.peer) + " port " + std::to_string(socket.peer_port));
                    if (!socket.link.active()) socket.state = StreamState::Disconnected;
                }
                if (socket.link.failed) {
                    socket.link.failed = false;
                    parse_stream(handle, socket);
                    socket.link.close();
                    if (socket.state == StreamState::Established) {
                        report("stream to " + format_mac(socket.peer) + " closed");
                        socket.state = StreamState::Disconnected;
                    }
                }
                break;
            }
            if (socket.link.active())
                socket.link.pause_reading = socket.kind != StreamKind::Listen && socket.received.size() >= socket.capacity;
            ++it;
        }
    }

    void parse_stream(int handle, StreamSocket &socket) {
        std::string &in = socket.link.input;
        std::size_t offset = 0;
        if (socket.kind == StreamKind::Listen) {
            while (in.size() - offset >= relay::kPtpNoticeSize) {
                PendingConnection pending;
                pending.mac = wire::get_mac(in.data() + offset);
                pending.port = wire::get16(in.data() + offset + 8);
                pending.arrived = Clock::now();
                offset += relay::kPtpNoticeSize;
                trace("relay < ptp " + std::to_string(handle) + " connection request from " +
                      format_mac(pending.mac) + " port " + std::to_string(pending.port));
                if (socket.backlog.size() < std::max<std::size_t>(socket.backlog_limit, 1u))
                    socket.backlog.push_back(pending);
            }
            in.erase(0, offset);
            return;
        }
        if (!socket.notice_seen) {
            if (in.size() < relay::kPtpNoticeSize) return;
            socket.notice_seen = true;
            offset = relay::kPtpNoticeSize;
            trace("relay < ptp " + std::to_string(handle) + " established with " + format_mac(wire::get_mac(in.data())) +
                  " port " + std::to_string(wire::get16(in.data() + 8)));
            if (socket.state == StreamState::Opening) {
                socket.state = StreamState::Established;
                report("stream to " + format_mac(socket.peer) + " established");
            }
        }
        while (in.size() - offset >= relay::kPtpHeaderSize) {
            const std::uint32_t size = wire::get32(in.data() + offset);
            if (size > relay::kPtpBlockMax * 2u) {
                report("the relay sent an oversized stream block; closing the stream");
                in.clear();
                socket.link.fail();
                return;
            }
            if (in.size() - offset - relay::kPtpHeaderSize < size) break;
            socket.received.append(in.data() + offset + relay::kPtpHeaderSize, size);
            socket.received_total += size;
            ++traffic.packets_in;
            traffic.bytes_in += size;
            heard[socket.peer] = Clock::now();
            offset += relay::kPtpHeaderSize + size;
            trace("relay < ptp " + std::to_string(handle) + " data " + std::to_string(size));
        }
        in.erase(0, offset);
    }

    std::optional<double> ctl_rtt_ms() const {
        if (!ctl.open) return std::nullopt;
#if defined(__APPLE__) && defined(TCP_CONNECTION_INFO)
        tcp_connection_info info{};
        socklen_t length = sizeof(info);
        if (getsockopt(ctl.socket, IPPROTO_TCP, TCP_CONNECTION_INFO, &info, &length) == 0 && info.tcpi_srtt != 0u)
            return static_cast<double>(info.tcpi_srtt);
#elif defined(__linux__) && defined(TCP_INFO)
        tcp_info info{};
        socklen_t length = sizeof(info);
        if (getsockopt(ctl.socket, IPPROTO_TCP, TCP_INFO, &info, &length) == 0 && info.tcpi_rtt != 0u)
            return static_cast<double>(info.tcpi_rtt) / 1000.0;
#endif
        return std::nullopt;
    }

    // Copies the state the network panel shows. Runs on the network thread
    // with `mutex` held.
    void publish() {
        const auto now = Clock::now();
        const auto ms = [&](Clock::time_point since) {
            return static_cast<std::uint64_t>(std::chrono::duration_cast<milliseconds>(now - since).count());
        };
        Diagnostics d;
        d.active = active;
        d.server = identity.server;
        d.server_address = server && ctl.open ? server->describe() : std::string{};
        d.nickname = identity.nickname;
        d.mac = identity.mac;
        d.product = identity.product;
        d.state = !active || identity.server.empty() ? ServerState::Off
                  : logged_in                        ? ServerState::Online
                                                     : ServerState::Connecting;
        d.failed_attempts = failed_attempts;
        d.reconnects = reconnects;
        d.last_error = last_error;
        if (online_since) d.online_ms = ms(*online_since);
        d.rtt_ms = ctl_rtt_ms();
        if (game_connected && wanted_group) d.group = wanted_group;
        else if (wanted_group) d.joining = wanted_group;
        if (lost_since) d.rejoin_ms = ms(*lost_since);
        for (const auto &[mac, entry] : peers) {
            PeerSummary peer;
            peer.mac = mac;
            peer.nickname = entry.peer.nickname;
            const auto joined = Clock::time_point(milliseconds(entry.peer.joined_ms));
            peer.in_group_ms = ms(joined);
            if (const auto found = heard.find(mac); found != heard.end()) peer.last_heard_ms = ms(found->second);
            d.peers.push_back(peer);
        }
        for (const auto &[handle, socket] : datagrams) {
            if (socket.closing) continue;
            SocketSummary summary;
            summary.kind = "PDP";
            summary.handle = handle;
            summary.port = socket.port;
            summary.relay_linked = socket.link.open;
            summary.state = socket.link.open ? "linked" : (joined ? "linking" : "waiting for a group");
            d.sockets.push_back(summary);
            ++d.relay_links_wanted;
            if (socket.link.open) ++d.relay_links_up;
        }
        for (const auto &[handle, socket] : streams) {
            if (socket.closing) continue;
            SocketSummary summary;
            summary.kind = socket.kind == StreamKind::Listen    ? "PTP listen"
                           : socket.kind == StreamKind::Connect ? "PTP open"
                                                                : "PTP accepted";
            summary.handle = handle;
            summary.port = socket.local_port;
            summary.relay_linked = socket.link.open;
            if (socket.kind != StreamKind::Listen) {
                summary.peer = socket.peer;
                summary.peer_port = socket.peer_port;
            }
            switch (socket.state) {
            case StreamState::Closed: summary.state = "closed"; break;
            case StreamState::Listening:
                summary.state = std::string(socket.link.open ? "listening" : "linking") +
                                (socket.backlog.empty() ? "" : ", " + std::to_string(socket.backlog.size()) +
                                                                   " waiting");
                break;
            case StreamState::Opening: summary.state = "connecting"; break;
            case StreamState::Established: summary.state = "established"; break;
            case StreamState::Failed: summary.state = "refused"; break;
            case StreamState::Disconnected: summary.state = "disconnected"; break;
            }
            d.sockets.push_back(summary);
            if (socket.state == StreamState::Listening || socket.state == StreamState::Established ||
                socket.state == StreamState::Opening) {
                ++d.relay_links_wanted;
                if (socket.link.open) ++d.relay_links_up;
            }
        }
        d.total = traffic;
        const double seconds = std::chrono::duration<double>(now - snapshot_at).count();
        if (seconds > 0.0 && snapshot_at != Clock::time_point{}) {
            const auto rate = [&](std::uint64_t current, std::uint64_t previous) {
                return static_cast<std::uint64_t>(static_cast<double>(current - previous) / seconds + 0.5);
            };
            d.per_second.packets_in = rate(traffic.packets_in, snapshot_traffic.packets_in);
            d.per_second.packets_out = rate(traffic.packets_out, snapshot_traffic.packets_out);
            d.per_second.bytes_in = rate(traffic.bytes_in, snapshot_traffic.bytes_in);
            d.per_second.bytes_out = rate(traffic.bytes_out, snapshot_traffic.bytes_out);
        }
        d.dropped = dropped;
        d.timeouts = timeouts;
        snapshot_at = now;
        snapshot_traffic = traffic;
        std::lock_guard lock(snapshot_mutex);
        snapshot = std::move(d);
    }

    // The network thread. Holds the lock except in poll() and name lookups.
    void run() {
        std::vector<PollEntry> entries;
        std::vector<Link *> links;
        while (!quit) {
            std::string to_resolve;
            std::uint64_t resolve_generation = 0;
            {
                std::unique_lock lock(mutex);
                const auto now = Clock::now();
                if (active && !identity.server.empty() && addresses.empty() && !resolving && now >= ctl_retry_at) {
                    resolving = true;
                    to_resolve = identity.server;
                    resolve_generation = generation;
                }
                maintain_ctl(now);
                maintain_datagrams(now);
                maintain_streams(now);
                entries.clear();
                links.clear();
                const auto add = [&](Link &link) {
                    if (!link.active()) return;
                    link.flush();
                    if (!link.active()) return;
                    PollEntry entry{};
                    entry.fd = link.socket;
                    entry.events = static_cast<short>((link.pause_reading ? 0 : POLLIN) |
                                                      (!link.open || !link.output.empty() ? POLLOUT : 0));
                    entries.push_back(entry);
                    links.push_back(&link);
                };
                add(ctl);
                for (auto &[handle, socket] : datagrams) add(socket.link);
                for (auto &[handle, socket] : streams) add(socket.link);
            }
            if (!to_resolve.empty()) {
                std::string host;
                std::uint16_t port = kAdhocctlPort;
                split_server(to_resolve, host, port);
                {
                    std::lock_guard lock(mutex);
                    relay_port = relay_port_for(port);
                }
                trace("resolving " + host);
                std::vector<Address> found = resolve(host, port);
                std::lock_guard lock(mutex);
                resolving = false;
                if (resolve_generation == generation) {
                    if (found.empty()) {
                        ++failed_attempts;
                        if (failed_attempts == 1u || failed_attempts % 10u == 0u)
                            report("cannot resolve the ad hoc server " + host);
                        ctl_retry_at = Clock::now() + ctl_backoff.next();
                    } else {
                        addresses = std::move(found);
                        address_index = 0;
                    }
                }
                continue;
            }
            const int ready = poll_sockets(entries.data(), entries.size(), kPollIntervalMs);
            std::lock_guard lock(mutex);
            for (std::size_t i = 0; i < entries.size(); ++i) {
                // A socket the game closed meanwhile is gone from the maps only
                // when this thread erases it, so the pointer is still valid.
                Link &link = *links[i];
                if (link.socket != entries[i].fd) continue;
                link.service(ready > 0 ? entries[i].revents : static_cast<short>(0));
            }
            if (!ctl.input.empty()) handle_ctl_input();
            for (auto &[handle, socket] : datagrams)
                if (!socket.link.input.empty()) parse_datagrams(handle, socket);
            for (auto &[handle, socket] : streams)
                if (!socket.link.input.empty()) parse_stream(handle, socket);
            if (Clock::now() - snapshot_at >= milliseconds(250)) publish();
        }
        std::lock_guard lock(mutex);
        ctl.close();
        for (auto &[handle, socket] : datagrams) socket.link.close();
        for (auto &[handle, socket] : streams) socket.link.close();
    }
};

// Never destroyed: its thread is stopped by shutdown() before the process
// exits, not by static destruction in an order nobody controls.
Client &Client::get() {
    static Client *client = new Client;
    return *client;
}

void Client::shutdown() noexcept {
    try {
        stop();
        impl_->quit = true;
        if (impl_->thread.joinable()) impl_->thread.join();
    } catch (...) {
    }
}

Client::Client() : impl_(std::make_unique<Impl>()) {}
Client::~Client() = default;

bool Client::tracing() { return tracing_flag().load(std::memory_order_relaxed); }

void Client::set_tracing(bool enabled) {
    tracing_flag() = enabled;
    log(std::string("[adhoc] tracing ") + (enabled ? "on" : "off"), true);
}

void Client::log(const std::string &line, bool print) {
    LogBuffer &buffer = log_buffer();
    std::lock_guard lock(buffer.mutex);
    if (print || tracing()) std::cerr << line << std::endl;
    buffer.lines.push_back(wall_clock() + " " + line);
    if (buffer.lines.size() > kLogLines) buffer.lines.pop_front();
}

Diagnostics Client::diagnostics() const {
    std::lock_guard lock(impl_->snapshot_mutex);
    return impl_->snapshot;
}

void Client::reconnect_now() {
    std::lock_guard lock(impl_->mutex);
    impl_->reconnect_requested = true;
}

void Client::disconnect_now() {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (!s.game_connected && !s.wanted_group) return;
    report("disconnecting from the group, as asked");
    if (s.logged_in) s.send_ctl(ctl::opcode_only(ctl::kDisconnect), "disconnect");
    s.lose_group_for_good();
}

void Client::note_timeout() {
    std::lock_guard lock(impl_->mutex);
    ++impl_->timeouts;
}

std::filesystem::path Client::save_log(const std::filesystem::path &dir) const {
    const Diagnostics d = diagnostics();
    std::vector<std::string> lines;
    {
        LogBuffer &buffer = log_buffer();
        std::lock_guard lock(buffer.mutex);
        lines.assign(buffer.lines.begin(), buffer.lines.end());
    }
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char name[64];
    std::strftime(name, sizeof(name), "adhoc-%Y%m%d-%H%M%S.log", &tm);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    std::filesystem::path path = dir / name;
    for (int suffix = 2; std::filesystem::exists(path, ec) && suffix < 100; ++suffix)
        path = dir / (std::string(name).substr(0, std::strlen(name) - 4u) + "-" + std::to_string(suffix) + ".log");
    std::ofstream out(path, std::ios::binary);
    if (!out) return {};
    const auto state = [](ServerState value) {
        switch (value) {
        case ServerState::Off: return "off";
        case ServerState::Connecting: return "connecting";
        case ServerState::Online: return "online";
        }
        return "?";
    };
    out << "Yakumo ad hoc log\n\n";
    out << "server: " << (d.server.empty() ? "(none)" : d.server) << " -> "
        << (d.server_address.empty() ? "-" : d.server_address) << "\n";
    out << "state: " << state(d.state) << ", failed attempts " << d.failed_attempts << ", reconnects " << d.reconnects
        << ", last error: " << (d.last_error.empty() ? "-" : d.last_error) << "\n";
    out << "me: " << format_mac(d.mac) << " \"" << d.nickname << "\" " << d.product << "\n";
    if (d.rtt_ms) out << "round trip: " << *d.rtt_ms << " ms\n";
    out << "group: " << d.group.value_or(d.joining ? *d.joining + " (joining)" : "-") << "\n";
    for (const PeerSummary &peer : d.peers)
        out << "peer: " << format_mac(peer.mac) << " \"" << peer.nickname << "\" in group " << peer.in_group_ms
            << " ms, heard " << (peer.last_heard_ms ? std::to_string(*peer.last_heard_ms) + " ms ago" : "never")
            << "\n";
    for (const SocketSummary &socket : d.sockets)
        out << "socket " << socket.handle << ": " << socket.kind << " port " << socket.port << " " << socket.state
            << (socket.peer ? " peer " + format_mac(*socket.peer) + " port " + std::to_string(socket.peer_port) : "")
            << "\n";
    out << "traffic: in " << d.total.packets_in << " packets / " << d.total.bytes_in << " bytes, out "
        << d.total.packets_out << " packets / " << d.total.bytes_out << " bytes; dropped " << d.dropped
        << ", timeouts " << d.timeouts << "\n\n";
    for (const std::string &line : lines) out << line << "\n";
    if (!out) return {};
    return path;
}

void Client::start(const Identity &identity) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    const bool same = s.active && s.identity.server == identity.server && s.identity.mac == identity.mac &&
                      s.identity.nickname == identity.nickname && s.identity.product == identity.product;
    if (same) return;
    s.stop_ctl = true;
    s.logged_in = false;
    s.identity = identity;
    s.active = true;
    ++s.generation;
    s.addresses.clear();
    s.server.reset();
    s.ctl_backoff.reset();
    s.ctl_retry_at = Clock::now();
    s.failed_attempts = 0;
    // A group left this way is a normal disconnect to the game.
    if (s.game_connected) s.events.push_back(CtlEvent::Disconnected);
    s.game_connected = false;
    s.wanted_group.reset();
    s.join_started.reset();
    s.lost_since.reset();
    s.clear_group_state();
    s.close_relay_links(true);
    if (identity.server.empty()) report("no ad hoc server is configured; ad hoc play is offline");
    else report("using ad hoc server " + identity.server);
}

void Client::stop() {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (!s.active) return;
    // Closing the connection logs out; the server drops us from the group.
    s.stop_ctl = true;
    s.logged_in = false;
    s.active = false;
    s.wanted_group.reset();
    s.join_started.reset();
    s.lost_since.reset();
    s.game_connected = false;
    s.scan_wanted = false;
    s.clear_group_state();
    for (auto &[handle, socket] : s.datagrams) socket.closing = true;
    for (auto &[handle, socket] : s.streams) socket.closing = true;
    s.events.clear();
    trace("stopped");
}

ServerState Client::server_state() const {
    std::lock_guard lock(impl_->mutex);
    if (!impl_->active || impl_->identity.server.empty()) return ServerState::Off;
    return impl_->logged_in ? ServerState::Online : ServerState::Connecting;
}

void Client::join(const std::string &group) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (!s.active || s.identity.server.empty()) {
        s.events.push_back(CtlEvent::Error);
        return;
    }
    if (s.game_connected && s.wanted_group == group) {
        s.events.push_back(CtlEvent::Connected);
        return;
    }
    if (s.game_connected && s.logged_in) s.send_ctl(ctl::opcode_only(ctl::kDisconnect), "disconnect");
    s.game_connected = false;
    s.clear_group_state();
    s.close_relay_links(true);
    s.wanted_group = group;
    s.join_started = Clock::now();
    s.lost_since.reset();
    if (s.logged_in) s.send_ctl(ctl::connect(group), "connect \"" + group + "\"");
}

void Client::leave() {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (s.logged_in && s.wanted_group) s.send_ctl(ctl::opcode_only(ctl::kDisconnect), "disconnect");
    if (s.wanted_group) report("left group " + *s.wanted_group);
    s.wanted_group.reset();
    s.join_started.reset();
    s.lost_since.reset();
    s.game_connected = false;
    s.clear_group_state();
    s.close_relay_links(true);
}

void Client::scan() {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (!s.active || s.identity.server.empty()) {
        s.events.push_back(CtlEvent::Error);
        return;
    }
    s.scan_wanted = true;
    s.scan_started = Clock::now();
    s.scan_building.clear();
    if (s.logged_in) {
        s.send_ctl(ctl::opcode_only(ctl::kScan), "scan");
        s.scan_sent = true;
    } else {
        s.scan_sent = false;
    }
}

bool Client::in_group() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->game_connected;
}

std::optional<GroupInfo> Client::group() const {
    std::lock_guard lock(impl_->mutex);
    if (!impl_->game_connected) return std::nullopt;
    if (impl_->current_group) return impl_->current_group;
    if (impl_->wanted_group) return GroupInfo{*impl_->wanted_group, Mac{}};
    return std::nullopt;
}

std::vector<Peer> Client::peers() const {
    std::lock_guard lock(impl_->mutex);
    std::vector<Peer> result;
    if (!impl_->game_connected) return result;
    for (const auto &[mac, entry] : impl_->peers) result.push_back(entry.peer);
    return result;
}

std::vector<GroupInfo> Client::scan_results() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->scan_done;
}

std::vector<CtlEvent> Client::take_events() {
    std::lock_guard lock(impl_->mutex);
    std::vector<CtlEvent> events;
    events.swap(impl_->events);
    return events;
}

int Client::pdp_open(std::uint16_t port, std::size_t capacity) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (port == 0u) port = s.ephemeral_port(false);
    if (port == 0u || s.port_taken(port, false)) return 0;
    const int handle = s.next_handle++;
    DatagramSocket &socket = s.datagrams[handle];
    socket.port = port;
    socket.capacity = std::max<std::size_t>(capacity, 1u);
    trace("pdp " + std::to_string(handle) + " open port " + std::to_string(port));
    return handle;
}

void Client::pdp_close(int handle) {
    std::lock_guard lock(impl_->mutex);
    if (auto found = impl_->datagrams.find(handle); found != impl_->datagrams.end()) {
        found->second.closing = true;
        trace("pdp " + std::to_string(handle) + " close");
    }
}

bool Client::pdp_send(int handle, const Mac &destination, std::uint16_t port, const void *data, std::size_t size) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    auto found = s.datagrams.find(handle);
    if (found == s.datagrams.end() || found->second.closing) return false;
    DatagramSocket &socket = found->second;
    if (!socket.link.active() || !s.joined) {
        trace("pdp " + std::to_string(handle) + " not linked; datagram to " + format_mac(destination) + " dropped");
        ++s.dropped;
        return true;
    }
    if (size > relay::kPdpBlockMax) {
        ++s.dropped;
        return true;
    }
    const auto queue = [&](const Mac &target) {
        if (socket.link.output.size() > kMaxQueuedOutput) {
            trace("pdp " + std::to_string(handle) + " send queue full; datagram dropped");
            ++s.dropped;
            return;
        }
        ++s.traffic.packets_out;
        s.traffic.bytes_out += size;
        socket.link.output += relay::pdp_header(target, port, static_cast<std::uint32_t>(size));
        socket.link.output.append(static_cast<const char *>(data), size);
        trace("relay > pdp " + std::to_string(handle) + " to " + format_mac(target) + " port " + std::to_string(port) +
              " size " + std::to_string(size));
    };
    if (destination == kBroadcastMac) {
        for (const auto &[mac, entry] : s.peers) queue(mac);
    } else {
        queue(destination);
    }
    return true;
}

std::optional<std::size_t> Client::pdp_peek(int handle) const {
    std::lock_guard lock(impl_->mutex);
    const auto found = impl_->datagrams.find(handle);
    if (found == impl_->datagrams.end() || found->second.received.empty()) return std::nullopt;
    return found->second.received.front().data.size();
}

std::optional<Datagram> Client::pdp_receive(int handle) {
    std::lock_guard lock(impl_->mutex);
    const auto found = impl_->datagrams.find(handle);
    if (found == impl_->datagrams.end() || found->second.received.empty()) return std::nullopt;
    DatagramSocket &socket = found->second;
    Datagram datagram = std::move(socket.received.front());
    socket.received.pop_front();
    socket.received_bytes -= datagram.data.size();
    return datagram;
}

std::size_t Client::pdp_waiting_bytes(int handle) const {
    std::lock_guard lock(impl_->mutex);
    const auto found = impl_->datagrams.find(handle);
    return found == impl_->datagrams.end() ? 0u : found->second.received_bytes;
}

int Client::ptp_listen(std::uint16_t port, std::size_t capacity, std::size_t backlog) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (port == 0u) port = s.ephemeral_port(true);
    if (port == 0u || s.port_taken(port, true)) return 0;
    const int handle = s.next_handle++;
    StreamSocket &socket = s.streams[handle];
    socket.kind = StreamKind::Listen;
    socket.state = StreamState::Listening;
    socket.local_port = port;
    socket.capacity = std::max<std::size_t>(capacity, 1u);
    socket.backlog_limit = backlog;
    trace("ptp " + std::to_string(handle) + " listen port " + std::to_string(port));
    return handle;
}

int Client::ptp_open(std::uint16_t local_port, const Mac &peer, std::uint16_t peer_port, std::size_t capacity,
                     std::uint64_t retry_us, std::uint32_t retries) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    if (local_port == 0u) local_port = s.ephemeral_port(true);
    if (local_port == 0u) return 0;
    const int handle = s.next_handle++;
    StreamSocket &socket = s.streams[handle];
    socket.kind = StreamKind::Connect;
    socket.state = StreamState::Opening;
    socket.local_port = local_port;
    socket.peer = peer;
    socket.peer_port = peer_port;
    socket.capacity = std::max<std::size_t>(capacity, 1u);
    socket.retry_us = retry_us;
    socket.retries_left = retries;
    trace("ptp " + std::to_string(handle) + " open " + std::to_string(local_port) + " -> " + format_mac(peer) +
          " port " + std::to_string(peer_port));
    return handle;
}

int Client::ptp_accept(int listener) {
    std::lock_guard lock(impl_->mutex);
    Impl &s = *impl_;
    auto found = s.streams.find(listener);
    if (found == s.streams.end() || found->second.kind != StreamKind::Listen || !s.server) return 0;
    StreamSocket &listen = found->second;
    const auto now = Clock::now();
    while (!listen.backlog.empty() && now - listen.backlog.front().arrived > kPendingConnectionLife)
        listen.backlog.pop_front();
    if (listen.backlog.empty()) return 0;
    const PendingConnection pending = listen.backlog.front();
    listen.backlog.pop_front();
    const int handle = s.next_handle++;
    StreamSocket &socket = s.streams[handle];
    socket.kind = StreamKind::Accept;
    // The relay may still refuse, but to the game an accepted stream is open;
    // a refusal looks like the peer closing it at once.
    socket.state = StreamState::Established;
    socket.local_port = listen.local_port;
    socket.peer = pending.mac;
    socket.peer_port = pending.port;
    socket.capacity = listen.capacity;
    // The network thread opens its relay connection.
    socket.accept_pending = true;
    report("accepted a stream from " + format_mac(pending.mac));
    return handle;
}

StreamInfo Client::ptp_info(int handle) const {
    std::lock_guard lock(impl_->mutex);
    StreamInfo info;
    const auto found = impl_->streams.find(handle);
    if (found == impl_->streams.end()) return info;
    const StreamSocket &socket = found->second;
    info.state = socket.state;
    info.local_port = socket.local_port;
    info.peer = socket.peer;
    info.peer_port = socket.peer_port;
    info.readable = socket.kind == StreamKind::Listen ? socket.backlog.size() : socket.received.size();
    info.unsent = socket.link.output.size();
    // The game's data among the queued bytes. The game's stream messages are
    // small, so the queue holds at most a block or two, each with a 4-byte
    // size in front; before the relay link is up it holds the init record.
    info.unsent_data = socket.link.open && socket.link.output.size() > relay::kPtpHeaderSize
                           ? socket.link.output.size() - relay::kPtpHeaderSize
                           : 0u;
    info.capacity = socket.capacity;
    info.sent = socket.sent_total;
    info.received = socket.received_total;
    return info;
}

bool Client::ptp_exists(int handle) const {
    std::lock_guard lock(impl_->mutex);
    const auto found = impl_->streams.find(handle);
    return found != impl_->streams.end() && !found->second.closing;
}

std::size_t Client::ptp_send(int handle, const void *data, std::size_t size) {
    std::lock_guard lock(impl_->mutex);
    auto found = impl_->streams.find(handle);
    if (found == impl_->streams.end()) return 0u;
    StreamSocket &socket = found->second;
    if (socket.state != StreamState::Established || !socket.link.active()) return 0u;
    const std::size_t queued = socket.link.output.size();
    const std::size_t room = queued >= socket.capacity ? 0u : socket.capacity - queued;
    const std::size_t count = std::min(size, room);
    const char *bytes = static_cast<const char *>(data);
    for (std::size_t done = 0; done < count;) {
        const std::size_t block = std::min(count - done, relay::kPtpBlockMax);
        wire::put32(socket.link.output, static_cast<std::uint32_t>(block));
        socket.link.output.append(bytes + done, block);
        done += block;
    }
    socket.sent_total += count;
    if (count != 0u) {
        ++impl_->traffic.packets_out;
        impl_->traffic.bytes_out += count;
    }
    if (count != 0u) trace("relay > ptp " + std::to_string(handle) + " data " + std::to_string(count));
    return count;
}

std::size_t Client::ptp_receive(int handle, void *data, std::size_t size) {
    std::lock_guard lock(impl_->mutex);
    auto found = impl_->streams.find(handle);
    if (found == impl_->streams.end()) return 0u;
    StreamSocket &socket = found->second;
    const std::size_t count = std::min(size, socket.received.size());
    std::memcpy(data, socket.received.data(), count);
    socket.received.erase(0, count);
    return count;
}

void Client::ptp_close(int handle) {
    std::lock_guard lock(impl_->mutex);
    if (auto found = impl_->streams.find(handle); found != impl_->streams.end()) {
        found->second.closing = true;
        trace("ptp " + std::to_string(handle) + " close");
    }
}

std::vector<int> Client::ptp_handles() const {
    std::lock_guard lock(impl_->mutex);
    std::vector<int> handles;
    for (const auto &[handle, socket] : impl_->streams)
        if (!socket.closing) handles.push_back(handle);
    return handles;
}

} // namespace mhp3rd::adhoc
