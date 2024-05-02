#include "http_server.h"
#include <iostream>

namespace http_server {

    using namespace std::literals;
    
    void ReportError(beast::error_code ec, std::string_view what) {
        std::cerr << what << ": "sv << ec.message() << std::endl;
    }

}  // namespace http_server
