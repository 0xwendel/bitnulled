#pragma once
#include <string>
#include <string_view>

namespace bitnulled {

struct http_response {
    int status_code{0};
    std::string body;
};

// performs a synchronous blocking HTTP GET request via asio standalone.
// throws std::runtime_error on dns, socket, or protocol failures.
// caller is responsible for checking http_response::status_code.
http_response http_get(std::string_view url);

} // namespace bitnulled
