#include "bitnulled/torrent.hpp"
#include "bitnulled/utils.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

int main() {
    const std::filesystem::path dummy_path = "test_sample.torrent";
    {
        std::ofstream out(dummy_path, std::ios::binary);
        out << "d8:announce14:http://tracker4:infod4:name4:test12:piece lengthi16384e6:pieces20:12345678901234567890eee";
    }

    bitnulled::torrent_file tf = bitnulled::load_torrent_file(dummy_path);
    assert(tf.announce == "http://tracker");
    assert(tf.name == "test");
    assert(tf.piece_length == 16384);
    assert(tf.pieces == "12345678901234567890");

    std::filesystem::remove(dummy_path);
    std::cout << "torrent tests passed\n";
    return 0;
}
