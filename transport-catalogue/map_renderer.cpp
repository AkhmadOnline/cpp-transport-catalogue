#include "map_renderer.h"
#include <algorithm>
#include <unordered_set>

MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {}

svg::Document MapRenderer::RenderMap(const std::vector<const Domain::Bus*>& buses) const {
    svg::Document doc;

    std::unordered_set<const Domain::Stop*> unique_stops;
    for (const auto& bus : buses) {
        unique_stops.insert(bus->stops.begin(), bus->stops.end());
    }

    std::vector<Geo::Coordinates> all_coords;
    for (const auto& stop : unique_stops) {
        all_coords.push_back(stop->coordinates);
    }

    SphereProjector projector(all_coords.begin(), all_coords.end(), settings_.width, settings_.height, settings_.padding);

    // Отрисовка линий маршрутов
    std::vector<const Domain::Bus*> sorted_buses = buses;
    std::sort(sorted_buses.begin(), sorted_buses.end(), [](const Domain::Bus* lhs, const Domain::Bus* rhs) {
        return lhs->name < rhs->name;
    });

    size_t color_index = 0;
    for (const auto& bus : sorted_buses) {
        if (bus->stops.empty()) continue;

        svg::Polyline line;
        for (const auto& stop : bus->stops) {
            line.AddPoint(projector(stop->coordinates));
        }
        if (!bus->is_circular) {
            for (auto it = std::next(bus->stops.rbegin()); it != bus->stops.rend(); ++it) {
                line.AddPoint(projector((*it)->coordinates));
            }
        }

        line.SetStrokeColor(settings_.color_palette[color_index])
            .SetFillColor("none")
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        doc.Add(line);

        color_index = (color_index + 1) % settings_.color_palette.size(); 
    }

    // Отрисовка названий маршрутов
    color_index = 0; // Сбрасываем color_index перед отрисовкой названий
    for (const auto& bus : sorted_buses) {
        if (bus->stops.empty()) continue;

        std::vector<const Domain::Stop*> end_stops;
        end_stops.push_back(bus->stops.front());
        if (!bus->is_circular && bus->stops.front() != bus->stops.back()) {
            end_stops.push_back(bus->stops.back());
        }

        for (const auto& stop : end_stops) {
            svg::Point point = projector(stop->coordinates);

            // Подложка текста
            svg::Text underlayer;
            underlayer.SetPosition(point)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(underlayer);

            // Сам текст
            svg::Text text;
            text.SetPosition(point)
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->name)
                .SetFillColor(settings_.color_palette[color_index]); 
            doc.Add(text);
        }

        color_index = (color_index + 1) % settings_.color_palette.size();
    }

    // Отрисовка символов остановок
    std::vector<const Domain::Stop*> sorted_stops(unique_stops.begin(), unique_stops.end());
    std::sort(sorted_stops.begin(), sorted_stops.end(), [](const Domain::Stop* lhs, const Domain::Stop* rhs) {
        return lhs->name < rhs->name;
    });

    for (const auto& stop : sorted_stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white");
        doc.Add(circle);
    }

    // Отрисовка названий остановок
    for (const auto& stop : sorted_stops) {
        svg::Point point = projector(stop->coordinates);

        // Подложка текста
        svg::Text underlayer;
        underlayer.SetPosition(point)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(underlayer);

        // Сам текст
        svg::Text text;
        text.SetPosition(point)
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor("black");
        doc.Add(text);
    }

    return doc;
}