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
        std::cout << "Piece Length:   " << torrent.piece_length << " bytes (" << (torrent.piece_length / 1024) << " KiB)\n";
        std::cout << "Total Length:   " << torrent.total_length << " bytes (" << (torrent.total_length / (1024.0 * 1024.0)) << " MiB)\n";
        std::cout << "Files Count:    " << torrent.files.size() << "\n";
        std::cout << "Infohash (Hex): " << bitnulled::to_hex(torrent.info_hash) << "\n";
        std::cout << "Infohash (URL): " << bitnulled::url_encode(torrent.info_hash) << "\n";

        if (!torrent.files.empty()) {
            std::cout << "\nFiles List:\n";
            for (const auto& file : torrent.files) {
                std::cout << " - " << file.path.string() << " (" << file.length << " bytes)\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}