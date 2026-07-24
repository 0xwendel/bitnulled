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

    if (c == 'l') {
        return bencode_value{parse_list(in)};
    }

    if (c == 'd') {
        return bencode_value{parse_dict(in)};
    }


    throw std::runtime_error("byte invalido encontrado no bencode");
}

bencode_list parse_list(std::string_view& in) {
    if (in.empty() || in.front() != 'l') {
        throw std::runtime_error("erro: parse_list esperado 'l'");
    }

    in.remove_prefix(1); // consome 'l'

    bencode_list list; // std::vector<bencode_value>

    while (!in.empty() && in.front() != 'e') {
        list.push_back(parse_value(in)); // recursão
    }

    if (in.empty()) {
        throw std::runtime_error("lista nao fechada: sem 'e' no final");
    }

    in.remove_prefix(1);

    return list;
}

bencode_dict parse_dict(std::string_view& in) {
    if (in.empty() || in.front() != 'd') {
        throw std::runtime_error("erro: parse_dict esperado: 'd'");
    }

    in.remove_prefix(1); // 'd'

    bencode_dict dict;

    while(!in.empty() && in.front() != 'e') {
        if (!std::isdigit(in.front())) {
            throw std::runtime_error("a chave DEVE ser uma string");
        }

        bencode_string key = parse_string(in);

        bencode_value val = parse_value(in);
        
        dict[key] = val;
    }

    if (in.empty()) {
        throw std::runtime_error("dict nao fechado: sem 'e' no final");
    }

    in.remove_prefix(1);

    return dict;
}