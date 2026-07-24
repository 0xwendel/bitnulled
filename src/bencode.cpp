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

bencode_int parse_int(std::string_view& in) {

    if (in.empty() || in.front() != 'i'){
        throw std::runtime_error("erro: parse_int chamado sem o 'i' inicial");
    }

    in.remove_prefix(1);

    // find "e"
    size_t e_pos = in.find('e');
    if(e_pos == std::string::npos) {
        throw std::runtime_error("int malformatado!");
    }

    bencode_int val = std::stoll(std::string(in.substr(0,e_pos)));

    in.remove_prefix(e_pos + 1);

    return val;
    
}

bencode_value parse_value(std::string_view& in){
    if (in.empty()) throw std::runtime_error("buffer vazio!");

    char c = in.front();

    if (std::isdigit(c)) {
        return bencode_value{parse_string(in) };
    }
    if (c == 'i') {
        return bencode_value{parse_int(in) };
    }


    throw std::runtime_error("byte invalido encontrado no bencode");
}
