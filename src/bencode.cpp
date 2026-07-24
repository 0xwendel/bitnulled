#include "bencode.hpp"
#include <string_view>
#include <string>
#include <stdexcept>

bencode_string parse_string(std::string_view& in) {
    // find :
    size_t colon = in.find(":");
    if (colon == std::string_view::npos) {
        throw std::runtime_error("sem ':' na string.");
    }

    // length string
    size_t len = std::stoull(std::string(in.substr(0,colon)));

    in.remove_prefix(colon + 1);

    if (in.size() < len){
        throw std::runtime_error("buffer menor que o tamanho declarado da string!");
    }

    bencode_string res(in.substr(0,len));

    // consome os bytes da str view
    in.remove_prefix(len);


    return res;
}

