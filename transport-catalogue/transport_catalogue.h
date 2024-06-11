#pragma once

#include "geo.h"

#include <deque>
#include <iostream>
#include <string>
#include <string_view>
#include <set>
#include <unordered_map>
#include <vector>

namespace TransportCatalog {
	namespace Transport {
//Хранит имя и координаты остановки
struct Stop {
	std::string name;
	Geo::Coordinates coordinates;
};

//Хранит имя, остановки и тип маршрута
struct Bus {
	std::string name;
	std::vector<const Stop*> stops;
	bool is_circular;
};

//Хранит кол-во остоновок в автобусе, уникальные остановки, длину маршрута
struct BusInfo {
        int stops_on_route = 0;
        int unique_stops = 0;
        double route_length = 0.0;
};

class TransportCatalogue {
	// Реализуйте класс самостоятельно
	public:
		//Добовляет остановку
		void AddStop(const std::string& name, const Geo::Coordinates& coordinates);

		//Ищет остановку
		const Stop* FindStop(const std::string& name) const;

		//Добавляет маршрут автобуса
		void AddBus(const std::string name, const std::vector<std::string>& stops, const bool is_circular);

		//Ищет маршрут автобуса
		const Bus* FindBus(const std::string_view name) const;

		//Выдает инфорамацию о маршрутах автобусов
		const BusInfo GetBusInfo(const std::string_view name) const; //изменил string на string_view

		//Получение списка автобусов, проходящих через остановку
		const std::set<std::string> GetBusesByStop(const std::string& stop_name) const;
	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;

		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;

		std::unordered_map<const Stop*, std::set<std::string>> stop_to_buses_;
};
	}//namespace Transport
}//namespace TransportCatalog
