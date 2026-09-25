#include "adhoc/discovery.hpp"

#include "adhoc/client.hpp"
#include "adhoc/protocol.hpp"
#include "adhoc/sockets.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <random>
#include <thread>

#if defined(_WIN32)
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace mhp3rd::adhoc {
namespace {

using Clock = std::chrono::steady_clock;
using std::chrono::milliseconds;
using namespace net;

constexpr char kMagic[4] = {'Y', 'K', 'A', 'H'};
constexpr std::uint8_t kVersion = 1u;
constexpr std::uint8_t kKindAnnouncement = 1u;
constexpr std::uint8_t kKindQuery = 2u;
constexpr std::size_t kQuerySize = 6u;
constexpr std::size_t kAnnouncementSize = 58u;
constexpr std::size_t kProductField = 10u;
constexpr std::size_t kNameField = 32u;
// 239.255.27.14: administratively scoped, so routers keep it on the site.
constexpr std::uint32_t kMulticastGroup = (239u << 24u) | (255u << 16u) | (27u << 8u) | 14u;

constexpr auto kAnnounceInterval = milliseconds(1000);
constexpr auto kQueryInterval = milliseconds(2000);
constexpr auto kQueryLife = milliseconds(10000);
constexpr auto kHostLife = milliseconds(5000);
constexpr auto kInterfaceRefresh = milliseconds(10000);
constexpr int kPollIntervalMs = 100;

void log_detail(const std::string &line) { Client::log("[adhoc-discovery] " + line, false); }

std::string classify(const std::string &interface, std::uint32_t host_order) {
    std::string name = interface;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    const auto has = [&](const char *part) { return name.find(part) != std::string::npos; };
    if ((host_order >> 22u) == ((100u << 2u) | 1u) || has("tailscale")) return "Tailscale";  // 100.64.0.0/10
    if (name.rfind("zt", 0) == 0 || has("zerotier")) return "ZeroTier";
    if (name.rfind("ham", 0) == 0 || has("hamachi")) return "Hamachi";
    if (name.rfind("utun", 0) == 0 || name.rfind("tun", 0) == 0 || name.rfind("tap", 0) == 0 ||
        name.rfind("wg", 0) == 0 || name.rfind("ppp", 0) == 0 || name.rfind("ipsec", 0) == 0 || has("wireguard") ||
        has("vpn"))
        return "VPN";
    const std::uint32_t a = host_order >> 24u;
    const std::uint32_t b = (host_order >> 16u) & 0xFFu;
    const bool private_range = a == 10u || (a == 172u && b >= 16u && b < 32u) || (a == 192u && b == 168u);
    if (private_range) return "Local network";
    if (a == 169u && b == 254u) return "Link-local";
    return "Internet";
}

std::string ipv4_text(std::uint32_t network_order) {
    return Address::ipv4_address(network_order, 0u).host();
}

struct Interface {
    LocalAddress address;
    std::uint32_t ipv4{};        // network order
    std::uint32_t broadcast{};   // network order, 0 without one
    bool multicast{};
};

std::vector<Interface> interfaces() {
    std::vector<Interface> result;
#if defined(_WIN32)
    ULONG size = 16u * 1024u;
    std::vector<unsigned char> buffer;
    ULONG status = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 3 && status == ERROR_BUFFER_OVERFLOW; ++attempt) {
        buffer.resize(size);
        status = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &size);
    }
    if (status != NO_ERROR) return result;
    for (auto *adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()); adapter != nullptr;
         adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        std::string name;
        if (adapter->FriendlyName != nullptr) {
            const int length = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1, nullptr, 0, nullptr, nullptr);
            if (length > 1) {
                name.resize(static_cast<std::size_t>(length));
                WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1, name.data(), length, nullptr, nullptr);
                name.resize(static_cast<std::size_t>(length - 1));
            }
        }
        for (auto *unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
            const sockaddr *raw = unicast->Address.lpSockaddr;
            if (raw == nullptr || raw->sa_family != AF_INET) continue;
            Interface entry;
            entry.ipv4 = reinterpret_cast<const sockaddr_in *>(raw)->sin_addr.s_addr;
            const std::uint32_t host = ntohl(entry.ipv4);
            if ((host >> 24u) == 127u) continue;
            const unsigned prefix = unicast->OnLinkPrefixLength;
            if (prefix > 0u && prefix < 31u) entry.broadcast = htonl(host | (0xFFFFFFFFu >> prefix));
            entry.multicast = (adapter->Flags & IP_ADAPTER_NO_MULTICAST) == 0;
            entry.address.address = ipv4_text(entry.ipv4);
            entry.address.interface = name;
            entry.address.network = classify(name + " " + std::string(adapter->AdapterName), host);
            result.push_back(entry);
        }
    }
