#pragma once
#include <string_view>
#include <string>

#include <variant>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

struct bencode_value;

using bencode_int = std::int64_t;
using bencode_string = std::string;
using bencode_list = std::vector<bencode_value>;
using bencode_dict = std::map<std::string, bencode_value>;


struct bencode_value {
  std::variant<bencode_int, bencode_string, bencode_list, bencode_dict> data;  
};

bencode_string parse_string(std::string_view& in);