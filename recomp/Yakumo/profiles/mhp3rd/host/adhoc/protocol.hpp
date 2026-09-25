#pragma once

// Wire formats of the two services a PSP ad hoc server offers. Both run over
// TCP and use little-endian integers with no padding between fields.
//
// adhocctl (TCP 27312): the matchmaking service. A client logs in with its
// virtual MAC, nickname and product code, joins or leaves one group at a
// time, scans for groups, and pings to stay listed. The server tells group
// members when someone joins or leaves.
//
// Relay (TCP 27313): every ad hoc socket is one TCP connection to the server,
// which opens with a 24-byte init record naming the socket (and, for a stream,
// its peer). Datagrams and stream data then travel through the server, so
// neither player needs a reachable address.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace mhp3rd::adhoc {

using Mac = std::array<std::uint8_t, 6>;

inline constexpr std::uint16_t kAdhocctlPort = 27312;
inline constexpr std::uint16_t kRelayPort = 27313;

// A server on another adhocctl port has its relay on the next port up, so
// 27312 goes with 27313 as everywhere else.
[[nodiscard]] constexpr std::uint16_t relay_port_for(std::uint16_t adhocctl_port) {
    return adhocctl_port == 0xFFFFu ? kRelayPort : static_cast<std::uint16_t>(adhocctl_port + 1u);
}

inline constexpr Mac kBroadcastMac{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

[[nodiscard]] std::string format_mac(const Mac &mac);

namespace wire {

inline void put8(std::string &out, std::uint8_t value) { out.push_back(static_cast<char>(value)); }
inline void put16(std::string &out, std::uint16_t value) {
    put8(out, static_cast<std::uint8_t>(value));
    put8(out, static_cast<std::uint8_t>(value >> 8u));
}
inline void put32(std::string &out, std::uint32_t value) {
    put16(out, static_cast<std::uint16_t>(value));
    put16(out, static_cast<std::uint16_t>(value >> 16u));
}
// A fixed-size field: `text` truncated or zero-padded to `size` bytes.
inline void put_fixed(std::string &out, std::string_view text, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) out.push_back(i < text.size() ? text[i] : '\0');
}
inline void put_mac(std::string &out, const Mac &mac) {
    for (std::uint8_t byte : mac) put8(out, byte);
}

inline std::uint16_t get16(const char *data) {
    return static_cast<std::uint16_t>(static_cast<std::uint8_t>(data[0]) |
                                      (static_cast<std::uint8_t>(data[1]) << 8u));
}
inline std::uint32_t get32(const char *data) {
    return static_cast<std::uint32_t>(get16(data)) | (static_cast<std::uint32_t>(get16(data + 2)) << 16u);
}
inline Mac get_mac(const char *data) {
    Mac mac{};
    std::memcpy(mac.data(), data, mac.size());
    return mac;
}
// A NUL-padded fixed-size field as text.
inline std::string get_fixed(const char *data, std::size_t size) {
    std::size_t length = 0;
    while (length < size && data[length] != '\0') ++length;
    return std::string(data, length);
}

} // namespace wire

