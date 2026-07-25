#include "bitnulled/utils.hpp"

namespace bitnulled {

std::string to_hex(std::span<const uint8_t> bytes) {
    // fast hex conversion so we don't bleed CPU on stringstreams
    static constexpr char hex_digits[] = "0123456789abcdef";
    std::string hex_str;
    hex_str.reserve(bytes.size() * 2);
    for (uint8_t b : bytes) {
        hex_str.push_back(hex_digits[(b >> 4) & 0x0f]);
        hex_str.push_back(hex_digits[b & 0x0f]);
    }
    return hex_str;
}

std::string url_encode(std::span<const uint8_t> bytes) {
    // rfc 3986 percent-encoding for raw binary infohash
    static constexpr char hex_digits[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(bytes.size() * 3);

    for (uint8_t b : bytes) {
        if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || 
            (b >= '0' && b <= '9') || b == '.' || b == '-' || b == '_' || b == '~') {
            encoded.push_back(static_cast<char>(b));
        } else {
            encoded.push_back('%');
            encoded.push_back(hex_digits[(b >> 4) & 0x0f]);
            encoded.push_back(hex_digits[b & 0x0f]);
        }
    }
    return encoded;
}

} // namespace bitnulled
