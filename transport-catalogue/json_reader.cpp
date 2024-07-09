#include "json_reader.h"
#include <sstream>

namespace json_reader {

JsonReader::JsonReader(TransportCatalog::Transport::TransportCatalogue& db, const RenderSettings& render_settings)
    : db_(db), handler_(db), renderer_(render_settings) {}

void JsonReader::ProcessRequests(const json::Document& doc) {
    const json::Dict& root = doc.GetRoot().AsMap();
    ProcessBaseRequests(root.at("base_requests").AsArray());
    ProcessStatRequests(root.at("stat_requests").AsArray());
}

void JsonReader::SetDistances() {
    for (const auto& [from, to, distance] : distances_to_set_) {
        const Domain::Stop* from_ = db_.FindStop(from);
        const Domain::Stop* to_ = db_.FindStop(to);
        db_.SetDistance(from_, to_, distance);  
    }
}

void JsonReader::ProcessBaseRequests(const json::Array& base_requests) {
    // Сначала добавляем все остановки
    for (const json::Node& request : base_requests) {
        const json::Dict& req_map = request.AsMap();
        if (req_map.at("type").AsString() == "Stop") {
            AddStop(req_map);
        }
    }
    
    // Устанавливаем расстояния между остановками
    SetDistances();
    
    // Затем добавляем все маршруты
    for (const json::Node& request : base_requests) {
        const json::Dict& req_map = request.AsMap();
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
        const auto& distances = stop_dict.at("road_distances").AsMap();
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
    
    //std::cerr << "DEBUG: Adding bus " << name << " with " << stops.size() << " stops" << std::endl;
    db_.AddBus(name, stops, is_roundtrip);
}

void JsonReader::ProcessStatRequests(const json::Array& stat_requests) {
    for (const json::Node& request : stat_requests) {
        const json::Dict& req_map = request.AsMap();
        if (req_map.at("type").AsString() == "Bus") {
            responses_.push_back(ProcessBusRequest(req_map));
        } else if (req_map.at("type").AsString() == "Stop") {
            responses_.push_back(ProcessStopRequest(req_map));
        } else if (req_map.at("type").AsString() == "Map") {
            responses_.push_back(ProcessMapRequest(req_map)); 
        }
    }
}

json::Node JsonReader::ProcessMapRequest(const json::Dict& request) {
    int id = request.at("id").AsInt();

    std::vector<const Domain::Bus*> buses;
    for (const auto& [name, bus] : db_.GetAllBuses()) {
        buses.push_back(bus);
    }

    std::ostringstream svg_stream;
    svg::Document map = renderer_.RenderMap(buses);
    map.Render(svg_stream);
    std::string svg_string = svg_stream.str();

    return json::Dict{
        {"map", json::Node(svg_string)},  // Экранируем SVG
        {"request_id", id}
    };
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request) {
    int id = request.at("id").AsInt();
    const std::string& name = request.at("name").AsString();
    
    auto bus_info = handler_.GetBusStat(name);
    if (!bus_info) {
        return json::Dict{
            {"request_id", id},
            {"error_message", json::Node(std::string("not found"))}
        };
    }
    
    auto response = json::Dict{
        {"request_id", id},
        {"curvature", bus_info->curvature},
        {"route_length", bus_info->route_length},
        {"stop_count", bus_info->stops_on_route},
        {"unique_stop_count", bus_info->unique_stops}
    };
    return response;
}

json::Node JsonReader::ProcessStopRequest(const json::Dict& request) {
    int id = request.at("id").AsInt();
    const auto& name_node = request.at("name"); // Получаем узел с именем остановки
    const std::string& name = name_node.AsString(); // Преобразование в std::string

    
    auto stop_info = handler_.GetStopInfo(name);
    if (!stop_info) {
        //std::cerr << "DEBUG: Stop not found: " << name << std::endl;
        return json::Dict{
            {"request_id", id},
            {"error_message", json::Node(std::string("not found"))}
        };
    }

    //std::cerr << "DEBUG: Stop found: " << name << std::endl;
    
    json::Array buses;
    for (const auto& bus : *stop_info) {
        buses.push_back(std::string(bus));
    }
    
    return json::Dict{
        {"request_id", id},
        {"buses", std::move(buses)}
    };
}

const json::Array& JsonReader::GetResponses() const {
    return responses_;
}

void FillTransportCatalogue(TransportCatalog::Transport::TransportCatalogue& catalog, const json::Array& base_requests) {
    for (const auto& request : base_requests) {
        const auto& req = request.AsMap();
        const std::string& type = req.at("type").AsString();

        if (type == "Stop") {
            const std::string& name = req.at("name").AsString();
            double latitude = req.at("latitude").AsDouble();
            double longitude = req.at("longitude").AsDouble();
            catalog.AddStop(name, {latitude, longitude});
        }
    }

    for (const auto& request : base_requests) {
        const auto& req = request.AsMap();
        const std::string& type = req.at("type").AsString();

        if (type == "Stop") {
            const std::string& name = req.at("name").AsString();
            const auto& distances = req.at("road_distances").AsMap();
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