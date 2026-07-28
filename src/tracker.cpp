#include "bitnulled/tracker.hpp"
#include "bitnulled/bencode.hpp"
#include "bitnulled/http_client.hpp"
#include "bitnulled/utils.hpp"
#include <random>
#include <stdexcept>
#include <string>

namespace bitnulled {

std::string generate_peer_id() {
    static constexpr char prefix[] = "-BN0001-";
    static constexpr char alnum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    // mersenne is overkill for a peer id but love is overkill too, whatever
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<size_t> dist(0, sizeof(alnum) - 2);

    std::string peer_id;
    peer_id.reserve(20);
    peer_id.append(prefix);

    while (peer_id.size() < 20) {
        peer_id.push_back(alnum[dist(rng)]);
    }
    return peer_id;
}

std::vector<peer_info> parse_compact_peers(std::string_view blob) {
    if (blob.size() % 6 != 0) {
        throw std::runtime_error("compact peer buffer size is not a multiple of 6");
    }

    std::vector<peer_info> peers;
    peers.reserve(blob.size() / 6);

    for (size_t i = 0; i < blob.size(); i += 6) {
        const auto* raw = reinterpret_cast<const uint8_t*>(blob.data() + i);

        peer_info peer;
        peer.ip = std::to_string(raw[0]) + "." + std::to_string(raw[1]) + "." +
                  std::to_string(raw[2]) + "." + std::to_string(raw[3]);

        // port arrives big-endian because network people hate little-endian
        peer.port = static_cast<uint16_t>((static_cast<uint16_t>(raw[4]) << 8) |
                                          static_cast<uint16_t>(raw[5]));

        peers.push_back(std::move(peer));
    }
    return peers;
}

std::string build_announce_url(
    std::string_view announce_url,
    const std::array<uint8_t, 20>& info_hash,
    std::string_view peer_id,
    uint16_t port,
    std::int64_t left) {
    if (peer_id.size() != 20) {
        throw std::runtime_error("peer id must be exactly 20 bytes");
    }

    // url_encode only touches binary bytes; alphanumerics pass through raw.
    // that's fine for info_hash but the peer_id prefix starts with '-', which
    // is an unreserved char anyway, so it survives intact.
    std::string info_hash_encoded = url_encode(info_hash);
    std::string peer_id_encoded = url_encode(
        std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(peer_id.data()), peer_id.size()));

    std::string url;
    url.reserve(announce_url.size() + 256);
    url += announce_url;
    url += (announce_url.find('?') == std::string_view::npos) ? '?' : '&';
    url += "info_hash=" + info_hash_encoded;
    url += "&peer_id=" + peer_id_encoded;
    url += "&port=" + std::to_string(port);
    url += "&uploaded=0";
    url += "&downloaded=0";
    url += "&left=" + std::to_string(left);
    url += "&compact=1";
    url += "&event=started";
    return url;
}

tracker_response announce(const torrent_file& torrent, std::string_view peer_id, uint16_t port) {
    if (torrent.announce.empty()) {
        throw std::runtime_error("torrent has no announce url");
    }

    std::string url = build_announce_url(
        torrent.announce, torrent.info_hash, peer_id, port, torrent.total_length);

    http_response http_resp = http_get(url);

    if (http_resp.status_code != 200) {
        throw std::runtime_error(
            "tracker returned http status " + std::to_string(http_resp.status_code));
    }

    std::string_view body_view(http_resp.body);
    bencode_dict resp_dict = parse_dict(body_view);

    // tracker politely telling us to go away
    if (auto it = resp_dict.find("failure reason"); it != resp_dict.end()) {
        if (std::holds_alternative<bencode_string>(it->second.data)) {
            throw std::runtime_error(
                "tracker failure: " + std::get<bencode_string>(it->second.data));
        }
        throw std::runtime_error("tracker failure: unknown reason");
    }

    tracker_response response;

    if (auto it = resp_dict.find("interval"); it != resp_dict.end()) {
        if (!std::holds_alternative<bencode_int>(it->second.data)) {
            throw std::runtime_error("tracker response: 'interval' is not an int");
        }
        response.interval = std::get<bencode_int>(it->second.data);
    }

    if (auto it = resp_dict.find("peers"); it != resp_dict.end()) {
        if (std::holds_alternative<bencode_string>(it->second.data)) {
            // compact mode (bep 23), the only mode we asked for anyway
            response.peers = parse_compact_peers(std::get<bencode_string>(it->second.data));
        } else if (std::holds_alternative<bencode_list>(it->second.data)) {
            // tracker ignored compact=1 and sent a dict list. parse it anyway,
            // some trackers just love being difficult.
            for (const auto& entry : std::get<bencode_list>(it->second.data)) {
                if (!std::holds_alternative<bencode_dict>(entry.data)) {
                    continue;
                }
                const auto& peer_dict = std::get<bencode_dict>(entry.data);
                auto ip_it = peer_dict.find("ip");
                auto port_it = peer_dict.find("port");
                if (ip_it == peer_dict.end() || port_it == peer_dict.end()) {
                    continue;
                }
                if (!std::holds_alternative<bencode_string>(ip_it->second.data) ||
                    !std::holds_alternative<bencode_int>(port_it->second.data)) {
                    continue;
                }
                peer_info peer;
                peer.ip = std::get<bencode_string>(ip_it->second.data);
                peer.port = static_cast<uint16_t>(std::get<bencode_int>(port_it->second.data));
                response.peers.push_back(std::move(peer));
            }
        } else {
            throw std::runtime_error("tracker response: 'peers' has unexpected type");
        }
    }

    return response;
}

} // namespace bitnulled
