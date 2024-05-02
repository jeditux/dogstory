#include "request_handler.h"
#include <boost/json.hpp>

namespace http_handler {
    StringResponse RequestHandler::HandleRequest(StringRequest&& req) {
        const auto text_response = [&req, this](http::status status, std::string_view text) {
            return MakeStringResponse(status, text, req.version(), req.keep_alive(), ContentType::APPLICATION_JSON);
        };

        auto method = req.method();
        auto target = req.target();
        auto uriParts = Split(target);
        auto [status, body] = MakeResponse(uriParts, method);

        return text_response(status, body);
    }

    std::tuple<http::status, std::string> RequestHandler::MakeResponse(const std::vector<std::string_view>& uriParts, http::verb method) const {
        auto requestType = GetRequestType(uriParts, method);
        switch (requestType) {
        case RequestType::MAP_LIST:
            return MakeGetMapListResponse();
        case RequestType::MAP_BY_ID:
            return MakeGetMapByIdResponse(model::Map::Id{uriParts[3].data()});
        default:
            return MakeBadRequestResponse();
        }
    }

    StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestType RequestHandler::GetRequestType(const std::vector<std::string_view>& uriParts, http::verb method) const {
        if (!RequestIsCorrect(uriParts, method)) {
            return RequestType::UNKNOWN;
        }
        if (uriParts.size() == 3) {
            return RequestType::MAP_LIST;
        }
        return RequestType::MAP_BY_ID;
    }

    bool RequestHandler::RequestIsCorrect(const std::vector<std::string_view>& uriParts, http::verb method) const {
        return (method == http::verb::get) && 
            (uriParts.size() == 3 || uriParts.size() == 4) &&
            uriParts[0] == "api" && uriParts[1] == "v1" && uriParts[2] == "maps";
    }

    std::tuple<http::status, std::string> RequestHandler::MakeGetMapListResponse() const {
        boost::json::array mapsJson{};
        auto maps = game_.GetMaps();
        for (const auto& map : maps) {
            mapsJson.emplace_back(boost::json::value{{"id", *map.GetId()}, {"name", map.GetName()}});
        }
        return std::make_tuple(http::status::ok, boost::json::serialize(mapsJson)); 
    }

    std::tuple<http::status, std::string> RequestHandler::MakeGetMapByIdResponse(const model::Map::Id& id) const {
        auto map = game_.FindMap(id);
        if (!map) {
            return MakeMapNotFoundResponse();
        }

        boost::json::array roadsJson{};
        for (const auto& road : map->GetRoads()) {
            auto roadStart = road.GetStart();
            auto roadEnd = road.GetEnd();
            if (road.IsHorizontal()) {
                boost::json::object roadObj = {
                    {"x0", roadStart.x},
                    {"y0", roadStart.y},
                    {"x1", roadEnd.x}
                };
                roadsJson.emplace_back(roadObj);
            } else if (road.IsVertical()) {
                boost::json::object roadObj = {
                    {"x0", roadStart.x},
                    {"y0", roadStart.y},
                    {"y1", roadEnd.y}
                };
                roadsJson.emplace_back(roadObj);
            }
        }

        boost::json::array buildingsJson{};
        for (const auto& building : map->GetBuildings()) {
            auto bounds = building.GetBounds();
            boost::json::object buildingObj = {
                {"x", bounds.position.x},
                {"y", bounds.position.y},
                {"w", bounds.size.width},
                {"h", bounds.size.height}
            };
            buildingsJson.emplace_back(buildingObj);
        }

        boost::json::array officesJson{};
        for (const auto& office : map->GetOffices()) {
            auto id = *office.GetId();
            auto pos = office.GetPosition();
            auto offset = office.GetOffset();
            
            boost::json::object officeObj = {
                {"id", id},
                {"x", pos.x},
                {"y", pos.y},
                {"offsetX", offset.dx},
                {"offsetY", offset.dy}
            };
            officesJson.emplace_back(officeObj);
        }

        boost::json::object mapJson = {
            {"id", *map->GetId()},
            {"name", map->GetName()},
            {"roads", roadsJson},
            {"buildings", buildingsJson},
            {"offices", officesJson}
        };

        return std::make_tuple(http::status::ok, boost::json::serialize(mapJson));
    }

    std::tuple<http::status, std::string> RequestHandler::MakeMapNotFoundResponse() const {
        boost::json::object errorObj {
            {"code", "mapNotFound"},
            {"message", "Map not found"}
        };
        return std::make_tuple(http::status::not_found, boost::json::serialize(errorObj));
    }

    std::tuple<http::status, std::string> RequestHandler::MakeBadRequestResponse() const {
        boost::json::object errorObj {
            {"code", "badRequest"},
            {"message", "Bad request"}
        };
        return std::make_tuple(http::status::bad_request, boost::json::serialize(errorObj));
    }

    std::vector<std::string_view> RequestHandler::Split(std::string_view source, std::string_view delimiter) {
        std::vector<std::string_view> output;
        size_t first = 0;

        while (first < source.size()) {
            const auto second = source.find_first_of(delimiter, first);

            if (first != second)
                output.emplace_back(source.substr(first, second - first));

            if (second == std::string_view::npos)
                break;

            first = second + 1;
        }

        return output;
    }
}
