#pragma once

#include "domain.h"

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <set>
#include <unordered_map>
#include <vector>

namespace TransportCatalog {
namespace Transport {

class TransportCatalogue {
public:
    // Добавляет остановку
    void AddStop(const std::string& name, const Geo::Coordinates& coordinates);
    
    // Ищет остановку
    const Domain::Stop* FindStop(const std::string& name) const;
    
    // Добавляет маршрут автобуса
    void AddBus(const std::string name, const std::vector<std::string>& stops, const bool is_circular);
    
    // Ищет маршрут автобуса
    const Domain::Bus* FindBus(const std::string_view name) const;
    
    // Выдает информацию о маршрутах автобусов
    const Domain::BusInfo GetBusInfo(const std::string_view name) const;
    
    // Получение списка автобусов, проходящих через остановку
    const std::set<std::string_view>& GetBusesByStop(const std::string& stop_name) const;
    
    // Задает расстояние между остановками
    void SetDistance(const Domain::Stop* from, const Domain::Stop* to, int distance);
    
    // Получает расстояние между остановками
    int GetDistance(const Domain::Stop* from, const Domain::Stop* to) const;

    const std::unordered_map<std::string_view, const Domain::Stop*>& GetAllStops() const;
    
    const std::unordered_map<std::string_view, const Domain::Bus*>& GetAllBuses() const;
private:
    std::deque<Domain::Stop> stops_;
    std::unordered_map<std::string_view, const Domain::Stop*> stopname_to_stop_;

    std::deque<Domain::Bus> buses_;
    std::unordered_map<std::string_view, const Domain::Bus*> busname_to_bus_;

    std::unordered_map<const Domain::Stop*, std::set<std::string_view>> stop_to_buses_;

	struct StopPairHasher {
		size_t operator()(const std::pair<const Domain::Stop*, const Domain::Stop*>& pair) const {
			std::hash<const void*> hasher;
			size_t first = hasher(pair.first);
			size_t second = hasher(pair.second);
			return first + 37 * second;
		}
	};
    std::unordered_map<std::pair<const Domain::Stop*, const Domain::Stop*>, int, StopPairHasher> distances_;
};
} // namespace Transport
} // namespace TransportCatalog