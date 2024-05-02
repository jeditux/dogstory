#pragma once
#include "http_server.h"
#include "model.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
};

enum class RequestType {
    MAP_LIST,
    MAP_BY_ID,
    UNKNOWN
};

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    bool MethodIsValid(http::verb method) {
        return method == http::verb::get || method == http::verb::head;
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        send(HandleRequest(std::move(req)));
    }

private:
    model::Game& game_;

    StringResponse HandleRequest(StringRequest&& req);

    StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type = ContentType::TEXT_HTML);

    RequestType GetRequestType(const std::vector<std::string_view>& uriParts, http::verb method) const;

    bool RequestIsCorrect(const std::vector<std::string_view>& uriParts, http::verb method) const;

    std::tuple<http::status, std::string> MakeResponse(const std::vector<std::string_view>& uriParts, http::verb method) const;
    
    std::tuple<http::status, std::string> MakeGetMapListResponse() const;

    std::tuple<http::status, std::string> MakeGetMapByIdResponse(const model::Map::Id& id) const;

    std::tuple<http::status, std::string> MakeMapNotFoundResponse() const;

    std::tuple<http::status, std::string> MakeBadRequestResponse() const;

    std::vector<std::string_view> Split(std::string_view source, std::string_view delimiter = "/");
};

}
