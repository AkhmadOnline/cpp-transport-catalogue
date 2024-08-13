#include "transport_router.h"

TransportRouter::TransportRouter(const TransportCatalog::Transport::TransportCatalogue& db, int bus_wait_time, double bus_velocity)
    : db_(db), bus_wait_time_(bus_wait_time), bus_velocity_(bus_velocity * 1000 / 60)  // км/ч -> м/мин
{
    BuildGraph();
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

void TransportRouter::BuildGraph() {
    size_t vertex_count = 2 * db_.GetAllStops().size();
    graph_ = graph::DirectedWeightedGraph<double>(vertex_count);

    size_t vertex_id = 0;
    for (const auto& [stop_name, _] : db_.GetAllStops()) {
        stop_to_vertex_[stop_name] = vertex_id;
        vertex_to_stop_.push_back(stop_name);
        vertex_to_stop_.push_back(stop_name);

        graph_.AddEdge({vertex_id, vertex_id + 1, static_cast<double>(bus_wait_time_)});
        vertex_id += 2;
    }

    for (const auto& [_, bus] : db_.GetAllBuses()) {
        const auto& stops = bus->stops;
        size_t last_stop_idx = stops.size();
        if (!bus->is_circular) {
            last_stop_idx--;
        }
        for (size_t i = 0; i < last_stop_idx; ++i) {
            for (size_t j = i + 1; j <= last_stop_idx; ++j) {
                // Прямое направление
                double distance = 0;
                for (size_t k = i; k < j; ++k) {
                    distance += db_.GetDistance(stops[k], stops[(k + 1) % stops.size()]);
                }
                double time = distance / bus_velocity_;
                size_t from_vertex = stop_to_vertex_[stops[i]->name] + 1;
                size_t to_vertex = stop_to_vertex_[stops[j % stops.size()]->name];
                graph_.AddEdge({from_vertex, to_vertex, time});

                // Обратное направление (только если маршрут не кольцевой)
                if (!bus->is_circular) {
                    double reverse_distance = 0;
                    for (size_t k = j; k > i; --k) {
                        reverse_distance += db_.GetDistance(stops[k % stops.size()], stops[(k - 1) % stops.size()]);
                    }
                    double reverse_time = reverse_distance / bus_velocity_;
                    size_t reverse_from_vertex = stop_to_vertex_[stops[j % stops.size()]->name] + 1;
                    size_t reverse_to_vertex = stop_to_vertex_[stops[i]->name];
                    graph_.AddEdge({reverse_from_vertex, reverse_to_vertex, reverse_time});
                }
            }
        }
    }
}

std::optional<TransportRouter::RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    size_t from_vertex = stop_to_vertex_.at(from);
    size_t to_vertex = stop_to_vertex_.at(to);

    auto route = router_->BuildRoute(from_vertex, to_vertex);
    if (!route) {
        return std::nullopt;
    }

    return ConvertRouteToRouteInfo(*route);
}

TransportRouter::RouteInfo TransportRouter::ConvertRouteToRouteInfo(const graph::Router<double>::RouteInfo& route) const {
    RouteInfo result;
    result.total_time = route.weight;

    for (size_t i = 0; i < route.edges.size(); ++i) {
        const auto& edge = graph_.GetEdge(route.edges[i]);
        if (edge.from % 2 == 0) {
            // Wait edge
            result.items.push_back({RouteItem::Type::WAIT, vertex_to_stop_[edge.from], edge.weight, 0});
        } else {
            // Bus edge
            const std::string_view from_stop = vertex_to_stop_[edge.from - 1];
            const std::string_view to_stop = vertex_to_stop_[edge.to];
                const std::string_view bus_name = FindBusBetweenStops(from_stop, to_stop);
    int span_count = CalculateSpansBetweenStops(bus_name, from_stop, to_stop);
            result.items.push_back({RouteItem::Type::BUS, bus_name, edge.weight, span_count});
        }
    }

    return result;
}

std::string_view TransportRouter::FindBusBetweenStops(std::string_view from, std::string_view to) const {
    for (const auto& [name, bus] : db_.GetAllBuses()) {
        const auto& stops = bus->stops;
        auto it_from = std::find_if(stops.begin(), stops.end(), [from](const Domain::Stop* stop) { return stop->name == from; });
        auto it_to = std::find_if(stops.begin(), stops.end(), [to](const Domain::Stop* stop) { return stop->name == to; });
        if (it_from != stops.end() && it_to != stops.end() && std::distance(it_from, it_to) > 0) {
            return name;
        }
    }
    return {};
}

int TransportRouter::CalculateSpansBetweenStops(std::string_view bus_name, std::string_view from, std::string_view to) const {
    const auto& bus = db_.FindBus(bus_name);
    if (!bus) return 0;
    const auto& stops = bus->stops;
    auto it_from = std::find_if(stops.begin(), stops.end(), [from](const Domain::Stop* stop) { return stop->name == from; });
    auto it_to = std::find_if(stops.begin(), stops.end(), [to](const Domain::Stop* stop) { return stop->name == to; });
    if (it_from == stops.end() || it_to == stops.end()) return 0;
    return std::abs(std::distance(it_from, it_to));
}