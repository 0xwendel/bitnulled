#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace bitnulled {

inline constexpr std::size_t peer_handshake_size = 68;
inline constexpr std::size_t peer_hash_size = 20;
inline constexpr std::size_t peer_id_size = 20;

using peer_hash = std::array<std::uint8_t, peer_hash_size>;
using peer_id = std::array<std::uint8_t, peer_id_size>;
using peer_reserved = std::array<std::uint8_t, 8>;
using peer_handshake_bytes = std::array<std::uint8_t, peer_handshake_size>;

struct peer_handshake {
    peer_reserved reserved{};
    peer_hash info_hash{};
    peer_id remote_peer_id{};
};

// serializes the fixed 68-byte bittorrent v1 handshake.
peer_handshake_bytes build_peer_handshake(
    const peer_hash& info_hash,
    std::string_view local_peer_id,
    const peer_reserved& reserved = {});

// validates and decodes one complete bittorrent v1 handshake.
peer_handshake parse_peer_handshake(std::span<const std::uint8_t> bytes);

// connects to one numeric peer address, exchanges handshakes and validates info_hash.
// throws std::runtime_error on address, timeout, socket, protocol or swarm mismatch.
peer_handshake perform_peer_handshake(
    std::string_view ip,
    std::uint16_t port,
    const peer_hash& info_hash,
    std::string_view local_peer_id,
    std::chrono::milliseconds timeout = std::chrono::seconds(10));

} // namespace bitnulled
