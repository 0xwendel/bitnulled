#include "bitnulled/torrent.hpp"
#include "bitnulled/utils.hpp"
#include <iostream>
#include <filesystem>
#include <exception>

int main() {
    const std::filesystem::path torrent_path = R"(E:\vault\Backup\Download\kali-nethunter-2025.3-generic-arm64-minimal.zip.torrent)";
    
    try {
        bitnulled::torrent_file torrent = bitnulled::load_torrent_file(torrent_path);
        
        std::cout << "=== Torrent Metadata ===\n";
        std::cout << "Name:           " << torrent.name << "\n";
        std::cout << "Announce:       " << torrent.announce << "\n";
        std::cout << "Piece Length:   " << torrent.piece_length << " bytes\n";
        std::cout << "Total Length:   " << torrent.total_length << " bytes\n";
        std::cout << "Infohash (Hex): " << bitnulled::to_hex(torrent.info_hash) << "\n";
        std::cout << "Infohash (URL): " << bitnulled::url_encode(torrent.info_hash) << "\n";

    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}