#pragma once

#include "transport_catalogue.h"
#include <optional>

class RequestHandler {
public:
    RequestHandler(const TransportCatalog::Transport::TransportCatalogue& db);

    std::optional<const Domain::BusInfo> GetBusStat(const std::string& bus_name) const;
    std::optional<const std::set<std::string_view>> GetStopInfo(const std::string& stop_name) const;

private:
    const TransportCatalog::Transport::TransportCatalogue& db_;
};