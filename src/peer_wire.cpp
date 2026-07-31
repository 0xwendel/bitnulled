#include "bitnulled/peer_wire.hpp"
#include <algorithm>
#include <asio.hpp>
#include <stdexcept>
#include <string>

namespace bitnulled {

namespace {

constexpr std::uint8_t protocol_length = 19;
constexpr std::string_view protocol_name = "BitTorrent protocol";
constexpr std::size_t reserved_offset = 20;
constexpr std::size_t info_hash_offset = 28;
constexpr std::size_t peer_id_offset = 48;

static_assert(1 + protocol_name.size() + 8 + peer_hash_size + peer_id_size == peer_handshake_size);

} // namespace

peer_handshake_bytes build_peer_handshake(
    const peer_hash& info_hash,
    std::string_view local_peer_id,
    const peer_reserved& reserved) {
    if (local_peer_id.size() != peer_id_size) {
        throw std::runtime_error("peer id must be exactly 20 bytes");
    }

    peer_handshake_bytes bytes{};
    bytes[0] = protocol_length;
    std::copy(protocol_name.begin(), protocol_name.end(), bytes.begin() + 1);
    std::copy(reserved.begin(), reserved.end(), bytes.begin() + reserved_offset);
    std::copy(info_hash.begin(), info_hash.end(), bytes.begin() + info_hash_offset);
    std::copy(local_peer_id.begin(), local_peer_id.end(), bytes.begin() + peer_id_offset);
    return bytes;
}

peer_handshake parse_peer_handshake(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != peer_handshake_size) {
        throw std::runtime_error("peer handshake must be exactly 68 bytes");
    }
    if (bytes[0] != protocol_length) {
        throw std::runtime_error("invalid peer handshake protocol length");
    }

    const auto protocol_begin = bytes.begin() + 1;
    if (!std::equal(protocol_name.begin(), protocol_name.end(), protocol_begin)) {
        throw std::runtime_error("invalid peer handshake protocol name");
    }

    peer_handshake handshake;
    std::copy_n(bytes.begin() + reserved_offset, handshake.reserved.size(), handshake.reserved.begin());
    std::copy_n(bytes.begin() + info_hash_offset, handshake.info_hash.size(), handshake.info_hash.begin());
    std::copy_n(bytes.begin() + peer_id_offset, handshake.remote_peer_id.size(), handshake.remote_peer_id.begin());
    return handshake;
}

peer_handshake perform_peer_handshake(
    std::string_view ip,
    std::uint16_t port,
    const peer_hash& info_hash,
    std::string_view local_peer_id,
    std::chrono::milliseconds timeout) {
    if (ip.empty()) {
        throw std::runtime_error("peer address is empty");
    }
    if (port == 0) {
        throw std::runtime_error("peer port must not be zero");
    }
    if (timeout <= std::chrono::milliseconds::zero()) {
        throw std::runtime_error("peer handshake timeout must be positive");
    }

    const peer_handshake_bytes request = build_peer_handshake(info_hash, local_peer_id);
    peer_handshake_bytes response{};

    asio::error_code address_error;
    const asio::ip::address address = asio::ip::make_address(std::string(ip), address_error);
    if (address_error) {
        throw std::runtime_error("invalid peer address " + std::string(ip) + ": " + address_error.message());
    }

    asio::io_context io_context;
    asio::ip::tcp::socket socket(io_context);
    asio::steady_timer timer(io_context);
    asio::error_code operation_error;
    bool completed = false;
    bool timed_out = false;

    timer.expires_after(timeout);
    timer.async_wait([&](const asio::error_code& ec) {
        if (ec || completed) {
            return;
        }

        timed_out = true;
        asio::error_code ignored;
        socket.cancel(ignored);
        socket.close(ignored);
    });

    const auto finish = [&](const asio::error_code& ec) {
        operation_error = ec;
        completed = true;
        try {
            (void)timer.cancel();
        } catch (const asio::system_error& timer_error) {
            if (!operation_error) {
                operation_error = timer_error.code();
            }
        }
    };

    socket.async_connect(asio::ip::tcp::endpoint(address, port),
        [&](const asio::error_code& connect_error) {
            if (connect_error) {
                finish(connect_error);
                return;
            }

            asio::async_write(socket, asio::buffer(request),
                [&](const asio::error_code& write_error, std::size_t) {
                    if (write_error) {
                        finish(write_error);
                        return;
                    }

                    asio::async_read(socket, asio::buffer(response),
                        [&](const asio::error_code& read_error, std::size_t) {
                            finish(read_error);
                        });
                });
        });

    io_context.run();

    if (timed_out) {
        throw std::runtime_error("peer handshake timed out");
    }
    if (operation_error) {
        throw std::runtime_error("peer handshake socket failure: " + operation_error.message());
    }

    peer_handshake handshake = parse_peer_handshake(response);
    if (handshake.info_hash != info_hash) {
        throw std::runtime_error("peer handshake info_hash mismatch");
    }
    return handshake;
}

} // namespace bitnulled
