#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>

class TransportRouter {
public:
    TransportRouter(const TransportCatalog::Transport::TransportCatalogue& db, int bus_wait_time, double bus_velocity);
    
    struct RouteItem {
        enum class Type { WAIT, BUS };
        Type type;
        std::string_view name;
        double time;
        int span_count;  // Только для типа BUS
    };

    struct RouteInfo {
        double total_time;
        std::vector<RouteItem> items;
    };

    std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

private:
    const TransportCatalog::Transport::TransportCatalogue& db_;
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    
    int bus_wait_time_;
    double bus_velocity_;

    std::unordered_map<std::string_view, size_t> stop_to_vertex_;
    std::vector<std::string_view> vertex_to_stop_;

    void BuildGraph();
    std::string_view FindBusBetweenStops(std::string_view from, std::string_view to) const;
    int CalculateSpansBetweenStops(std::string_view bus_name, std::string_view from, std::string_view to) const;
    RouteInfo ConvertRouteToRouteInfo(const graph::Router<double>::RouteInfo& route) const;
};