#else
    ifaddrs *list = nullptr;
    if (getifaddrs(&list) != 0) return result;
    for (ifaddrs *entry = list; entry != nullptr; entry = entry->ifa_next) {
        if (entry->ifa_addr == nullptr || entry->ifa_addr->sa_family != AF_INET) continue;
        if ((entry->ifa_flags & IFF_UP) == 0 || (entry->ifa_flags & IFF_LOOPBACK) != 0) continue;
        Interface item;
        item.ipv4 = reinterpret_cast<const sockaddr_in *>(entry->ifa_addr)->sin_addr.s_addr;
        if ((entry->ifa_flags & IFF_BROADCAST) != 0 && entry->ifa_broadaddr != nullptr &&
            entry->ifa_broadaddr->sa_family == AF_INET)
            item.broadcast = reinterpret_cast<const sockaddr_in *>(entry->ifa_broadaddr)->sin_addr.s_addr;
        item.multicast = (entry->ifa_flags & IFF_MULTICAST) != 0;
        item.address.address = ipv4_text(item.ipv4);
        item.address.interface = entry->ifa_name != nullptr ? entry->ifa_name : "";
        item.address.network = classify(item.address.interface, ntohl(item.ipv4));
        result.push_back(item);
    }
    freeifaddrs(list);
#endif
    return result;
}

std::string encode_announcement(const Announcement &info) {
    std::string out(kMagic, sizeof(kMagic));
    wire::put8(out, kVersion);
    wire::put8(out, kKindAnnouncement);
    wire::put16(out, info.adhocctl_port);
    wire::put16(out, info.relay_port);
    wire::put8(out, static_cast<std::uint8_t>(std::min(info.players, 255u)));
    wire::put8(out, 0u);
    wire::put32(out, info.session);
    wire::put_fixed(out, info.product.substr(0, kProductField - 1u), kProductField);
    // Cut on a character boundary so the name stays valid UTF-8.
    std::size_t length = std::min(info.name.size(), kNameField - 1u);
    while (length > 0u && length < info.name.size() &&
           (static_cast<unsigned char>(info.name[length]) & 0xC0u) == 0x80u)
        --length;
    wire::put_fixed(out, info.name.substr(0, length), kNameField);
    return out;
}

std::string encode_query() {
    std::string out(kMagic, sizeof(kMagic));
    wire::put8(out, kVersion);
    wire::put8(out, kKindQuery);
    return out;
}

// The packet's kind, or 0 when it is not one of ours.
std::uint8_t packet_kind(const char *data, std::size_t size) {
    if (size < kQuerySize || std::memcmp(data, kMagic, sizeof(kMagic)) != 0 ||
        static_cast<std::uint8_t>(data[4]) != kVersion)
        return 0u;
    const auto kind = static_cast<std::uint8_t>(data[5]);
    if (kind == kKindAnnouncement && size < kAnnouncementSize) return 0u;
    return kind;
}

Announcement decode_announcement(const char *data) {
    Announcement info;
    info.adhocctl_port = wire::get16(data + 6);
    info.relay_port = wire::get16(data + 8);
    info.players = static_cast<std::uint8_t>(data[10]);
    info.session = wire::get32(data + 12);
    info.product = wire::get_fixed(data + 16, kProductField);
    info.name = wire::get_fixed(data + 26, kNameField);
    return info;
}

Socket open_udp(std::uint32_t bind_address, std::uint16_t port, bool shared) {
    const Socket s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == kNoSocket) return s;
    if (shared) {
        set_option(s, SOL_SOCKET, SO_REUSEADDR, 1);
#if defined(SO_REUSEPORT) && !defined(__linux__)
        // BSD and macOS hand broadcasts to every socket on the port only with
        // this, so several instances on one machine all hear hosts.
        set_option(s, SOL_SOCKET, SO_REUSEPORT, 1);
#endif
    }
    set_option(s, SOL_SOCKET, SO_BROADCAST, 1);
    const Address address = Address::ipv4_address(bind_address, port);
    if (::bind(s, reinterpret_cast<const sockaddr *>(&address.storage), address.length) != 0 || !set_nonblocking(s)) {
        close_socket(s);
        return kNoSocket;
    }
    return s;
}

