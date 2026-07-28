#pragma once
#include "bitnulled/torrent.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bitnulled {

struct peer_info {
    std::string ip;
    std::uint16_t port{0};
};

struct tracker_response {
    std::int64_t interval{0};
    std::vector<peer_info> peers;
};

// generates a fresh 20-byte peer id: "-BN0001-" prefix + 12 random alnum chars.
std::string generate_peer_id();

// decodes BEP 0023 compact peer blob (6 bytes per peer: 4 ipv4 + 2 port big-endian).
// throws std::runtime_error if the blob size is not a multiple of 6.
std::vector<peer_info> parse_compact_peers(std::string_view blob);

// builds the full announce url with percent-encoded info_hash and peer_id.
std::string build_announce_url(
    std::string_view announce_url,
    const std::array<std::uint8_t, 20>& info_hash,
    std::string_view peer_id,
    std::uint16_t port,
    std::int64_t left);

// contacts the tracker over http and parses the bencoded response.
// throws std::runtime_error on network failure, non-200 status or tracker error.
tracker_response announce(const torrent_file& torrent, std::string_view peer_id, std::uint16_t port);

} // namespace bitnulled
