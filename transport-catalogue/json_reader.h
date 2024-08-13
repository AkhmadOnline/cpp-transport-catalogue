#pragma once

#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"
#include "json_builder.h"
#include "transport_router.h"

namespace json_reader {

class JsonReader {
public:
    JsonReader(TransportCatalog::Transport::TransportCatalogue& db, 
               const RenderSettings& render_settings,
               const TransportRouter& router);
    
    void ProcessRequests(const json::Document& doc);
    const json::Array& GetResponses() const;

private:
    TransportCatalog::Transport::TransportCatalogue& db_;
    RequestHandler handler_;
    json::Array responses_;
    MapRenderer renderer_;
    const TransportRouter& router_;

    void ProcessBaseRequests(const json::Array& base_requests);
    void ProcessStatRequests(const json::Array& stat_requests);
    
    void AddStop(const json::Dict& stop_dict);
    void AddBus(const json::Dict& bus_dict);
    
    void ProcessMapRequest(const json::Dict& request, json::Builder& response_builder);
    void ProcessBusRequest(const json::Dict& request, json::Builder& response_builder);
    void ProcessStopRequest(const json::Dict& request, json::Builder& response_builder);
    void ProcessRouteRequest(const json::Dict& request, json::Builder& response_builder);
    
    std::vector<std::tuple<std::string, std::string, int>> distances_to_set_;
};

void FillTransportCatalogue(TransportCatalog::Transport::TransportCatalogue& catalog, const json::Array& base_requests);
RenderSettings ParseRenderSettings(const json::Dict& render_settings);
svg::Color ParseColor(const json::Node& color_node);

} //namespace json_reader