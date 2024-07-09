#pragma once
#include "geo.h"
#include <string>
#include <vector>

namespace Domain {

// Хранит имя и координаты остановки
struct Stop {
    std::string name;
    Geo::Coordinates coordinates;
};

// Хранит имя, остановки и тип маршрута
struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_circular;
};

// Хранит кол-во остановок в автобусе, уникальные остановки, длину маршрута
struct BusInfo {
    int stops_on_route = 0;
    int unique_stops = 0;
    double route_length = 0.0;
    double geo_length = 0.0;
    double curvature = 0.0;
};

} // namespace Domain