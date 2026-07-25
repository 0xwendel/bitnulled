#include "bitnulled/torrent.hpp"
#include "bitnulled/sha1.hpp"
#include <fstream>
#include <stdexcept>

namespace bitnulled {

static size_t get_bencode_length(std::string_view sv) {
    std::string_view copy = sv;
    parse_value(copy);
    return sv.size() - copy.size();
}

std::string read_file_to_string(const std::filesystem::path& file_path) {
    // open in binary mode at the end to query file size immediately
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        // if this throws, path is bad or permissions hate you
        throw std::runtime_error("failed to open file: " + file_path.string());
    }

    const auto file_size = file.tellg();
    if (file_size <= 0) {
        return {};
    }

    file.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(static_cast<size_t>(file_size));

    if (!file.read(buffer.data(), file_size)) {
        throw std::runtime_error("failed to read file contents: " + file_path.string());
    }

    return buffer;
}

torrent_file load_torrent_file(const std::filesystem::path& file_path) {
    std::string raw_data = read_file_to_string(file_path);
    if (raw_data.empty()) {
        throw std::runtime_error("empty torrent file: " + file_path.string());
    }

    // calculate info_hash from raw bencoded info dictionary
    size_t info_key_pos = raw_data.find("4:info");
    if (info_key_pos == std::string::npos) {
        throw std::runtime_error("invalid torrent: missing 4:info key");
    }

    size_t info_val_pos = info_key_pos + 6;
    std::string_view info_sv = std::string_view(raw_data).substr(info_val_pos);
    size_t info_len = get_bencode_length(info_sv);
    std::string_view info_bencoded = info_sv.substr(0, info_len);

    SHA1 sha;
    sha.update(std::string(info_bencoded));

    torrent_file torrent;
    torrent.info_hash = sha.final_bytes();

    std::string_view root_sv(raw_data);
    bencode_value root_val = parse_value(root_sv);

    const auto* root_dict = std::get_if<bencode_dict>(&root_val.data);
    if (!root_dict) {
        throw std::runtime_error("invalid torrent: root value is not a bencode dictionary");
    }

    torrent.raw_dict = *root_dict;

    // extract announce
    if (auto it = root_dict->find("announce"); it != root_dict->end()) {
        if (const auto* s = std::get_if<bencode_string>(&it->second.data)) {
            torrent.announce = *s;
        }
    }

    // extract announce-list
    if (auto it = root_dict->find("announce-list"); it != root_dict->end()) {
        if (const auto* l = std::get_if<bencode_list>(&it->second.data)) {
            for (const auto& elem : *l) {
                if (const auto* sub_l = std::get_if<bencode_list>(&elem.data)) {
                    for (const auto& item : *sub_l) {
                        if (const auto* s = std::get_if<bencode_string>(&item.data)) {
                            torrent.announce_list.push_back(*s);
                        }
                    }
                }
            }
        }
    }

    // extract info dictionary fields
    if (auto it = root_dict->find("info"); it != root_dict->end()) {
        if (const auto* info_dict = std::get_if<bencode_dict>(&it->second.data)) {
            if (auto name_it = info_dict->find("name"); name_it != info_dict->end()) {
                if (const auto* s = std::get_if<bencode_string>(&name_it->second.data)) {
                    torrent.name = *s;
                }
            }
            if (auto pl_it = info_dict->find("piece length"); pl_it != info_dict->end()) {
                if (const auto* val = std::get_if<bencode_int>(&pl_it->second.data)) {
                    torrent.piece_length = *val;
                }
            }
            if (auto pcs_it = info_dict->find("pieces"); pcs_it != info_dict->end()) {
                if (const auto* s = std::get_if<bencode_string>(&pcs_it->second.data)) {
                    torrent.pieces = *s;
                }
            }
            if (auto len_it = info_dict->find("length"); len_it != info_dict->end()) {
                // single-file mode
                if (const auto* val = std::get_if<bencode_int>(&len_it->second.data)) {
                    torrent.total_length = *val;
                    torrent.files.push_back({torrent.name, *val});
                }
            } else if (auto files_it = info_dict->find("files"); files_it != info_dict->end()) {
                // multi-file mode
                if (const auto* files_list = std::get_if<bencode_list>(&files_it->second.data)) {
                    for (const auto& file_elem : *files_list) {
                        if (const auto* f_dict = std::get_if<bencode_dict>(&file_elem.data)) {
                            torrent_file_entry entry;
                            if (auto f_len_it = f_dict->find("length"); f_len_it != f_dict->end()) {
                                if (const auto* val = std::get_if<bencode_int>(&f_len_it->second.data)) {
                                    entry.length = *val;
                                    torrent.total_length += *val;
                                }
                            }
                            if (auto f_path_it = f_dict->find("path"); f_path_it != f_dict->end()) {
                                if (const auto* path_list = std::get_if<bencode_list>(&f_path_it->second.data)) {
                                    std::filesystem::path rel_path;
                                    for (const auto& p_elem : *path_list) {
                                        if (const auto* p_str = std::get_if<bencode_string>(&p_elem.data)) {
                                            rel_path /= *p_str;
                                        }
                                    }
                                    entry.path = rel_path;
                                }
                            }
                            torrent.files.push_back(entry);
                        }
                    }
                }
            }
        }
    }

    return torrent;
}

} // namespace bitnulled
