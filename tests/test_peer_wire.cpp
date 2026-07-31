#include "bitnulled/peer_wire.hpp"
#include <array>
#include <asio.hpp>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using namespace std::chrono_literals;

bool throws_runtime_error(const std::function<void()>& function) {
    try {
        function();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

bitnulled::peer_hash make_hash(std::uint8_t first) {
    bitnulled::peer_hash hash{};
    for (std::size_t index = 0; index < hash.size(); ++index) {
        hash[index] = static_cast<std::uint8_t>(first + index);
    }
    return hash;
}

void test_build_and_parse() {
    const bitnulled::peer_hash hash = make_hash(0x10);
    bitnulled::peer_reserved reserved{};
    reserved[5] = 0x10;
    const std::string peer_id = "-BN0001-ABCDEFGHIJKL";

    const auto bytes = bitnulled::build_peer_handshake(hash, peer_id, reserved);
    assert(bytes.size() == 68);
    assert(bytes[0] == 19);
    assert(std::string(bytes.begin() + 1, bytes.begin() + 20) == "BitTorrent protocol");

    const auto parsed = bitnulled::parse_peer_handshake(bytes);
    assert(parsed.info_hash == hash);
    assert(parsed.reserved == reserved);
    assert(std::string(parsed.remote_peer_id.begin(), parsed.remote_peer_id.end()) == peer_id);
    std::cout << "[ok] test_build_and_parse\n";
}

void test_invalid_inputs() {
    const bitnulled::peer_hash hash{};
    assert(throws_runtime_error([&] { bitnulled::build_peer_handshake(hash, "short"); }));

    std::array<std::uint8_t, 67> short_packet{};
    assert(throws_runtime_error([&] { bitnulled::parse_peer_handshake(short_packet); }));

    auto bad_protocol = bitnulled::build_peer_handshake(hash, std::string(20, 'A'));
    bad_protocol[1] = 'X';
    assert(throws_runtime_error([&] { bitnulled::parse_peer_handshake(bad_protocol); }));
    std::cout << "[ok] test_invalid_inputs\n";
}

void test_tcp_exchange() {
    const bitnulled::peer_hash hash = make_hash(0x20);
    const std::string local_peer_id = "-BN0001-LOCALPEER001";
    const std::string remote_peer_id = "-UT0001-REMOTEPEER01";

    asio::io_context server_context;
    asio::ip::tcp::acceptor acceptor(
        server_context, asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 0));
    const std::uint16_t port = acceptor.local_endpoint().port();

    std::jthread server([&] {
        asio::ip::tcp::socket socket(server_context);
        acceptor.accept(socket);

        bitnulled::peer_handshake_bytes request{};
        asio::read(socket, asio::buffer(request));
        const auto parsed = bitnulled::parse_peer_handshake(request);
        assert(parsed.info_hash == hash);

        const auto response = bitnulled::build_peer_handshake(hash, remote_peer_id);
        asio::write(socket, asio::buffer(response));
    });

    const auto result = bitnulled::perform_peer_handshake(
        "127.0.0.1", port, hash, local_peer_id, 2s);
    assert(result.info_hash == hash);
    assert(std::string(result.remote_peer_id.begin(), result.remote_peer_id.end()) == remote_peer_id);
    std::cout << "[ok] test_tcp_exchange\n";
}

void test_swarm_mismatch() {
    const bitnulled::peer_hash expected_hash = make_hash(0x30);
    const bitnulled::peer_hash wrong_hash = make_hash(0x40);
    const std::string local_peer_id = "-BN0001-LOCALPEER001";

    asio::io_context server_context;
    asio::ip::tcp::acceptor acceptor(
        server_context, asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), 0));
    const std::uint16_t port = acceptor.local_endpoint().port();

    std::jthread server([&] {
        asio::ip::tcp::socket socket(server_context);
        acceptor.accept(socket);
        bitnulled::peer_handshake_bytes request{};
        asio::read(socket, asio::buffer(request));
        const auto response = bitnulled::build_peer_handshake(wrong_hash, std::string(20, 'R'));
        asio::write(socket, asio::buffer(response));
    });

    assert(throws_runtime_error([&] {
        bitnulled::perform_peer_handshake(
            "127.0.0.1", port, expected_hash, local_peer_id, 2s);
    }));
    std::cout << "[ok] test_swarm_mismatch\n";
}

} // namespace

int main() {
    test_build_and_parse();
    test_invalid_inputs();
    test_tcp_exchange();
    test_swarm_mismatch();
    std::cout << "all peer wire tests passed\n";
    return 0;
}
