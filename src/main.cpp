#include "bitnulled/torrent.hpp"
#include "bitnulled/tracker.hpp"
#include "bitnulled/peer_wire.hpp"
#include "bitnulled/utils.hpp"
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "uso: " << argv[0] << " <arquivo.torrent>\n";
        return 1;
    }

    const std::filesystem::path torrent_path = argv[1];

    if (!std::filesystem::exists(torrent_path)) {
        std::cerr << "erro: arquivo nao encontrado: " << torrent_path << "\n";
        return 1;
    }

    try {
        bitnulled::torrent_file torrent = bitnulled::load_torrent_file(torrent_path);

        std::cout << "=== torrent metadata ===\n";
        std::cout << "name:           " << torrent.name << "\n";
        std::cout << "announce:       " << torrent.announce << "\n";
        std::cout << "piece length:   " << torrent.piece_length << " bytes ("
                  << (torrent.piece_length / 1024) << " kib)\n";
        std::cout << "total length:   " << torrent.total_length << " bytes ("
                  << (torrent.total_length / (1024.0 * 1024.0)) << " mib)\n";
        std::cout << "files count:    " << torrent.files.size() << "\n";
        std::cout << "infohash (hex): " << bitnulled::to_hex(torrent.info_hash) << "\n";

        if (!torrent.files.empty()) {
            std::cout << "\nfiles list:\n";
            for (const auto& file : torrent.files) {
                std::cout << " - " << file.path.string() << " (" << file.length << " bytes)\n";
            }
        }

        // fase 3: talk to the tracker, beg for peers
        std::cout << "\n=== tracker announce ===\n";

        const std::string peer_id = bitnulled::generate_peer_id();
        std::cout << "peer id: " << peer_id << "\n";

        constexpr std::uint16_t listen_port = 6881;
        bitnulled::tracker_response response = bitnulled::announce(torrent, peer_id, listen_port);

        std::cout << "interval: " << response.interval << "s\n";
        std::cout << "peers:    " << response.peers.size() << "\n";
        for (const auto& peer : response.peers) {
            std::cout << "  " << peer.ip << ":" << peer.port << "\n";
        }

        std::cout << "\n=== peer wire handshake ===\n";
        bool handshake_complete = false;
        for (const auto& peer : response.peers) {
            try {
                const bitnulled::peer_handshake handshake = bitnulled::perform_peer_handshake(
                    peer.ip, peer.port, torrent.info_hash, peer_id);
                const std::string remote_peer_id(
                    handshake.remote_peer_id.begin(), handshake.remote_peer_id.end());
                std::cout << "connected: " << peer.ip << ":" << peer.port << "\n";
                std::cout << "remote peer id: " << remote_peer_id << "\n";
                handshake_complete = true;
                break;
            } catch (const std::exception& e) {
                std::cerr << "handshake failed for " << peer.ip << ":" << peer.port
                          << ": " << e.what() << "\n";
            }
        }

        if (!handshake_complete) {
            throw std::runtime_error("no tracker peer completed the peer wire handshake");
        }

    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
