#include "bitnulled/torrent.hpp"
#include "bitnulled/utils.hpp"
#include <iostream>
#include <filesystem>
#include <exception>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <caminho_para_arquivo.torrent>\n";
        return 1;
    }

    const std::filesystem::path torrent_path = argv[1];

    if (!std::filesystem::exists(torrent_path)) {
        std::cerr << "Erro: Arquivo nao encontrado: " << torrent_path << "\n";
        return 1;
    }

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