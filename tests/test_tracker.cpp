#include "bitnulled/tracker.hpp"
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static bool throws_runtime_error(const std::function<void()>& fn) {
    try {
        fn();
    } catch (const std::runtime_error&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

// ---------------------------------------------------------------------------
// peer id generation
// ---------------------------------------------------------------------------

static void test_peer_id_length() {
    std::string id = bitnulled::generate_peer_id();
    assert(id.size() == 20);
    std::cout << "[ok] test_peer_id_length\n";
}

static void test_peer_id_prefix() {
    std::string id = bitnulled::generate_peer_id();
    assert(id.compare(0, 8, "-BN0001-") == 0);
    std::cout << "[ok] test_peer_id_prefix\n";
}

static void test_peer_id_suffix_is_alnum() {
    std::string id = bitnulled::generate_peer_id();
    for (size_t i = 8; i < id.size(); ++i) {
        char c = id[i];
        bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        assert(ok);
    }
    std::cout << "[ok] test_peer_id_suffix_is_alnum\n";
}

static void test_peer_id_uniqueness() {
    std::string a = bitnulled::generate_peer_id();
    std::string b = bitnulled::generate_peer_id();
    // statistically impossible to collide with 62^12 possibilities
    assert(a != b);
    std::cout << "[ok] test_peer_id_uniqueness\n";
}

// ---------------------------------------------------------------------------
// compact peer parsing (bep 23)
// ---------------------------------------------------------------------------

static void test_compact_peers_single() {
    // 192.168.1.100:6881 -> c0 a8 01 64 | 1a e1
    const char blob[] = {
        static_cast<char>(0xc0), static_cast<char>(0xa8),
        static_cast<char>(0x01), static_cast<char>(0x64),
        static_cast<char>(0x1a), static_cast<char>(0xe1)
    };

    auto peers = bitnulled::parse_compact_peers(std::string_view(blob, sizeof(blob)));
    assert(peers.size() == 1);
    assert(peers[0].ip == "192.168.1.100");
    assert(peers[0].port == 6881);
    std::cout << "[ok] test_compact_peers_single\n";
}

static void test_compact_peers_multiple() {
    // 10.0.0.1:80 and 255.255.255.255:65535
    const char blob[] = {
        10, 0, 0, 1, 0, 80,
        static_cast<char>(0xff), static_cast<char>(0xff), static_cast<char>(0xff),
        static_cast<char>(0xff), static_cast<char>(0xff), static_cast<char>(0xff)
    };

    auto peers = bitnulled::parse_compact_peers(std::string_view(blob, sizeof(blob)));
    assert(peers.size() == 2);
    assert(peers[0].ip == "10.0.0.1");
    assert(peers[0].port == 80);
    assert(peers[1].ip == "255.255.255.255");
    assert(peers[1].port == 65535);
    std::cout << "[ok] test_compact_peers_multiple\n";
}

static void test_compact_peers_empty() {
    auto peers = bitnulled::parse_compact_peers("");
    assert(peers.empty());
    std::cout << "[ok] test_compact_peers_empty\n";
}

static void test_compact_peers_malformed_size() {
    // 7 bytes is not a multiple of 6, tracker is drunk
    const char blob[] = {1, 2, 3, 4, 5, 6, 7};
    assert(throws_runtime_error([&] {
        bitnulled::parse_compact_peers(std::string_view(blob, sizeof(blob)));
    }));
    std::cout << "[ok] test_compact_peers_malformed_size\n";
}

static void test_compact_peers_port_endianness() {
    // port 0x1234 = 4660 must come out as 4660, not 0x3412
    const char blob[] = {127, 0, 0, 1, 0x12, 0x34};
    auto peers = bitnulled::parse_compact_peers(std::string_view(blob, sizeof(blob)));
    assert(peers.size() == 1);
    assert(peers[0].port == 4660);
    std::cout << "[ok] test_compact_peers_port_endianness\n";
}

// ---------------------------------------------------------------------------
// announce url builder
// ---------------------------------------------------------------------------

static void test_announce_url_structure() {
    std::array<uint8_t, 20> info_hash{};
    info_hash.fill(0xab); // every byte needs escaping

    std::string peer_id = "-BN0001-AAAABBBBCCCC"; // exactly 20 bytes
    std::string url = bitnulled::build_announce_url(
        "http://tracker.example.com/announce", info_hash, peer_id, 6881, 123456);

    assert(url.find("http://tracker.example.com/announce?") == 0);
    assert(url.find("port=6881") != std::string::npos);
    assert(url.find("uploaded=0") != std::string::npos);
    assert(url.find("downloaded=0") != std::string::npos);
    assert(url.find("left=123456") != std::string::npos);
    assert(url.find("compact=1") != std::string::npos);
    assert(url.find("event=started") != std::string::npos);
    std::cout << "[ok] test_announce_url_structure\n";
}

static void test_announce_url_info_hash_encoding() {
    std::array<uint8_t, 20> info_hash{};
    for (size_t i = 0; i < info_hash.size(); ++i) {
        info_hash[i] = static_cast<uint8_t>(i); // 0x00..0x13, all need escaping
    }

    std::string peer_id(20, 'A');
    std::string url = bitnulled::build_announce_url(
        "http://t/announce", info_hash, peer_id, 6881, 0);

    // first byte 0x00 -> %00, second 0x01 -> %01 etc.
    assert(url.find("info_hash=%00%01%02%03") != std::string::npos);
    // 0x0f -> %0F (uppercase hex per existing util)
    assert(url.find("%0F") != std::string::npos);
    std::cout << "[ok] test_announce_url_info_hash_encoding\n";
}

static void test_announce_url_peer_id_passthrough() {
    std::array<uint8_t, 20> info_hash{};
    info_hash.fill(0x00);

    // alnum + '-', all unreserved, must survive without %xx mangling
    std::string peer_id = "-BN0001-0123456789AB";
    std::string url = bitnulled::build_announce_url(
        "http://t/announce", info_hash, peer_id, 6881, 0);

    assert(url.find("peer_id=-BN0001-0123456789AB") != std::string::npos);
    std::cout << "[ok] test_announce_url_peer_id_passthrough\n";
}

static void test_announce_url_bad_peer_id_size() {
    std::array<uint8_t, 20> info_hash{};
    info_hash.fill(0x00);

    assert(throws_runtime_error([&] {
        bitnulled::build_announce_url("http://t/announce", info_hash, "tooshort", 6881, 0);
    }));
    std::cout << "[ok] test_announce_url_bad_peer_id_size\n";
}

static void test_announce_url_existing_query_string() {
    std::array<uint8_t, 20> info_hash{};
    info_hash.fill(0x00);
    std::string peer_id(20, 'B');

    // some trackers hand you a url that already has ?foo=bar in it
    std::string url = bitnulled::build_announce_url(
        "http://t/announce?token=xyz", info_hash, peer_id, 6881, 0);
    assert(url.find("token=xyz&info_hash=") != std::string::npos);
    std::cout << "[ok] test_announce_url_existing_query_string\n";
}

// ---------------------------------------------------------------------------

int main() {
    test_peer_id_length();
    test_peer_id_prefix();
    test_peer_id_suffix_is_alnum();
    test_peer_id_uniqueness();

    test_compact_peers_single();
    test_compact_peers_multiple();
    test_compact_peers_empty();
    test_compact_peers_malformed_size();
    test_compact_peers_port_endianness();

    test_announce_url_structure();
    test_announce_url_info_hash_encoding();
    test_announce_url_peer_id_passthrough();
    test_announce_url_bad_peer_id_size();
    test_announce_url_existing_query_string();

    std::cout << "all tracker tests passed\n";
    return 0;
}