void send_to(Socket s, const std::string &packet, std::uint32_t ipv4, std::uint16_t port) {
    const Address address = Address::ipv4_address(ipv4, port);
    ::sendto(s, packet.data(), static_cast<int>(packet.size()), 0, reinterpret_cast<const sockaddr *>(&address.storage),
             address.length);
}

// "host" or "host:port" to an IPv4 address and port; a lookup, so only on the
// discovery thread.
std::optional<std::pair<std::uint32_t, std::uint16_t>> resolve_query_target(const std::string &text) {
    std::string host = text;
    std::uint16_t port = kAdhocctlPort;
    if (const auto colon = text.rfind(':'); colon != std::string::npos && text.find(':') == colon) {
        host = text.substr(0, colon);
        const unsigned long value = std::strtoul(text.c_str() + colon + 1u, nullptr, 10);
        if (value > 0u && value < 65536u) port = static_cast<std::uint16_t>(value);
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo *list = nullptr;
    if (host.empty() || getaddrinfo(host.c_str(), nullptr, &hints, &list) != 0 || list == nullptr) return std::nullopt;
    const std::uint32_t address = reinterpret_cast<const sockaddr_in *>(list->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(list);
    return std::make_pair(address, port);
}

} // namespace

std::vector<LocalAddress> local_addresses() {
    WinsockSession winsock;
    std::vector<LocalAddress> result;
    for (const Interface &entry : interfaces()) result.push_back(entry.address);
    // The ones others can reach first: LAN and VPN before link-local.
    std::stable_sort(result.begin(), result.end(), [](const LocalAddress &a, const LocalAddress &b) {
        return (a.network == "Link-local") < (b.network == "Link-local");
    });
    return result;
}

std::string local_host_name() {
    WinsockSession winsock;
    char name[256] = {};
    if (::gethostname(name, static_cast<int>(sizeof(name) - 1u)) != 0) return {};
    std::string text = name;
    // "machine.local" and the like: the first label is the name people know.
    if (const auto dot = text.find('.'); dot != std::string::npos && dot != 0u) text.resize(dot);
    return text;
}

std::string FoundHost::join_address() const {
    return info.adhocctl_port == kAdhocctlPort || info.adhocctl_port == 0u
               ? address
               : address + ":" + std::to_string(info.adhocctl_port);
}

struct Discovery::Impl {
    WinsockSession winsock;
    mutable std::mutex mutex;
    std::thread thread;
    std::atomic<bool> quit{false};

    // Under `mutex`.
    bool announcing{};
    std::uint16_t announce_port{};
    std::function<Announcement()> announce_info;
    std::uint32_t session{};
    bool listening{};
    std::map<std::string, Clock::time_point> queries;  // address -> asked until
    struct Heard {
        FoundHost host;
        Clock::time_point at{};
    };
    std::map<std::uint32_t, Heard> heard;  // by session
    DiscoveryStatus stats;

    // The thread's own.
    Socket announce_socket{kNoSocket};   // bound to the adhocctl port for queries, or any port
    std::uint16_t announce_socket_port{};
    Socket listen_socket{kNoSocket};
    std::vector<Interface> known_interfaces;
    std::vector<std::uint32_t> joined_groups;  // interfaces the listen socket joined the group on
    Clock::time_point interfaces_at{};
    Clock::time_point announced_at{};
    Clock::time_point queried_at{};

    Impl() {
        std::random_device random;
        session = static_cast<std::uint32_t>(random()) | 1u;
        thread = std::thread([this] { run(); });
    }

    ~Impl() {
        quit = true;
        if (thread.joinable()) thread.join();
        if (announce_socket != kNoSocket) close_socket(announce_socket);
        if (listen_socket != kNoSocket) close_socket(listen_socket);
    }

    void refresh_interfaces(Clock::time_point now) {
        if (now - interfaces_at < kInterfaceRefresh && interfaces_at != Clock::time_point{}) return;
        interfaces_at = now;
        known_interfaces = interfaces();
        if (listen_socket == kNoSocket) return;
        for (const Interface &entry : known_interfaces) {
            if (!entry.multicast ||
                std::find(joined_groups.begin(), joined_groups.end(), entry.ipv4) != joined_groups.end())
                continue;
            ip_mreq request{};
            request.imr_multiaddr.s_addr = htonl(kMulticastGroup);
            request.imr_interface.s_addr = entry.ipv4;
            if (setsockopt(listen_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char *>(&request),
                           sizeof(request)) == 0)
                joined_groups.push_back(entry.ipv4);
        }
    }

    void update_sockets(bool want_announce, std::uint16_t port, bool want_listen) {
        if ((!want_announce || port != announce_socket_port) && announce_socket != kNoSocket) {
            close_socket(announce_socket);
            announce_socket = kNoSocket;
            announce_socket_port = 0u;
        }
        if (want_announce && announce_socket == kNoSocket) {
            announce_socket = open_udp(htonl(INADDR_ANY), port, false);
            std::string note;
            if (announce_socket == kNoSocket) {
                // Announcing still works from any port; only typed addresses
                // cannot be checked.
                announce_socket = open_udp(htonl(INADDR_ANY), 0u, false);
                note = "UDP " + std::to_string(port) + " is taken; typed addresses cannot be checked";
            }
            announce_socket_port = port;
            if (announce_socket != kNoSocket) {
                const unsigned char ttl = 4;
                setsockopt(announce_socket, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char *>(&ttl),
                           sizeof(ttl));
            } else {
                note = "cannot make a UDP socket";
            }
            std::lock_guard lock(mutex);
            stats.announce_note = note;
        }
        if (want_listen && listen_socket == kNoSocket) {
            listen_socket = open_udp(htonl(INADDR_ANY), kDiscoveryPort, true);
            joined_groups.clear();
            interfaces_at = Clock::time_point{};
            std::lock_guard lock(mutex);
            stats.listening = listen_socket != kNoSocket;
            stats.listen_error =
                listen_socket == kNoSocket ? "cannot listen on UDP " + std::to_string(kDiscoveryPort) : "";
        }
    }

    void announce(const std::string &packet) {
        std::vector<std::uint32_t> sent;
        const auto once = [&](std::uint32_t target) {
            if (std::find(sent.begin(), sent.end(), target) != sent.end()) return;
            sent.push_back(target);
            send_to(announce_socket, packet, target, kDiscoveryPort);
        };
        once(htonl(INADDR_BROADCAST));
        std::size_t networks = 0;
        for (const Interface &entry : known_interfaces) {
            if (entry.broadcast != 0u) once(entry.broadcast);
            if (entry.multicast) {
                in_addr interface_address{};
                interface_address.s_addr = entry.ipv4;
                setsockopt(announce_socket, IPPROTO_IP, IP_MULTICAST_IF,
                           reinterpret_cast<const char *>(&interface_address), sizeof(interface_address));
                send_to(announce_socket, packet, htonl(kMulticastGroup), kDiscoveryPort);
            }
            ++networks;
        }
        std::lock_guard lock(mutex);
        ++stats.announcements_sent;
        if (stats.announce_note.empty() || stats.announce_note.rfind("on ", 0) == 0)
            stats.announce_note = "on " + std::to_string(networks) + " interface" + (networks == 1u ? "" : "s");
    }

    void receive(Socket s, bool answer_queries, const std::string &announcement) {
        char buffer[512];
        for (int round = 0; round < 32; ++round) {
            Address from;
            from.length = sizeof(from.storage);
            const auto count = ::recvfrom(s, buffer, static_cast<int>(sizeof(buffer)), 0,
                                          reinterpret_cast<sockaddr *>(&from.storage), &from.length);
            if (count <= 0) return;
            const std::uint8_t kind = packet_kind(buffer, static_cast<std::size_t>(count));
            if (kind == kKindQuery && answer_queries && !announcement.empty()) {
                ::sendto(s, announcement.data(), static_cast<int>(announcement.size()), 0,
                         reinterpret_cast<const sockaddr *>(&from.storage), from.length);
                std::lock_guard lock(mutex);
                ++stats.queries_answered;
            } else if (kind == kKindAnnouncement) {
                const Announcement info = decode_announcement(buffer);
                std::lock_guard lock(mutex);
                ++stats.announcements_heard;
                if (announcing && info.session == session) continue;
                auto [entry, added] = heard.try_emplace(info.session);
                // The first address a host is heard at stays, unless it is gone
                // quiet there; a host on several networks is one entry.
                const auto now = Clock::now();
                if (added || now - entry->second.at > milliseconds(3000) ||
                    entry->second.host.address == from.host())
                    entry->second.host.address = from.host();
                entry->second.host.info = info;
                entry->second.at = now;
                if (added) log_detail("heard host \"" + info.name + "\" at " + from.host());
            }
        }
    }

    void run() {
        while (!quit) {
            bool want_announce;
            bool want_listen;
            std::uint16_t port;
            std::function<Announcement()> info;
            std::vector<std::string> query_targets;
            const auto now = Clock::now();
            {
                std::lock_guard lock(mutex);
                want_announce = announcing;
                want_listen = listening;
                port = announce_port;
                info = announce_info;
                for (auto it = queries.begin(); it != queries.end();) {
                    if (now > it->second) {
                        it = queries.erase(it);
                    } else {
                        query_targets.push_back(it->first);
                        ++it;
                    }
                }
                for (auto it = heard.begin(); it != heard.end();)
                    it = now - it->second.at > kHostLife ? heard.erase(it) : std::next(it);
                stats.announcing = want_announce;
                stats.hosts = heard.size();
            }
            update_sockets(want_announce, port, want_listen);
            if (want_announce || want_listen) refresh_interfaces(now);

            std::string announcement;
            if (want_announce && info && announce_socket != kNoSocket) {
                Announcement current = info();
                {
                    std::lock_guard lock(mutex);
                    current.session = session;
                }
                current.adhocctl_port = port;
                current.relay_port = relay_port_for(port);
                announcement = encode_announcement(current);
                if (now - announced_at >= kAnnounceInterval) {
                    announced_at = now;
                    announce(announcement);
                }
            }
            if (listen_socket != kNoSocket && !query_targets.empty() && now - queried_at >= kQueryInterval) {
                queried_at = now;
                const std::string query = encode_query();
                for (const std::string &target : query_targets)
                    if (const auto resolved = resolve_query_target(target))
                        send_to(listen_socket, query, resolved->first, resolved->second);
            }

            PollEntry entries[2]{};
            std::size_t count = 0;
            for (const Socket s : {announce_socket, listen_socket}) {
                if (s == kNoSocket) continue;
                entries[count].fd = s;
                entries[count].events = POLLIN;
                ++count;
            }
            if (poll_sockets(entries, count, kPollIntervalMs) <= 0) continue;
            for (std::size_t i = 0; i < count; ++i) {
                if ((entries[i].revents & POLLIN) == 0) continue;
                const bool is_announce_socket = entries[i].fd == announce_socket;
                receive(entries[i].fd, is_announce_socket, announcement);
            }
        }
    }
};

// Never destroyed; see shutdown().
Discovery &Discovery::get() {
    static Discovery *discovery = new Discovery;
    return *discovery;
}

void Discovery::shutdown() noexcept {
    try {
        {
            std::lock_guard lock(impl_->mutex);
            impl_->announcing = false;
            impl_->announce_info = nullptr;
            impl_->listening = false;
        }
        impl_->quit = true;
        if (impl_->thread.joinable()) impl_->thread.join();
    } catch (...) {
    }
}

Discovery::Discovery() : impl_(std::make_unique<Impl>()) {}
Discovery::~Discovery() = default;

void Discovery::start_announcing(std::uint16_t adhocctl_port, std::function<Announcement()> info) {
    std::lock_guard lock(impl_->mutex);
    impl_->announcing = true;
    impl_->announce_port = adhocctl_port;
    impl_->announce_info = std::move(info);
    impl_->stats.announcements_sent = 0u;
    impl_->stats.queries_answered = 0u;
}

void Discovery::stop_announcing() {
    std::lock_guard lock(impl_->mutex);
    impl_->announcing = false;
    impl_->announce_info = nullptr;
    // A new session gets a new number, so joiners do not mix it up with this one.
    impl_->session += 2u;
}

void Discovery::start_listening() {
    std::lock_guard lock(impl_->mutex);
    impl_->listening = true;
}

void Discovery::query(const std::vector<std::string> &addresses) {
    std::lock_guard lock(impl_->mutex);
    impl_->listening = true;
    const auto until = Clock::now() + kQueryLife;
    for (const std::string &address : addresses)
        if (!address.empty()) impl_->queries[address] = until;
}

std::vector<FoundHost> Discovery::hosts() const {
    std::lock_guard lock(impl_->mutex);
    std::vector<FoundHost> result;
    const auto now = Clock::now();
    for (const auto &[session, entry] : impl_->heard) {
        if (impl_->announcing && session == impl_->session) continue;
        FoundHost host = entry.host;
        host.heard_ms = static_cast<std::uint64_t>(std::chrono::duration_cast<milliseconds>(now - entry.at).count());
        result.push_back(std::move(host));
    }
    std::sort(result.begin(), result.end(),
              [](const FoundHost &a, const FoundHost &b) { return a.info.name < b.info.name; });
    return result;
}

DiscoveryStatus Discovery::status() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->stats;
}

} // namespace mhp3rd::adhoc
