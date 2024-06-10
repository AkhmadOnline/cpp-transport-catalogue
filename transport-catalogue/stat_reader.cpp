#include "stat_reader.h"

#include <iomanip>

using namespace std;

namespace TransportCatalog {
namespace Stat { 
namespace detail {
enum class RequestType {
    BUS,
    STOP,
    INVALID
};

struct ParsedRequest {
    RequestType type;
    std::string name;
};

ParsedRequest ParseRequest(std::string_view request) {
    if (request.empty()) {
        return {RequestType::INVALID, {}};
    }

    auto space_pos = request.find(' ');
    if (space_pos == request.npos) {
        return {RequestType::INVALID, {}};
    }

    auto type_str = request.substr(0, space_pos);
    auto name = std::string(request.substr(space_pos + 1));

    if (type_str == "Bus") {
        return {RequestType::BUS, name};
    } else if (type_str == "Stop") {
        return {RequestType::STOP, name};
    } else {
        return {RequestType::INVALID, {}};
    }
}

void PrintStat(const Transport::TransportCatalogue& transport_catalogue, 
               const ParsedRequest& request, std::ostream& output) {
    if (request.type == RequestType::BUS) {
        if (const auto info = transport_catalogue.GetBusInfo(request.name); info.stops_on_route > 0) {
            output << "Bus " << request.name << ": " << info.stops_on_route
                   << " stops on route, " << info.unique_stops << " unique stops, "
                   << setprecision(6) << info.route_length << " route length\n";
        } else {
            output << "Bus " << request.name << ": not found\n";
        }
    } else if (request.type == RequestType::STOP) {
        if (const auto* stop = transport_catalogue.FindStop(request.name); stop) {
            const auto& buses = transport_catalogue.GetBusesByStop(request.name);
            if (buses.empty()) {
                output << "Stop " << request.name << ": no buses\n";
            } else {
                output << "Stop " << request.name << ": buses";
                for (const auto& bus : buses) {
                    output << " " << bus;
                }
                output << "\n";
            }
        } else {
            output << "Stop " << request.name << ": not found\n";
        }
    } 
}
}//namespace detail

void ParseAndPrintStat(const Transport::TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output) {
    auto parsed_request = detail::ParseRequest(request);
    detail::PrintStat(transport_catalogue, parsed_request, output);
}
} // namespace Stat
} // namespace TransportCatalog
