#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "json_reader.h"

#include <iostream>
#include <string>

using namespace std;

int main() {
    json::Document doc = json::Load(cin);
    const auto& input = doc.GetRoot().AsDict();

    TransportCatalog::Transport::TransportCatalogue catalogue;
    json_reader::FillTransportCatalogue(catalogue, input.at("base_requests").AsArray());

    const auto& routing_settings = input.at("routing_settings").AsDict();
    int bus_wait_time = routing_settings.at("bus_wait_time").AsInt();
    double bus_velocity = routing_settings.at("bus_velocity").AsDouble();

    TransportRouter router(catalogue, bus_wait_time, bus_velocity);

    const auto& render_settings = input.at("render_settings").AsDict();
    RenderSettings settings = json_reader::ParseRenderSettings(render_settings);

    // Создаем JsonReader с настройками рендеринга и маршрутизатором:
    json_reader::JsonReader reader(catalogue, settings, router);

    // Обрабатываем запросы:
    reader.ProcessRequests(doc);

    // Выводим ответы в JSON:
    json::Print(json::Document(reader.GetResponses()), cout);

    return 0;
}