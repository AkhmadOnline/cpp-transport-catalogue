#include "request_handler.h"
#include <iostream>

RequestHandler::RequestHandler(const TransportCatalog::Transport::TransportCatalogue& db)
    : db_(db) {}

std::optional<const Domain::BusInfo> RequestHandler::GetBusStat(const std::string& bus_name) const {
    if (db_.FindBus(bus_name)) {
        return db_.GetBusInfo(bus_name);
    }
    return std::nullopt;
}

std::optional<const std::set<std::string_view>> RequestHandler::GetStopInfo(const std::string& stop_name) const {
    const std::set<std::string_view> stops = db_.GetBusesByStop(stop_name);
    if (db_.FindStop(stop_name)) {
        return stops;
    }
    return std::nullopt;
}