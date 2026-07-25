#include "bitnulled/bencode.hpp"
#include <cassert>
#include <iostream>

int main() {
    // test string parsing
    std::string_view sv = "4:spam";
    auto str = bitnulled::parse_string(sv);
    assert(str == "spam");

    // test int parsing
    std::string_view iv = "i42e";
    auto val = bitnulled::parse_int(iv);
    assert(val == 42);

    std::cout << "bencode tests passed\n";
    return 0;
}
