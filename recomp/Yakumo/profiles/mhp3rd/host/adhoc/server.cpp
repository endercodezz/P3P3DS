#include "adhoc/server.hpp"

#include "adhoc/client.hpp"
#include "adhoc/sockets.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>

namespace mhp3rd::adhoc {
namespace {

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;
using namespace net;

// How long the server thread sleeps in poll() when nothing happens; timeouts
// are checked this often, and stop() waits at most this long.
constexpr int kPollIntervalMs = 20;
// A connection that has not logged in by then is dropped.
constexpr auto kLoginTimeout = milliseconds(5000);
// Clients ping every few seconds; one silent this long is gone.
constexpr auto kIdleTimeout = milliseconds(30000);
// A relay connection that sends no complete init record by then is dropped.
constexpr auto kInitTimeout = milliseconds(5000);
// A connecting stream waits this long for a listening socket, then as long
// again to be accepted.
constexpr auto kStreamWait = milliseconds(5000);
// Data queued towards one connection before it counts as not reading:
// datagrams are dropped, a stream stops being read, an adhocctl connection is
// closed.
constexpr std::size_t kQueueLimit = 512u * 1024u;
constexpr std::size_t kReadChunk = 16u * 1024u;
constexpr std::size_t kMaxAdhocctlConnections = 256u;
constexpr std::size_t kMaxRelayConnections = 2048u;

void log_event(const std::string &line, bool print) { Client::log("[adhoc-server] " + line, print); }
void log_detail(const std::string &line) { Client::log("[adhoc-server] " + line, false); }

bool address_in_use(int error) {
#if defined(_WIN32)
    return error == WSAEADDRINUSE || error == WSAEACCES;
#else
    return error == EADDRINUSE;
#endif
}

// A listening TCP socket on every interface: IPv6 with IPv4 mapped where the
// system has IPv6, IPv4 alone otherwise.
Socket open_listener(std::uint16_t port, std::string &error) {
    const auto finish = [](Socket s) {
        if (::listen(s, 64) != 0 || !set_nonblocking(s)) {
            close_socket(s);
            return kNoSocket;
        }
        return s;
    };
    const auto prepare = [](Socket s) {
#if defined(_WIN32)
        set_option(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
        set_option(s, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
    };
    Socket s = ::socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (s != kNoSocket) {
        prepare(s);
        set_option(s, IPPROTO_IPV6, IPV6_V6ONLY, 0);
        sockaddr_in6 address{};
        address.sin6_family = AF_INET6;
        address.sin6_addr = in6addr_any;
        address.sin6_port = htons(port);
        if (::bind(s, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == 0) return finish(s);
        const int failure = socket_error();
        close_socket(s);
        if (address_in_use(failure)) {
            error = "TCP port " + std::to_string(port) + " is in use by another program";
            return kNoSocket;
        }
    }
    s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == kNoSocket) {
        error = "cannot make a socket";
        return kNoSocket;
    }
    prepare(s);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (::bind(s, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) != 0) {
        const int failure = socket_error();
        close_socket(s);
        error = address_in_use(failure) ? "TCP port " + std::to_string(port) + " is in use by another program"
                                        : "cannot listen on TCP port " + std::to_string(port);
        return kNoSocket;
    }
    s = finish(s);
    if (s == kNoSocket) error = "cannot listen on TCP port " + std::to_string(port);
    return s;
}

void tune(Socket s) {
    set_option(s, IPPROTO_TCP, TCP_NODELAY, 1);
    set_option(s, SOL_SOCKET, SO_KEEPALIVE, 1);
    no_sigpipe(s);
}

// One accepted TCP connection.
struct Connection {
    Socket socket{kNoSocket};
    Address peer;
    std::string in;
    std::string out;
    Clock::time_point created{};
    Clock::time_point last_seen{};
    bool eof{};      // the peer closed its side; what it sent is still parsed
    bool closing{};  // close once `out` is written
    bool dead{};     // close now

    // Reads what is waiting, up to `limit` buffered bytes.
    void read(std::size_t limit) {
        char buffer[kReadChunk];
        for (int round = 0; round < 8 && in.size() < limit; ++round) {
            const auto count = ::recv(socket, buffer, static_cast<int>(sizeof(buffer)), 0);
            if (count > 0) {
                in.append(buffer, static_cast<std::size_t>(count));
                last_seen = Clock::now();
                if (static_cast<std::size_t>(count) < sizeof(buffer)) return;
                continue;
            }
            if (count == 0 || !would_block(socket_error())) eof = true;
            return;
        }
    }

    void write() {
        while (!out.empty()) {
            const auto count = ::send(socket, out.data(), static_cast<int>(out.size()), kSendFlags);
            if (count > 0) {
                out.erase(0, static_cast<std::size_t>(count));
                continue;
            }
            if (count < 0 && would_block(socket_error())) return;
            dead = true;
            return;
        }
        if (closing) dead = true;
    }
};

struct Player {
    Connection link;
    bool logged_in{};
    Mac mac{};
    std::string nickname;
    std::string product;
    std::optional<std::string> group;
    std::uint32_t id{};
    Clock::time_point login_time{};
};

enum class RelayKind { Pending, Datagram, Listen, Connect, Accept };

struct RelaySession {
    Connection link;
    RelayKind kind{RelayKind::Pending};
    Mac mac{};
    std::uint16_t port{};
    Mac peer_mac{};
    std::uint16_t peer_port{};
    int partner{};                          // the paired stream connection, once paired
    bool announced{};                       // a connecting stream was reported to the listener
    Clock::time_point announced_at{};
};

std::string group_key(const std::string &product, const std::string &group) { return product + '\n' + group; }

} // namespace

struct Server::Impl {
    WinsockSession winsock;
    mutable std::mutex mutex;  // guards `running_flag`, `status_snapshot` and start/stop
    std::thread thread;
    std::atomic<bool> quit{false};
    bool running_flag{};
    ServerStatus status_snapshot;

    // Owned by the server thread while it runs.
    ServerConfig config;
    Socket ctl_listener{kNoSocket};
    Socket relay_listener{kNoSocket};
    Clock::time_point started{};
    int next_id{1};
    std::map<int, Player> players;
    std::map<int, RelaySession> sessions;
    std::map<std::string, Mac> group_hosts;  // group_key -> the MAC that created it
    std::uint32_t next_player_id{1};
    std::uint64_t relayed_packets{};
    std::uint64_t relayed_bytes{};
    std::uint64_t dropped{};
    Clock::time_point published{};

    // adhocctl ---------------------------------------------------------------

    std::uint32_t make_player_id(const Address &address) {
        // The PRO protocol names players by IPv4 address; clients that relay
        // everything only need the numbers to be unique. So the address when
        // it is unique and useful, otherwise a small number.
        const std::uint32_t v4 = address.ipv4();
        const auto taken = [&](std::uint32_t id) {
            for (const auto &[key, player] : players)
                if (player.logged_in && player.id == id) return true;
            return false;
        };
        if (v4 != 0u && !is_loopback_ipv4(v4) && !taken(v4)) return v4;
        for (;;) {
            const std::uint32_t id = next_player_id++;
            if (next_player_id >= 0x00FFFFFFu) next_player_id = 1u;
            if (!taken(id)) return id;
        }
    }

    std::vector<Player *> group_members(const std::string &product, const std::string &group, const Player *except) {
        std::vector<Player *> members;
        for (auto &[key, player] : players)
            if (&player != except && player.logged_in && !player.link.dead && player.group == group &&
                player.product == product)
                members.push_back(&player);
        return members;
    }

    void send(Player &player, const std::string &packet) {
        if (player.link.dead) return;
        player.link.out += packet;
        if (player.link.out.size() > kQueueLimit) {
            log_event(player.nickname + " is not reading; disconnecting", config.print_events);
            drop_player(player);
        }
    }

    void leave_group(Player &player) {
        if (!player.group) return;
        const std::string group = *player.group;
        player.group.reset();
        const auto members = group_members(player.product, group, &player);
        for (Player *member : members) send(*member, ctl::peer_left(player.id));
        if (members.empty()) group_hosts.erase(group_key(player.product, group));
        log_event(player.nickname + " left group " + group, config.print_events);
    }

    void drop_player(Player &player) {
        if (player.link.dead) return;
        player.link.dead = true;
        leave_group(player);
    }

    void join_group(Player &player, const std::string &group) {
        leave_group(player);
        const std::string key = group_key(player.product, group);
        const auto host = group_hosts.try_emplace(key, player.mac).first->second;
        for (Player *member : group_members(player.product, group, &player)) {
            send(*member, ctl::peer_joined(player.nickname, player.mac, player.id));
            send(player, ctl::peer_joined(member->nickname, member->mac, member->id));
        }
        player.group = group;
        send(player, ctl::joined(host));
        log_event(player.nickname + " joined group " + group, config.print_events);
    }

    bool login(Player &player, const char *packet) {
        const Mac mac = wire::get_mac(packet + 1);
        std::string nickname = wire::get_fixed(packet + 7, ctl::kNicknameLength);
        const std::string product = wire::get_fixed(packet + 7 + ctl::kNicknameLength, ctl::kProductCodeLength);
        const bool mac_ok = std::any_of(mac.begin() + 1, mac.end(), [](std::uint8_t b) { return b != 0u; });
        const bool product_ok =
            product.size() == ctl::kProductCodeLength && std::all_of(product.begin(), product.end(), [](char c) {
                return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
            });
        if (!mac_ok || !product_ok) {
            log_event("refused a login from " + player.link.peer.host() + ": bad address or product code",
                      config.print_events);
            return false;
        }
        // The same MAC again is the same player reconnecting.
        for (auto &[key, other] : players) {
            if (&other == &player || !other.logged_in || other.link.dead || other.mac != mac) continue;
            log_event(other.nickname + " logged in again; closing the older connection", config.print_events);
            drop_player(other);
        }
        player.logged_in = true;
        player.mac = mac;
        player.nickname = nickname.empty() ? format_mac(mac) : nickname;
        player.product = product;
        player.id = make_player_id(player.link.peer);
        player.login_time = Clock::now();
        log_event(player.nickname + " (" + format_mac(mac) + ", " + product + ") logged in from " +
                      player.link.peer.host(),
                  config.print_events);
        return true;
    }

    void scan(Player &player) {
        const std::string prefix = player.product + '\n';
        for (const auto &[key, host] : group_hosts)
            if (key.compare(0, prefix.size(), prefix) == 0) send(player, ctl::scan_result(key.substr(prefix.size()), host));
        send(player, ctl::opcode_only(ctl::kScanComplete));
    }

    void chat(Player &player, const char *packet) {
        if (!player.group) return;
        const std::string message = wire::get_fixed(packet + 1, ctl::kChatLength);
        for (Player *member : group_members(player.product, *player.group, &player))
            send(*member, ctl::chat_message(message, player.nickname));
    }

    void handle_adhocctl(Player &player) {
        std::string &in = player.link.in;
        std::size_t offset = 0;
        while (offset < in.size() && !player.link.dead) {
            const auto opcode = static_cast<std::uint8_t>(in[offset]);
            const std::size_t size = ctl::client_packet_size(opcode);
            if (size == 0u || (!player.logged_in && opcode != ctl::kLogin && opcode != ctl::kPing)) {
                log_event("closing a connection from " + player.link.peer.host() + ": unexpected opcode " +
                              std::to_string(opcode),
                          config.print_events);
                drop_player(player);
                break;
            }
            if (in.size() - offset < size) break;
            const char *packet = in.data() + offset;
            offset += size;
            switch (opcode) {
            case ctl::kLogin:
                if (!player.logged_in && !login(player, packet)) drop_player(player);
                break;
            case ctl::kConnect: join_group(player, wire::get_fixed(packet + 1, ctl::kGroupNameLength)); break;
            case ctl::kDisconnect: leave_group(player); break;
            case ctl::kScan: scan(player); break;
            case ctl::kChat: chat(player, packet); break;
            default: break;  // ping
            }
        }
        in.erase(0, std::min(offset, in.size()));
    }

    // Relay --------------------------------------------------------------------

    RelaySession *find_session(RelayKind kind, const Mac &mac, std::uint16_t port, int except) {
        for (auto &[key, session] : sessions)
            if (key != except && !session.link.dead && session.kind == kind && session.mac == mac &&
                session.port == port)
                return &session;
        return nullptr;
    }

    bool same_group(const Mac &a, const Mac &b) {
        const Player *first = nullptr;
        const Player *second = nullptr;
        for (const auto &[key, player] : players) {
            if (!player.logged_in || player.link.dead) continue;
            if (player.mac == a) first = &player;
            if (player.mac == b) second = &player;
        }
        return first != nullptr && second != nullptr && first->group && first->group == second->group &&
               first->product == second->product;
    }

    void deliver_datagram(const RelaySession &from, RelaySession &to, const char *data, std::size_t size) {
        if (to.link.out.size() > kQueueLimit) {
            ++dropped;
            return;
        }
        to.link.out += relay::pdp_header(from.mac, from.port, static_cast<std::uint32_t>(size));
        to.link.out.append(data, size);
        ++relayed_packets;
        relayed_bytes += size;
    }

    void handle_datagrams(int key, RelaySession &session) {
        std::string &in = session.link.in;
        std::size_t offset = 0;
        while (in.size() - offset >= relay::kPdpHeaderSize) {
            const Mac destination = wire::get_mac(in.data() + offset);
            const std::uint16_t port = wire::get16(in.data() + offset + 8);
            const std::uint32_t size = wire::get32(in.data() + offset + 10);
            if (size > relay::kPdpBlockMax * 2u) {
                log_detail("closing datagram socket " + format_mac(session.mac) + " port " +
                           std::to_string(session.port) + ": oversized datagram");
                session.link.dead = true;
                return;
            }
            if (in.size() - offset - relay::kPdpHeaderSize < size) break;
            const char *data = in.data() + offset + relay::kPdpHeaderSize;
            offset += relay::kPdpHeaderSize + size;
            if (destination == kBroadcastMac) {
                for (auto &[other_key, other] : sessions)
                    if (other_key != key && !other.link.dead && other.kind == RelayKind::Datagram &&
                        other.port == port && other.mac != session.mac && same_group(session.mac, other.mac))
                        deliver_datagram(session, other, data, size);
            } else if (RelaySession *target = find_session(RelayKind::Datagram, destination, port, key)) {
                deliver_datagram(session, *target, data, size);
            } else {
                ++dropped;
            }
        }
        in.erase(0, offset);
    }

    void handle_stream(RelaySession &session) {
        const auto found = sessions.find(session.partner);
        if (found == sessions.end() || found->second.link.dead) return;
        RelaySession &partner = found->second;
        std::string &in = session.link.in;
        std::size_t offset = 0;
        while (in.size() - offset >= relay::kPtpHeaderSize && partner.link.out.size() <= kQueueLimit) {
            const std::uint32_t size = wire::get32(in.data() + offset);
            if (size > relay::kPtpBlockMax * 2u) {
                log_detail("closing a stream of " + format_mac(session.mac) + ": oversized block");
                session.link.dead = true;
                return;
            }
            if (in.size() - offset - relay::kPtpHeaderSize < size) break;
            partner.link.out.append(in, offset, relay::kPtpHeaderSize + size);
            offset += relay::kPtpHeaderSize + size;
            ++relayed_packets;
            relayed_bytes += size;
        }
        in.erase(0, offset);
    }

    // Pairs an accepting connection with the connecting one it names.
    void accept_stream(int key, RelaySession &session) {
        for (auto &[other_key, other] : sessions) {
            if (other_key == key || other.link.dead || other.kind != RelayKind::Connect || other.partner != 0 ||
                other.mac != session.peer_mac || other.port != session.peer_port ||
                other.peer_mac != session.mac || other.peer_port != session.port)
                continue;
            other.partner = key;
            session.partner = other_key;
            other.link.out += relay::ptp_notice(session.mac, session.port);
            session.link.out += relay::ptp_notice(other.mac, other.port);
            log_detail("stream " + format_mac(other.mac) + " port " + std::to_string(other.port) + " <-> " +
                       format_mac(session.mac) + " port " + std::to_string(session.port) + " established");
            // Data the connecting side sent before it was accepted.
            handle_stream(other);
            return;
        }
        log_detail("refusing an accept from " + format_mac(session.mac) + " port " + std::to_string(session.port) +
                   ": no such connection is waiting");
        session.link.dead = true;
    }

    // Offers a connecting stream to its listening socket once one exists.
    void offer_stream(RelaySession &session, Clock::time_point now) {
        if (session.announced) return;
        RelaySession *listener = find_session(RelayKind::Listen, session.peer_mac, session.peer_port, 0);
        if (listener == nullptr) return;
        listener->link.out += relay::ptp_notice(session.mac, session.port);
        session.announced = true;
        session.announced_at = now;
        log_detail("stream request " + format_mac(session.mac) + " port " + std::to_string(session.port) + " -> " +
                   format_mac(session.peer_mac) + " port " + std::to_string(session.peer_port));
    }

    void handle_init(int key, RelaySession &session, Clock::time_point now) {
        std::string &in = session.link.in;
        if (in.size() < relay::kInitSize) return;
        const std::uint32_t type = wire::get32(in.data());
        session.mac = wire::get_mac(in.data() + 4);
        session.port = wire::get16(in.data() + 12);
        session.peer_mac = wire::get_mac(in.data() + 14);
        session.peer_port = wire::get16(in.data() + 22);
        in.erase(0, relay::kInitSize);
        const auto replace = [&](RelayKind kind) {
            if (RelaySession *older = find_session(kind, session.mac, session.port, key)) older->link.dead = true;
            session.kind = kind;
        };
        switch (type) {
        case relay::kInitPdp:
            replace(RelayKind::Datagram);
            log_detail("datagram socket " + format_mac(session.mac) + " port " + std::to_string(session.port));
            break;
        case relay::kInitPtpListen:
            replace(RelayKind::Listen);
            log_detail("listening socket " + format_mac(session.mac) + " port " + std::to_string(session.port));
            break;
        case relay::kInitPtpConnect:
            session.kind = RelayKind::Connect;
            offer_stream(session, now);
            break;
        case relay::kInitPtpAccept:
            session.kind = RelayKind::Accept;
            accept_stream(key, session);
            break;
        default:
            log_detail("closing a relay connection from " + session.link.peer.host() + ": unknown init type");
            session.link.dead = true;
            break;
        }
    }

    void handle_relay(int key, RelaySession &session, Clock::time_point now) {
        if (session.kind == RelayKind::Pending) handle_init(key, session, now);
        switch (session.kind) {
        case RelayKind::Datagram: handle_datagrams(key, session); break;
        case RelayKind::Listen: session.link.in.clear(); break;
        case RelayKind::Connect:
        case RelayKind::Accept:
            if (session.partner != 0) handle_stream(session);
            break;
        case RelayKind::Pending: break;
        }
    }

    // Whether to read more from a relay connection now.
    bool wants_input(const RelaySession &session) const {
        if (session.kind == RelayKind::Pending) return session.link.in.size() < relay::kInitSize;
        if (session.kind == RelayKind::Connect || session.kind == RelayKind::Accept) {
            if (session.partner == 0) return session.link.in.size() < kQueueLimit;
            const auto found = sessions.find(session.partner);
            return found != sessions.end() && found->second.link.out.size() <= kQueueLimit &&
                   session.link.in.size() < kQueueLimit;
        }
        return true;
    }

    // Upkeep ---------------------------------------------------------------------

    void accept_connections(Socket listener, bool relay, Clock::time_point now) {
        for (int round = 0; round < 32; ++round) {
            Address address;
            address.length = sizeof(address.storage);
            const Socket s = ::accept(listener, reinterpret_cast<sockaddr *>(&address.storage), &address.length);
            if (s == kNoSocket) return;
            const std::size_t count = relay ? sessions.size() : players.size();
            if (count >= (relay ? kMaxRelayConnections : kMaxAdhocctlConnections) || !set_nonblocking(s)) {
                close_socket(s);
                continue;
            }
            tune(s);
            Connection link;
            link.socket = s;
            link.peer = address;
            link.created = now;
            link.last_seen = now;
            const int key = next_id++;
            if (relay) sessions[key].link = std::move(link);
            else players[key].link = std::move(link);
        }
    }

    void expire(Clock::time_point now) {
        for (auto &[key, player] : players) {
            if (player.link.dead) continue;
            if (!player.logged_in && now - player.link.created > kLoginTimeout) {
                player.link.dead = true;
            } else if (player.logged_in && now - player.link.last_seen > kIdleTimeout) {
                log_event(player.nickname + " timed out", config.print_events);
                drop_player(player);
            }
        }
        for (auto &[key, session] : sessions) {
            if (session.link.dead) continue;
            if (session.kind == RelayKind::Pending && now - session.link.created > kInitTimeout) {
                session.link.dead = true;
            } else if (session.kind == RelayKind::Connect && session.partner == 0) {
                offer_stream(session, now);
                const bool expired = session.announced ? now - session.announced_at > kStreamWait
                                                       : now - session.link.created > kStreamWait;
                if (expired) {
                    log_detail("stream request " + format_mac(session.mac) + " port " + std::to_string(session.port) +
                               (session.announced ? " was not accepted" : " found no listening socket"));
                    session.link.dead = true;
                }
            }
        }
    }

    void remove_closed() {
        for (auto it = players.begin(); it != players.end();) {
            Player &player = it->second;
            if (player.link.eof && !player.link.dead) {
                if (player.logged_in) log_event(player.nickname + " disconnected", config.print_events);
                drop_player(player);
            }
            if (player.link.dead) {
                close_socket(player.link.socket);
                it = players.erase(it);
            } else {
                ++it;
            }
        }
        // A stream's partner is closed once it has sent what it was given.
        for (auto &[key, session] : sessions) {
            if (session.link.eof && !session.link.dead) session.link.dead = true;
            if (!session.link.dead || session.partner == 0) continue;
            if (auto partner = sessions.find(session.partner); partner != sessions.end()) {
                partner->second.link.closing = true;
                partner->second.partner = 0;
            }
            session.partner = 0;
        }
        for (auto it = sessions.begin(); it != sessions.end();) {
            if (it->second.link.dead) {
                close_socket(it->second.link.socket);
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }
    }

    void publish(Clock::time_point now) {
        ServerStatus status;
        status.running = true;
        status.adhocctl_port = config.adhocctl_port;
        status.relay_port = relay_port_for(config.adhocctl_port);
        status.uptime_ms = static_cast<std::uint64_t>(std::chrono::duration_cast<milliseconds>(now - started).count());
        status.connections = players.size();
        for (const auto &[key, player] : players) {
            if (!player.logged_in || player.link.dead) continue;
            ServerPlayer entry;
            entry.nickname = player.nickname;
            entry.mac = player.mac;
            entry.address = player.link.peer.host();
            entry.product = player.product;
            entry.group = player.group;
            entry.online_ms =
                static_cast<std::uint64_t>(std::chrono::duration_cast<milliseconds>(now - player.login_time).count());
            status.players.push_back(std::move(entry));
        }
        status.groups = group_hosts.size();
        status.relay_sessions = sessions.size();
        for (const auto &[key, session] : sessions)
            if (session.kind == RelayKind::Connect && session.partner != 0) ++status.streams;
        status.relayed_packets = relayed_packets;
        status.relayed_bytes = relayed_bytes;
        status.dropped = dropped;
        std::lock_guard lock(mutex);
        status_snapshot = std::move(status);
    }

    void run() {
        std::vector<PollEntry> entries;
        std::vector<Connection *> links;
        while (!quit) {
            entries.clear();
            links.clear();
            for (const Socket listener : {ctl_listener, relay_listener}) {
                PollEntry entry{};
                entry.fd = listener;
                entry.events = POLLIN;
                entries.push_back(entry);
                links.push_back(nullptr);
            }
            const auto add = [&](Connection &link, bool read) {
                PollEntry entry{};
                entry.fd = link.socket;
                entry.events = static_cast<short>((read ? POLLIN : 0) | (link.out.empty() ? 0 : POLLOUT));
                entries.push_back(entry);
                links.push_back(&link);
            };
            for (auto &[key, player] : players) add(player.link, player.link.in.size() < kQueueLimit);
            for (auto &[key, session] : sessions) add(session.link, wants_input(session));

            const int ready = poll_sockets(entries.data(), entries.size(), kPollIntervalMs);
            const auto now = Clock::now();
            if (ready > 0) {
                if ((entries[0].revents & POLLIN) != 0) accept_connections(ctl_listener, false, now);
                if ((entries[1].revents & POLLIN) != 0) accept_connections(relay_listener, true, now);
                for (std::size_t i = 2; i < entries.size(); ++i) {
                    Connection &link = *links[i];
                    if ((entries[i].revents & (POLLIN | POLLHUP | POLLERR)) != 0 && (entries[i].events & POLLIN) != 0)
                        link.read(kQueueLimit * 2u);
                    else if ((entries[i].revents & (POLLHUP | POLLERR)) != 0)
                        link.eof = true;
                }
            }
            for (auto &[key, player] : players)
                if (!player.link.in.empty() && !player.link.dead) handle_adhocctl(player);
            for (auto &[key, session] : sessions)
                if (!session.link.dead && (!session.link.in.empty() || session.kind == RelayKind::Pending))
                    handle_relay(key, session, now);
            expire(now);
            remove_closed();
            for (auto &[key, player] : players) player.link.write();
            for (auto &[key, session] : sessions) session.link.write();
            remove_closed();
            if (now - published >= milliseconds(250)) {
                published = now;
                publish(now);
            }
        }
        for (auto &[key, player] : players) close_socket(player.link.socket);
        for (auto &[key, session] : sessions) close_socket(session.link.socket);
        players.clear();
        sessions.clear();
        group_hosts.clear();
        close_socket(ctl_listener);
        close_socket(relay_listener);
        ctl_listener = relay_listener = kNoSocket;
    }
};

Server::Server() : impl_(std::make_unique<Impl>()) {}

// Safe whenever it runs: a running server is stopped and its thread joined,
// and nothing escapes.
Server::~Server() {
    try {
        stop();
    } catch (...) {
    }
}

bool Server::start(const ServerConfig &config) {
    stop();
    Impl &s = *impl_;
    std::string error;
    const std::uint16_t relay_port = relay_port_for(config.adhocctl_port);
    Socket ctl = open_listener(config.adhocctl_port, error);
    Socket relay = ctl == kNoSocket ? kNoSocket : open_listener(relay_port, error);
    if (ctl == kNoSocket || relay == kNoSocket) {
        if (ctl != kNoSocket) close_socket(ctl);
        log_event("cannot start: " + error, true);
        std::lock_guard lock(s.mutex);
        s.status_snapshot = ServerStatus{};
        s.status_snapshot.error = error;
        s.status_snapshot.adhocctl_port = config.adhocctl_port;
        s.status_snapshot.relay_port = relay_port;
        return false;
    }
    s.config = config;
    s.ctl_listener = ctl;
    s.relay_listener = relay;
    s.started = Clock::now();
    s.published = Clock::time_point{};
    s.relayed_packets = s.relayed_bytes = s.dropped = 0u;
    s.quit = false;
    {
        std::lock_guard lock(s.mutex);
        s.running_flag = true;
        s.status_snapshot = ServerStatus{};
        s.status_snapshot.running = true;
        s.status_snapshot.adhocctl_port = config.adhocctl_port;
        s.status_snapshot.relay_port = relay_port;
    }
    s.thread = std::thread([&s] { s.run(); });
    log_event("listening on TCP " + std::to_string(config.adhocctl_port) + " (matchmaking) and " +
                  std::to_string(relay_port) + " (relay)",
              true);
    return true;
}

void Server::stop() {
    Impl &s = *impl_;
    if (!s.thread.joinable()) return;
    s.quit = true;
    s.thread.join();
    std::lock_guard lock(s.mutex);
    s.running_flag = false;
    s.status_snapshot = ServerStatus{};
    log_event("stopped; everyone was disconnected", true);
}

bool Server::running() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->running_flag;
}

ServerStatus Server::status() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->status_snapshot;
}

} // namespace mhp3rd::adhoc
