#include "json_reader.h"
#include <sstream>

namespace json_reader {

JsonReader::JsonReader(TransportCatalog::Transport::TransportCatalogue& db, 
                       const RenderSettings& render_settings,
                       const TransportRouter& router)
    : db_(db), handler_(db), renderer_(render_settings), router_(router) {}

void JsonReader::ProcessRequests(const json::Document& doc) {
    const json::Dict& root = doc.GetRoot().AsDict();
    ProcessBaseRequests(root.at("base_requests").AsArray());
    ProcessStatRequests(root.at("stat_requests").AsArray());
}

void JsonReader::ProcessBaseRequests(const json::Array& base_requests) {
    // Сначала добавляем все остановки
    for (const json::Node& request : base_requests) {
        const json::Dict& req_map = request.AsDict();
        if (req_map.at("type").AsString() == "Stop") {
            AddStop(req_map);
        }
    }
    
    // Устанавливаем расстояния между остановками
    for (const auto& [from, to, distance] : distances_to_set_) {
        const Domain::Stop* from_ = db_.FindStop(from);
        const Domain::Stop* to_ = db_.FindStop(to);
        db_.SetDistance(from_, to_, distance);  
    }
    
    // Затем добавляем все маршруты
    for (const json::Node& request : base_requests) {
        const json::Dict& req_map = request.AsDict();
        if (req_map.at("type").AsString() == "Bus") {
            AddBus(req_map);
        }
    }
}

void JsonReader::AddStop(const json::Dict& stop_dict) {
    std::string name = stop_dict.at("name").AsString();
    double lat = stop_dict.at("latitude").AsDouble();
    double lon = stop_dict.at("longitude").AsDouble();
    
    db_.AddStop(name, {lat, lon});
    
    if (stop_dict.count("road_distances")) {
        const auto& distances = stop_dict.at("road_distances").AsDict();
        for (const auto& [stop_name, distance] : distances) {
            int dist = distance.AsInt();
            distances_to_set_.emplace_back(name, stop_name, dist);
        }
    }
}

void JsonReader::AddBus(const json::Dict& bus_dict) {
    std::string name = bus_dict.at("name").AsString();
    std::vector<std::string> stops;
    for (const json::Node& stop : bus_dict.at("stops").AsArray()) {
        stops.push_back(stop.AsString());
    }
    bool is_roundtrip = bus_dict.at("is_roundtrip").AsBool();
    
    db_.AddBus(name, stops, is_roundtrip);
}

void JsonReader::ProcessStatRequests(const json::Array& stat_requests) {
    for (const json::Node& request : stat_requests) {
        const json::Dict& req_map = request.AsDict();
        json::Builder response_builder; 

        if (req_map.at("type").AsString() == "Bus") {
            ProcessBusRequest(req_map, response_builder);
        } else if (req_map.at("type").AsString() == "Stop") {
            ProcessStopRequest(req_map, response_builder);
        } else if (req_map.at("type").AsString() == "Map") {
            ProcessMapRequest(req_map, response_builder); 
        } else if (req_map.at("type").AsString() == "Route") {
            ProcessRouteRequest(req_map, response_builder);
        }

        responses_.push_back(response_builder.Build());
    }
}

void JsonReader::ProcessMapRequest(const json::Dict& request, json::Builder& response_builder) {
    int id = request.at("id").AsInt();

    std::vector<const Domain::Bus*> buses;
    for (const auto& [name, bus] : db_.GetAllBuses()) {
        buses.push_back(bus);
    }

    std::ostringstream svg_stream;
    svg::Document map = renderer_.RenderMap(buses);
    map.Render(svg_stream);
    std::string svg_string = svg_stream.str();

   response_builder.StartDict()
        .Key("map").Value(svg_string)
        .Key("request_id").Value(id)
        .EndDict();
}

void JsonReader::ProcessBusRequest(const json::Dict& request, json::Builder& response_builder) {
    int id = request.at("id").AsInt();
    const std::string& name = request.at("name").AsString();
    
    auto bus_info = handler_.GetBusStat(name);

    if (!bus_info) {
        response_builder.StartDict()
            .Key("request_id").Value(id)
            .Key("error_message").Value("not found")
            .EndDict();
        return;
    }
    
    response_builder.StartDict()
        .Key("request_id").Value(id)
        .Key("curvature").Value(bus_info->curvature)
        .Key("route_length").Value(bus_info->route_length)
        .Key("stop_count").Value(bus_info->stops_on_route)
        .Key("unique_stop_count").Value(bus_info->unique_stops)
        .EndDict();
}

void JsonReader::ProcessStopRequest(const json::Dict& request, json::Builder& response_builder) {
    int id = request.at("id").AsInt();
    const std::string& name = request.at("name").AsString(); 

    auto stop_info = handler_.GetStopInfo(name);

    if (!stop_info) {
        response_builder.StartDict()
            .Key("request_id").Value(id)
            .Key("error_message").Value("not found")
            .EndDict();
        return;
    }
    
    response_builder.StartDict()
        .Key("request_id").Value(id);

    response_builder.Key("buses").StartArray();
    for (const auto& bus : *stop_info) {
        response_builder.Value(std::string(bus));
    }
    response_builder.EndArray();

    response_builder.EndDict();
}

void JsonReader::ProcessRouteRequest(const json::Dict& request, json::Builder& response_builder) {
    int id = request.at("id").AsInt();
    std::string from = request.at("from").AsString();
    std::string to = request.at("to").AsString();

    auto route = router_.BuildRoute(from, to);

    if (!route) {
        response_builder.StartDict()
            .Key("request_id").Value(id)
            .Key("error_message").Value("not found")
            .EndDict();
        return;
    }

    response_builder.StartDict()
        .Key("request_id").Value(id)
        .Key("total_time").Value(route->total_time)
        .Key("items").StartArray();

    for (const auto& item : route->items) {
        response_builder.StartDict()
            .Key("type").Value(item.type == TransportRouter::RouteItem::Type::WAIT ? "Wait" : "Bus")
            .Key(item.type == TransportRouter::RouteItem::Type::WAIT ? "stop_name" : "bus").Value(std::string(item.name))
            .Key("time").Value(item.time);
        if (item.type == TransportRouter::RouteItem::Type::BUS) {
            response_builder.Key("span_count").Value(item.span_count);
        }
        response_builder.EndDict();
    }

    response_builder.EndArray().EndDict();
}

const json::Array& JsonReader::GetResponses() const {
    return responses_;
}

void FillTransportCatalogue(TransportCatalog::Transport::TransportCatalogue& catalog, const json::Array& base_requests) {
    for (const auto& request : base_requests) {
        const auto& req = request.AsDict();
        const std::string& type = req.at("type").AsString();

        if (type == "Stop") {
            const std::string& name = req.at("name").AsString();
            double latitude = req.at("latitude").AsDouble();
            double longitude = req.at("longitude").AsDouble();
            catalog.AddStop(name, {latitude, longitude});
        }
    }

    for (const auto& request : base_requests) {
        const auto& req = request.AsDict();
        const std::string& type = req.at("type").AsString();

        if (type == "Stop") {
            const std::string& name = req.at("name").AsString();
            const auto& distances = req.at("road_distances").AsDict();
            for (const auto& [to, distance] : distances) {
                catalog.SetDistance(catalog.FindStop(name), catalog.FindStop(to), distance.AsInt());
            }
        } else if (type == "Bus") {
            const std::string& name = req.at("name").AsString();
            const auto& stops = req.at("stops").AsArray();
            std::vector<std::string> stop_names;
            for (const auto& stop : stops) {
                stop_names.push_back(stop.AsString());
            }
            bool is_roundtrip = req.at("is_roundtrip").AsBool();
            catalog.AddBus(name, stop_names, is_roundtrip);
        }
    }
}

RenderSettings ParseRenderSettings(const json::Dict& render_settings) {
    RenderSettings settings;
    settings.width = render_settings.at("width").AsDouble();
    settings.height = render_settings.at("height").AsDouble();
    settings.padding = render_settings.at("padding").AsDouble();
    settings.line_width = render_settings.at("line_width").AsDouble();
    settings.stop_radius = render_settings.at("stop_radius").AsDouble();
    settings.bus_label_font_size = render_settings.at("bus_label_font_size").AsInt();
    const auto& bus_label_offset = render_settings.at("bus_label_offset").AsArray();
    settings.bus_label_offset = {bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
    settings.stop_label_font_size = render_settings.at("stop_label_font_size").AsInt();
    const auto& stop_label_offset = render_settings.at("stop_label_offset").AsArray();
    settings.stop_label_offset = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};
    settings.underlayer_color = ParseColor(render_settings.at("underlayer_color"));
    settings.underlayer_width = render_settings.at("underlayer_width").AsDouble();
    
    const auto& color_palette = render_settings.at("color_palette").AsArray();
    for (const auto& color : color_palette) {
        settings.color_palette.push_back(ParseColor(color));
    }
    
    return settings;
}

svg::Color ParseColor(const json::Node& color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const auto& arr = color_node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            };
        } else if (arr.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt()),
                arr[3].AsDouble()
            };
        }
    }
    return svg::NoneColor;
}

} //namespace json_reader