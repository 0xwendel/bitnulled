#include "bitnulled/http_client.hpp"
#include <asio.hpp>
#include <stdexcept>
#include <string>

namespace bitnulled {

struct parsed_url {
    std::string scheme;
    std::string host;
    std::string port;
    std::string target; // path + query string
};

static parsed_url parse_url(std::string_view url) {
    parsed_url out;

    auto scheme_end = url.find("://");
    if (scheme_end == std::string_view::npos) {
        throw std::runtime_error("malformed url: missing scheme");
    }

    out.scheme = std::string(url.substr(0, scheme_end));
    if (out.scheme != "http") {
        // https needs tls and that's openssl territory. not today.
        throw std::runtime_error("only plain http trackers are supported");
    }

    auto host_start = scheme_end + 3;
    auto path_start = url.find('/', host_start);

    std::string_view authority;
    if (path_start == std::string_view::npos) {
        authority = url.substr(host_start);
        out.target = "/";
    } else {
        authority = url.substr(host_start, path_start - host_start);
        out.target = std::string(url.substr(path_start));
    }

    auto port_sep = authority.rfind(':');
    if (port_sep != std::string_view::npos) {
        out.host = std::string(authority.substr(0, port_sep));
        out.port = std::string(authority.substr(port_sep + 1));
    } else {
        out.host = std::string(authority);
        out.port = "80";
    }

    if (out.host.empty()) {
        throw std::runtime_error("malformed url: empty host");
    }

    return out;
}

http_response http_get(std::string_view url) {
    parsed_url parts = parse_url(url);

    asio::io_context io_context;
    asio::ip::tcp::resolver resolver(io_context);
    asio::ip::tcp::socket socket(io_context);

    // dns resolution. yeah it blocks, the whole point of this client is sync.
    asio::error_code ec;
    auto endpoints = resolver.resolve(parts.host, parts.port, ec);
    if (ec) {
        throw std::runtime_error("dns resolution failed for " + parts.host + ": " + ec.message());
    }

    asio::connect(socket, endpoints, ec);
    if (ec) {
        throw std::runtime_error("tcp connect failed: " + ec.message());
    }

    // build the request by hand. http/1.1 requires the host header, and
    // connection: close so the server slams the door when it's done talking.
    std::string request;
    request.reserve(256 + parts.target.size() + parts.host.size());
    request += "GET " + parts.target + " HTTP/1.1\r\n";
    request += "Host: " + parts.host + "\r\n";
    request += "Accept: */*\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";

    asio::write(socket, asio::buffer(request), ec);
    if (ec) {
        throw std::runtime_error("failed to send http request: " + ec.message());
    }

    // slurp the whole response until eof. trackers send small payloads,
    // no need for fancy streaming parsers here.
    std::string raw_response;
    std::array<char, 4096> read_buf{};

    while (true) {
        size_t n = socket.read_some(asio::buffer(read_buf), ec);
        if (n > 0) {
            raw_response.append(read_buf.data(), n);
        }
        if (ec == asio::error::eof) {
            break;
        }
        if (ec) {
            throw std::runtime_error("socket read failed: " + ec.message());
        }
    }

    // find the header/body delimiter first. we validate the body exists
    // before touching the status line so error messages stay coherent.
    auto header_end = raw_response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        throw std::runtime_error("malformed http response: missing header terminator");
    }

    auto status_line_end = raw_response.find("\r\n");
    if (status_line_end == std::string::npos || status_line_end > header_end) {
        throw std::runtime_error("malformed http response: missing status line");
    }

    // "HTTP/1.1 200 OK" -> skip two spaces to reach the status code digits.
    std::string_view status_line(raw_response.data(), status_line_end);
    auto sp1 = status_line.find(' ');
    if (sp1 == std::string_view::npos) {
        throw std::runtime_error("malformed http status line");
    }

    int status_code = 0;
    try {
        status_code = std::stoi(std::string(status_line.substr(sp1 + 1)));
    } catch (...) {
        throw std::runtime_error("malformed http status code");
    }

    http_response response;
    response.status_code = status_code;
    response.body = raw_response.substr(header_end + 4);
    return response;
}

} // namespace bitnulled
