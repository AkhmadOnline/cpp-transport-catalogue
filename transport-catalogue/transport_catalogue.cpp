#include "transport_catalogue.h"

#include <algorithm>

using namespace std;

namespace TransportCatalog {
namespace Transport {
void TransportCatalogue::AddStop(const string& name, const Geo::Coordinates& coordinates) {
    stops_.push_back({name, coordinates});
    stopname_to_stop_[stops_.back().name] = &stops_.back();
}

const Stop* TransportCatalogue::FindStop(const std::string& name) const {
    if (stopname_to_stop_.count(name)) {
        return stopname_to_stop_.at(name);
    }
    return nullptr;
}

void TransportCatalogue::AddBus(const std::string name, const std::vector<std::string>& stops, const bool is_circular) { 
    buses_.push_back({name, {}, is_circular}); 
    auto& bus_stops = buses_.back().stops; 
    bus_stops.reserve(stops.size()); 
    for (const auto& stop_name : stops) { 
        bus_stops.push_back(stopname_to_stop_.at(stop_name)); 
    } 
    busname_to_bus_[buses_.back().name] = &buses_.back(); 
    for (const auto& stop_name : stops) { 
        stop_to_buses_[stopname_to_stop_.at(stop_name)].insert(name);  
    } 
} 


const Bus* TransportCatalogue::FindBus(const std::string_view name) const {
    if (busname_to_bus_.count(name)) {
        return busname_to_bus_.at(name);
    }
    return nullptr;
}

const BusInfo TransportCatalogue::GetBusInfo(const std::string_view name) const {
    BusInfo info;
    if (const auto* bus = FindBus(name); bus) {
        set<string> unique_stops;
        info.stops_on_route = bus->stops.size();
        for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
            info.route_length += ComputeDistance(bus->stops[i]->coordinates, bus->stops[i + 1]->coordinates);
            unique_stops.insert(bus->stops[i]->name);
        }
        unique_stops.insert(bus->stops.back()->name);

        // Для некольцевых маршрутов добавляем расстояние от последней до первой остановки
        if (!bus->is_circular) {  
            info.route_length += ComputeDistance(bus->stops.back()->coordinates, bus->stops.front()->coordinates); 
        }

        info.unique_stops = unique_stops.size();
    }
    return info;
}

const std::set<std::string> TransportCatalogue::GetBusesByStop(const std::string& stop_name) const { 
    if (const auto* stop = FindStop(stop_name); stop && stop_to_buses_.count(stop) > 0) {  
        return stop_to_buses_.at(stop); 
    } else { 
        return {};  
    } 
} 
} // namespace Transport
} // namespace TransportCatalog