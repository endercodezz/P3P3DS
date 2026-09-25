#pragma once

// Finding a hosted session on the local network, and the addresses a host can
// be reached at.
//
// A host announces itself once a second by UDP to port 27314: to the
// broadcast address of each of its IPv4 interfaces and to the multicast group
// 239.255.27.14 on each of them, so it is heard on a LAN and on VPNs that
// carry broadcast or multicast (ZeroTier, Hamachi). Instances looking for a
// host listen on that port. For VPNs that carry neither (Tailscale, most
// WireGuard setups) the player types the host's address; a query sent to that
// address, to UDP on the host's adhocctl port, is answered with the same
// announcement, so the typed address can be checked before joining.
//
// Every packet is little-endian and at most 64 bytes, well within any VPN's
// MTU:
//
//   offset size
//   0      4    magic "YKAH"
//   4      1    version, 1
//   5      1    kind: 1 announcement, 2 query (a query ends here)
//   6      2    adhocctl TCP port (the relay is on the next port)
//   8      2    relay TCP port
//   10     1    players logged in to the host's server
//   11     1    reserved, 0
//   12     4    session: a random number the host keeps while it hosts
//   16     10   product code, NUL-padded (ULJM05800)
//   26     32   host's name, UTF-8, NUL-padded
//   58          end
//
// The host's address is where the packet came from.
//
// Threading. One thread of its own sends, receives and looks up interfaces;
// the calls below only exchange state with it under a mutex.
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mhp3rd::adhoc {

inline constexpr std::uint16_t kDiscoveryPort = 27314;

// An IPv4 address of this machine that others may reach it at.
struct LocalAddress {
    std::string address;    // "192.168.1.20"
    std::string interface;  // "en0", "Wi-Fi"
    std::string network;    // "Local network", "Tailscale", "ZeroTier", "Hamachi", "VPN", "Internet"
};

// Up interfaces, loopback left out, VPNs included.
[[nodiscard]] std::vector<LocalAddress> local_addresses();
// This machine's name, for announcing a server without a player; may be empty.
[[nodiscard]] std::string local_host_name();

struct Announcement {
    std::string name;
    std::string product;
    std::uint16_t adhocctl_port{};
    std::uint16_t relay_port{};
    unsigned players{};
    std::uint32_t session{};
};

struct FoundHost {
    Announcement info;
    std::string address;        // where it was heard from
    std::uint64_t heard_ms{};   // since the last announcement or answer
    // "address" or "address:port" when the port is not the usual one: what to
    // type, or use, to join it.
    [[nodiscard]] std::string join_address() const;
};

struct DiscoveryStatus {
    bool announcing{};
    std::string announce_note;   // interfaces announced on, or why answering queries is off
    std::uint64_t announcements_sent{};
    std::uint64_t queries_answered{};
    bool listening{};
    std::string listen_error;
    std::uint64_t announcements_heard{};
    std::size_t hosts{};
};

class Discovery {
public:
    static Discovery &get();
    ~Discovery();
    Discovery(const Discovery &) = delete;
    Discovery &operator=(const Discovery &) = delete;

    // Hosting: announces what `info` returns, asked once a second on the
    // discovery thread, and answers queries on UDP `adhocctl_port`.
    void start_announcing(std::uint16_t adhocctl_port, std::function<Announcement()> info);
    void stop_announcing();

    // Joining: listens for announcements from now on.
    void start_listening();
    // Asks these addresses ("host" or "host:port") directly, every two
    // seconds for the next ten; for VPNs without broadcast.
    void query(const std::vector<std::string> &addresses);
    // Hosts heard in the last few seconds, the one this instance hosts left
    // out.
    [[nodiscard]] std::vector<FoundHost> hosts() const;

    [[nodiscard]] DiscoveryStatus status() const;

    // Stops announcing and listening and ends the discovery thread for good,
    // so the announcement callback is never called again. Called once at exit.
    void shutdown() noexcept;

private:
    Discovery();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mhp3rd::adhoc
