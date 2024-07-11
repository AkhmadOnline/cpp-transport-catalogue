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

    const auto& render_settings = input.at("render_settings").AsDict();
    RenderSettings settings = json_reader::ParseRenderSettings(render_settings);

    // Создаем JsonReader с настройками рендеринга:
    json_reader::JsonReader reader(catalogue, settings); 

    // Обрабатываем запросы:
    reader.ProcessRequests(doc);

    // Выводим ответы в JSON:
    json::Print(json::Document(reader.GetResponses()), cout);

    return 0;
}