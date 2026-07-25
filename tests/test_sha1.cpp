#include "bitnulled/sha1.hpp"
#include <cassert>
#include <iostream>

int main() {
    SHA1 checksum;
    checksum.update("abc");
    std::string hash = checksum.final();

    assert(hash == "a9993e364706816aba3e25717850c26c9cd0d89d");

    SHA1 checksum_bytes;
    checksum_bytes.update("abc");
    auto raw_bytes = checksum_bytes.final_bytes();
    assert(raw_bytes.size() == 20);
    assert(raw_bytes[0] == 0xa9);
    assert(raw_bytes[1] == 0x99);

    std::cout << "sha1 tests passed\n";
    return 0;
}
