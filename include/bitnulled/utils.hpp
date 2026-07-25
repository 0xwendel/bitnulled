#pragma once
#include <string>
#include <cstdint>
#include <span>

namespace bitnulled {

// convert raw byte buffer to hexadecimal ascii string
std::string to_hex(std::span<const uint8_t> bytes);

// convert raw byte buffer to url-encoded string (%xx) for tracker HTTP GET
std::string url_encode(std::span<const uint8_t> bytes);

} // namespace bitnulled
