#pragma once
#include "bitnulled/bencode.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace bitnulled {

struct torrent_file_entry {
    std::filesystem::path path;
    std::int64_t length{0};
};

struct torrent_file {
    std::string announce;
    std::vector<std::string> announce_list;
    std::string name;
    std::int64_t piece_length{0};
    std::string pieces;
    std::int64_t total_length{0};
    std::vector<torrent_file_entry> files;
    std::array<uint8_t, 20> info_hash{};
    bencode_dict raw_dict;
};

// reads raw binary file content into memory string
std::string read_file_to_string(const std::filesystem::path& file_path);

// parses a .torrent file from disk into torrent_file struct
torrent_file load_torrent_file(const std::filesystem::path& file_path);

} // namespace bitnulled
