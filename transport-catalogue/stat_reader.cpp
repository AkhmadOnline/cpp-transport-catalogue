// место для вашего кода
#include "stat_reader.h"

#include <iomanip>

using namespace std;

namespace TransportCatalog {
namespace Stat { 
void ParseAndPrintStat(const Transport::TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    // Реализуйте самостоятельно
    if (request.empty()) { //  Завершаем работу, если строка запроса пустая
        return;
    }
    auto space_pos = request.find(' ');

    if (space_pos == request.npos) {
        return;
    }

    string name(request.substr(space_pos + 1));
    if (request.substr(0, 4) == "Bus ") {
        if (const auto info = transport_catalogue.GetBusInfo(name); info.stops_on_route > 0) {
            output << "Bus " << name << ": " << info.stops_on_route 
                   << " stops on route, " << info.unique_stops << " unique stops, "
                   << setprecision(6) << info.route_length << " route length\n";
        } else {
            output << "Bus " << name << ": not found\n";
        }
    } else if (request.substr(0, 5) == "Stop ") {
        if (const auto* stop = transport_catalogue.FindStop(name); stop) {
            const auto& buses = transport_catalogue.GetBusesByStop(name);
            if (buses.empty()) {
                output << "Stop " << name << ": no buses\n";
            } else {
                output << "Stop " << name << ": buses";
                for (const auto& bus : buses) {
                    output << " " << bus;
                }
                output << "\n";
            }
        } else {
            output << "Stop " << name << ": not found\n";
        }
    }
}
} // namespace Stat
} // namespace TransportCatalog