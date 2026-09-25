#pragma once

// The few socket calls the ad hoc client, server and discovery share, over
// BSD sockets and Winsock alike. Every socket they make is non-blocking and
// served from poll() on a network thread of its own.
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mhp3rd::adhoc::net {

#if defined(_WIN32)
using Socket = SOCKET;
inline constexpr Socket kNoSocket = INVALID_SOCKET;
using PollEntry = WSAPOLLFD;
inline void close_socket(Socket s) { closesocket(s); }
inline int socket_error() { return WSAGetLastError(); }
inline bool would_block(int error) { return error == WSAEWOULDBLOCK; }
inline bool connect_pending(int error) { return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS; }
inline int poll_sockets(PollEntry *entries, std::size_t count, int timeout_ms) {
    if (count == 0u) {
        Sleep(static_cast<DWORD>(timeout_ms));
        return 0;
    }
    return WSAPoll(entries, static_cast<ULONG>(count), timeout_ms);
}
inline bool set_nonblocking(Socket s) {
    u_long enabled = 1;
    return ioctlsocket(s, FIONBIO, &enabled) == 0;
}
// Winsock stays started while one of these lives.
struct WinsockSession {
    WinsockSession() {
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
    }
    ~WinsockSession() { WSACleanup(); }
    WinsockSession(const WinsockSession &) = delete;
    WinsockSession &operator=(const WinsockSession &) = delete;
};
#else
using Socket = int;
inline constexpr Socket kNoSocket = -1;
using PollEntry = pollfd;
inline void close_socket(Socket s) { ::close(s); }
inline int socket_error() { return errno; }
inline bool would_block(int error) { return error == EWOULDBLOCK || error == EAGAIN || error == EINTR; }
inline bool connect_pending(int error) { return error == EINPROGRESS || error == EINTR; }
inline int poll_sockets(PollEntry *entries, std::size_t count, int timeout_ms) {
    return ::poll(entries, static_cast<nfds_t>(count), timeout_ms);
}
inline bool set_nonblocking(Socket s) {
    const int flags = fcntl(s, F_GETFL, 0);
    return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
}
struct WinsockSession {};
#endif

#if defined(MSG_NOSIGNAL)
inline constexpr int kSendFlags = MSG_NOSIGNAL;
#else
inline constexpr int kSendFlags = 0;
#endif

inline void set_option(Socket s, int level, int name, int value) {
    setsockopt(s, level, name, reinterpret_cast<const char *>(&value), sizeof(value));
}

// No SIGPIPE on a write to a closed connection, where the platform needs it
// per socket.
inline void no_sigpipe(Socket s) {
#if defined(SO_NOSIGPIPE)
    set_option(s, SOL_SOCKET, SO_NOSIGPIPE, 1);
#else
    (void)s;
#endif
}

// An IPv4 or IPv6 socket address.
struct Address {
    sockaddr_storage storage{};
    socklen_t length{};

    [[nodiscard]] int family() const { return storage.ss_family; }

    [[nodiscard]] std::uint16_t port() const {
        if (storage.ss_family == AF_INET) return ntohs(reinterpret_cast<const sockaddr_in *>(&storage)->sin_port);
        if (storage.ss_family == AF_INET6) return ntohs(reinterpret_cast<const sockaddr_in6 *>(&storage)->sin6_port);
        return 0u;
    }

    [[nodiscard]] Address with_port(std::uint16_t port) const {
        Address copy = *this;
        if (copy.storage.ss_family == AF_INET)
            reinterpret_cast<sockaddr_in *>(&copy.storage)->sin_port = htons(port);
        else if (copy.storage.ss_family == AF_INET6)
            reinterpret_cast<sockaddr_in6 *>(&copy.storage)->sin6_port = htons(port);
        return copy;
    }

    // The IPv4 address in network byte order, also for an IPv4-mapped IPv6
    // address; 0 for anything else.
    [[nodiscard]] std::uint32_t ipv4() const {
        if (storage.ss_family == AF_INET) return reinterpret_cast<const sockaddr_in *>(&storage)->sin_addr.s_addr;
        if (storage.ss_family == AF_INET6) {
            const auto *bytes = reinterpret_cast<const sockaddr_in6 *>(&storage)->sin6_addr.s6_addr;
            static const unsigned char kMapped[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF};
            if (std::memcmp(bytes, kMapped, sizeof(kMapped)) == 0) {
                std::uint32_t value = 0;
                std::memcpy(&value, bytes + 12, 4);
                return value;
            }
        }
        return 0u;
    }

    // The host part alone: "192.168.1.5", "fd00::1".
    [[nodiscard]] std::string host() const {
        if (const std::uint32_t v4 = ipv4(); v4 != 0u || storage.ss_family == AF_INET) {
            const auto *b = reinterpret_cast<const unsigned char *>(&v4);
            return std::to_string(b[0]) + "." + std::to_string(b[1]) + "." + std::to_string(b[2]) + "." +
                   std::to_string(b[3]);
        }
        char text[NI_MAXHOST] = {};
        if (getnameinfo(reinterpret_cast<const sockaddr *>(&storage), length, text, sizeof(text), nullptr, 0,
                        NI_NUMERICHOST) != 0)
            return "?";
        return text;
    }

    // "192.168.1.5:27312" or "[fd00::1]:27312".
    [[nodiscard]] std::string describe() const {
        const std::string name = host();
        const bool v6 = storage.ss_family == AF_INET6 && ipv4() == 0u;
        return (v6 ? "[" + name + "]" : name) + ":" + std::to_string(port());
    }

    static Address ipv4_address(std::uint32_t network_order, std::uint16_t port) {
        Address address;
        auto *in = reinterpret_cast<sockaddr_in *>(&address.storage);
        in->sin_family = AF_INET;
        in->sin_addr.s_addr = network_order;
        in->sin_port = htons(port);
        address.length = sizeof(sockaddr_in);
        return address;
    }
};

// True for 127.0.0.0/8 (network byte order).
inline bool is_loopback_ipv4(std::uint32_t network_order) {
    return (ntohl(network_order) >> 24u) == 127u;
}

} // namespace mhp3rd::adhoc::net
