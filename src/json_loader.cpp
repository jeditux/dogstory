#include "json_loader.h"
#include <boost/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace json_loader {

    void AddRoad(model::Map& map, const boost::json::value& road) {
        auto roadObj = road.as_object();
        
        if (!roadObj.contains("x0") || !roadObj.contains("y0")) {
            throw std::invalid_argument("Road start point is not specified");
        }
        auto roadStartX = static_cast<model::Coord>(roadObj.at("x0").as_int64());
        auto roadStartY = static_cast<model::Coord>(roadObj.at("y0").as_int64());
        model::Point roadStart{roadStartX, roadStartY};

        if (roadObj.contains("x1")) {
            auto endCoord = static_cast<model::Coord>(roadObj.at("x1").as_int64());
            model::Road road {model::Road::HORIZONTAL, roadStart, endCoord};
            map.AddRoad(road);
            return;
        }

        if (roadObj.contains("y1")) {
            auto endCoord = static_cast<model::Coord>(roadObj.at("y1").as_int64());
            model::Road road {model::Road::VERTICAL, roadStart, endCoord};
            map.AddRoad(road);
            return;
        }
        
        throw std::invalid_argument("Road end coord is not specified");
    }

    void AddBuilding(model::Map& map, const boost::json::value& building) {
        auto buildingObj = building.as_object();

        if (!buildingObj.contains("x") || !buildingObj.contains("y")) {
            throw std::invalid_argument("Building coords are not specified");
        }

        if (!buildingObj.contains("w") || !buildingObj.contains("h")) {
            throw std::invalid_argument("Building size is not specified");
        }

        auto x = static_cast<model::Coord>(buildingObj.at("x").as_int64());
        auto y = static_cast<model::Coord>(buildingObj.at("y").as_int64());
        auto w = static_cast<model::Dimension>(buildingObj.at("w").as_int64());
        auto h = static_cast<model::Dimension>(buildingObj.at("h").as_int64());

        auto coords = model::Point{x, y};
        auto size = model::Size{w, h};
        auto rect = model::Rectangle{coords, size};
        map.AddBuilding(model::Building{rect});
    }

    void AddOffice(model::Map& map, const boost::json::value& office) {
        auto officeObj = office.as_object();

        if (!officeObj.contains("id")) {
            throw std::invalid_argument("Office id is not specified");
        }

        if (!officeObj.contains("x") || !officeObj.contains("y")) {
            throw std::invalid_argument("Office coords are not specified");
        }

        if (!officeObj.contains("offsetX") || !officeObj.contains("offsetY")) {
            throw std::invalid_argument("Office offset is not specified");
        }

        auto idStr = officeObj.at("id").as_string();
        auto x = static_cast<model::Coord>(officeObj.at("x").as_int64());
        auto y = static_cast<model::Coord>(officeObj.at("y").as_int64());
        auto offsetX = static_cast<model::Dimension>(officeObj.at("offsetX").as_int64());
        auto offsetY = static_cast<model::Dimension>(officeObj.at("offsetY").as_int64());

        model::Office::Id id{{idStr.c_str(), idStr.size()}};
        auto pos = model::Point{x, y};
        auto offset = model::Offset{offsetX, offsetY};

        map.AddOffice(model::Office{id, pos, offset});
    }

    model::Game LoadGame(const std::filesystem::path& json_path) {
        namespace sys = boost::system;
    
        model::Game game;

        try {
            std::ifstream input(json_path);
            if (!input) {
                throw std::invalid_argument("Missing config file");
            }

            std::stringstream buffer;
            buffer << input.rdbuf();

            sys::error_code ec;
            auto gameConfig = boost::json::parse(buffer.str(), ec);

            auto mapsList = gameConfig.as_object()["maps"];
            for (const auto& mapObj : mapsList.as_array()) {
                auto idStr = mapObj.as_object().at("id").as_string();
                model::Map::Id id{{idStr.c_str(), idStr.size()}};
                auto name = mapObj.as_object().at("name").as_string();
                model::Map newMap{id, {name.c_str(), name.size()}};

                const auto roads = mapObj.at("roads").as_array();
                for (const auto& road : roads) {
                    AddRoad(newMap, road);
                }

                const auto buildings = mapObj.at("buildings").as_array();
                for (const auto& building : buildings) {
                    AddBuilding(newMap, building);
                }

                const auto offices = mapObj.at("offices").as_array();
                for (const auto& office : offices) {
                    AddOffice(newMap, office);
                }

                game.AddMap(newMap);
            }
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        return game;
    }

}  // namespace json_loader
