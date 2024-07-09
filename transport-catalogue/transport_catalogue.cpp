#include "transport_catalogue.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace TransportCatalog {
namespace Transport {

void TransportCatalogue::AddStop(const string& name, const Geo::Coordinates& coordinates) {
    stops_.push_back({name, coordinates});
    stopname_to_stop_[stops_.back().name] = &stops_.back();
}

const Domain::Stop* TransportCatalogue::FindStop(const std::string_view name) const {
    if (stopname_to_stop_.count(name)) {
        return stopname_to_stop_.at(name);
    }
    return nullptr;
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string>& stops, const bool is_circular) { 
    buses_.push_back({name, {}, is_circular}); 
    auto& bus_stops = buses_.back().stops; 
    bus_stops.reserve(stops.size()); 
    for (const auto& stop_name : stops) { 
        bus_stops.push_back(stopname_to_stop_.at(stop_name)); 
    } 
    busname_to_bus_[buses_.back().name] = &buses_.back(); 
    for (const auto& stop_name : stops) {
        stop_to_buses_[stopname_to_stop_.at(stop_name)].insert(buses_.back().name); 
    } 
} 

const Domain::Bus* TransportCatalogue::FindBus(const std::string_view name) const {
    if (busname_to_bus_.count(name)) {
        return busname_to_bus_.at(name);
    }
    return nullptr;
}

const Domain::BusInfo TransportCatalogue::GetBusInfo(const std::string_view name) const {
    Domain::BusInfo info;
    if (const auto* bus = FindBus(name); bus) {
        set<string> unique_stops;
        info.stops_on_route = bus->is_circular ? bus->stops.size() : bus->stops.size() * 2 - 1;
        size_t last_stop = bus->is_circular ? bus->stops.size() - 1 : bus->stops.size() - 1;
            
            for (size_t i = 0; i < last_stop; ++i) {
                int distance = GetDistance(bus->stops[i], bus->stops[i + 1]);
                if (distance == 0) {
                    distance = GetDistance(bus->stops[i + 1], bus->stops[i]);
                }
                info.route_length += distance;
                info.geo_length += ComputeDistance(bus->stops[i]->coordinates, bus->stops[i + 1]->coordinates);
                unique_stops.insert(bus->stops[i]->name);
            }
            unique_stops.insert(bus->stops[last_stop]->name);

            if (!bus->is_circular) {
                for (size_t i = last_stop; i > 0; --i) {
                    int distance = GetDistance(bus->stops[i], bus->stops[i - 1]);
                    if (distance == 0) {
                        distance = GetDistance(bus->stops[i - 1], bus->stops[i]);
                    }
                    info.route_length += distance;
                    info.geo_length += ComputeDistance(bus->stops[i]->coordinates, bus->stops[i - 1]->coordinates);
                }
            }

            info.unique_stops = unique_stops.size();
            if (info.geo_length > 0) {
                info.curvature = info.route_length / info.geo_length;
            } else {
                info.curvature = 1;
            }
    }
    return info;
}

const std::set<std::string_view>& TransportCatalogue::GetBusesByStop(const std::string& stop_name) const { 
    static std::set<std::string_view> empty_set;
    if (const auto* stop = FindStop(stop_name); stop && stop_to_buses_.count(stop) > 0) {  
        return stop_to_buses_.at(stop); 
    } else { 
        return empty_set;  
    } 
} 

void TransportCatalogue::SetDistance(const Domain::Stop* from, const Domain::Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

int TransportCatalogue::GetDistance(const Domain::Stop* from, const Domain::Stop* to) const {
    if (auto it = distances_.find({from, to}); it != distances_.end()) {
        return it->second;
    }
    if (auto it = distances_.find({to, from}); it != distances_.end()) {
        return it->second;
    }
    return 0;
}

const std::unordered_map<std::string_view, const Domain::Stop*>& TransportCatalogue::GetAllStops() const {
    return stopname_to_stop_;
}
    
const std::unordered_map<std::string_view, const Domain::Bus*>& TransportCatalogue::GetAllBuses() const {
    return busname_to_bus_;
}
} // namespace Transport
} // namespace TransportCatalog