// adhocctl -----------------------------------------------------------------
namespace ctl {

enum Opcode : std::uint8_t {
    kPing = 0,
    kLogin = 1,
    kConnect = 2,
    kDisconnect = 3,
    kScan = 4,
    kScanComplete = 5,
    kConnectBssid = 6,
    kChat = 7,
};

inline constexpr std::size_t kNicknameLength = 128;
inline constexpr std::size_t kGroupNameLength = 8;
inline constexpr std::size_t kProductCodeLength = 9;
inline constexpr std::size_t kChatLength = 64;

// Client to server. Ping, Disconnect and Scan are the opcode alone.
inline std::string login(const Mac &mac, std::string_view nickname, std::string_view product) {
    std::string out;
    wire::put8(out, kLogin);
    wire::put_mac(out, mac);
    wire::put_fixed(out, nickname.substr(0, kNicknameLength - 1u), kNicknameLength);
    wire::put_fixed(out, product, kProductCodeLength);
    return out;
}
inline std::string connect(std::string_view group) {
    std::string out;
    wire::put8(out, kConnect);
    wire::put_fixed(out, group, kGroupNameLength);
    return out;
}
inline std::string opcode_only(Opcode opcode) { return std::string(1u, static_cast<char>(opcode)); }

// Client to server: sizes including the opcode, 0 for an opcode a client
// never sends.
//   Login:   mac[6], nickname[128], product code[9]
//   Connect: group name[8]
//   Chat:    message[64]
[[nodiscard]] constexpr std::size_t client_packet_size(std::uint8_t opcode) {
    switch (opcode) {
    case kPing: return 1u;
    case kLogin: return 1u + 6u + kNicknameLength + kProductCodeLength;
    case kConnect: return 1u + kGroupNameLength;
    case kDisconnect: return 1u;
    case kScan: return 1u;
    case kChat: return 1u + kChatLength;
    default: return 0u;
    }
}

// Server to client: sizes including the opcode, 0 for an opcode a server
// never sends.
//   Connect:      nickname[128], mac[6], u32 peer id (an address or a number)
//   Disconnect:   u32 peer id
//   Scan:         group name[8], host mac[6]
//   ScanComplete: nothing
//   ConnectBssid: group host mac[6], sent once the join is complete
//   Chat:         message[64], nickname[128]
[[nodiscard]] constexpr std::size_t server_packet_size(std::uint8_t opcode) {
    switch (opcode) {
    case kPing: return 1u;
    case kConnect: return 1u + kNicknameLength + 6u + 4u;
    case kDisconnect: return 1u + 4u;
    case kScan: return 1u + kGroupNameLength + 6u;
    case kScanComplete: return 1u;
    case kConnectBssid: return 1u + 6u;
    case kChat: return 1u + kChatLength + kNicknameLength;
    default: return 0u;
    }
}

// Server to client packets.
inline std::string peer_joined(std::string_view nickname, const Mac &mac, std::uint32_t id) {
    std::string out;
    wire::put8(out, kConnect);
    wire::put_fixed(out, nickname.substr(0, kNicknameLength - 1u), kNicknameLength);
    wire::put_mac(out, mac);
    wire::put32(out, id);
    return out;
}
inline std::string peer_left(std::uint32_t id) {
    std::string out;
    wire::put8(out, kDisconnect);
    wire::put32(out, id);
    return out;
}
inline std::string scan_result(std::string_view group, const Mac &host) {
    std::string out;
    wire::put8(out, kScan);
    wire::put_fixed(out, group, kGroupNameLength);
    wire::put_mac(out, host);
    return out;
}
inline std::string joined(const Mac &host) {
    std::string out;
    wire::put8(out, kConnectBssid);
    wire::put_mac(out, host);
    return out;
}
inline std::string chat_message(std::string_view message, std::string_view nickname) {
    std::string out;
    wire::put8(out, kChat);
    wire::put_fixed(out, message.substr(0, kChatLength - 1u), kChatLength);
    wire::put_fixed(out, nickname.substr(0, kNicknameLength - 1u), kNicknameLength);
    return out;
}

} // namespace ctl

// Relay ----------------------------------------------------------------------
namespace relay {

enum InitType : std::uint32_t {
    kInitPdp = 0,
    kInitPtpListen = 1,
    kInitPtpConnect = 2,
    kInitPtpAccept = 3,
};

// Largest datagram and stream block the relay forwards.
inline constexpr std::size_t kPdpBlockMax = 10u * 1024u;
inline constexpr std::size_t kPtpBlockMax = 50u * 1024u;

// Address fields are 8 bytes wide with the MAC in the first 6.
inline void put_address(std::string &out, const Mac &mac) {
    wire::put_mac(out, mac);
    wire::put16(out, 0u);
}

// i32 type, source address[8], u16 source port, destination address[8], u16 destination port.
inline constexpr std::size_t kInitSize = 24u;
inline std::string init(InitType type, const Mac &source, std::uint16_t source_port, const Mac &destination,
                        std::uint16_t destination_port) {
    std::string out;
    wire::put32(out, type);
    put_address(out, source);
    wire::put16(out, source_port);
    put_address(out, destination);
    wire::put16(out, destination_port);
    return out;
}

// A datagram: address[8], u16 port, u32 size, then the data. Towards the
// server the address and port name the destination; from it, the sender.
inline constexpr std::size_t kPdpHeaderSize = 14u;
inline std::string pdp_header(const Mac &mac, std::uint16_t port, std::uint32_t size) {
    std::string out;
    put_address(out, mac);
    wire::put16(out, port);
    wire::put32(out, size);
    return out;
}

// Stream notice: address[8], u16 port. A listening socket receives one per
// incoming connection; a connecting or accepted socket receives one when the
// stream is established.
inline constexpr std::size_t kPtpNoticeSize = 10u;

inline std::string ptp_notice(const Mac &mac, std::uint16_t port) {
    std::string out;
    put_address(out, mac);
    wire::put16(out, port);
    return out;
}

// Stream data: u32 size, then the data.
inline constexpr std::size_t kPtpHeaderSize = 4u;

} // namespace relay

} // namespace mhp3rd::adhoc
