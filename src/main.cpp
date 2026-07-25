#include "bencode.hpp"
#include <iostream>
#include <filesystem>
#include <exception>

int main() {
    const std::filesystem::path torrent_path = R"(E:\vault\Backup\Download\kali-nethunter-2025.3-generic-arm64-minimal.zip.torrent)";
    
    try {
        std::string raw_data = read_torrent_file(torrent_path);
        std::cout << "file loaded into memory: " << raw_data.size() << " bytes\n";

        std::string_view sv(raw_data);
        bencode_value parsed = parse_value(sv);

        if (const auto* dict = std::get_if<bencode_dict>(&parsed.data)) {
            std::cout << "torrent dictionary keys:\n";
            for (const auto& [key, val] : *dict) {
                std::cout << " - " << key << " (type index: " << val.data.index() << ")\n";
                if (key == "announce" && std::holds_alternative<bencode_string>(val.data)) {
                    std::cout << "   announce url: " << std::get<bencode_string>(val.data) << "\n";
                }
            }
        } else {
            std::cout << "root bencode value is not a dictionary\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error reading torrent: " << e.what() << "\n";
        return 1;
    }

    return 0;
}