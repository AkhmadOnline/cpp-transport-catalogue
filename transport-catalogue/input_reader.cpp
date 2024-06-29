#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace TransportCatalog {
namespace Input {

namespace detail {
/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
Geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::unordered_map<std::string, int> ParseStopDistances(std::string_view str) {
    std::unordered_map<std::string, int> distances;
    while (!str.empty()) {
        auto pos = str.find(',');
        auto sub_str = str.substr(0, pos);
        auto m_pos = sub_str.find('m');
        if (m_pos != sub_str.npos) {
            auto dist = std::stoi(std::string(sub_str.substr(0, m_pos)));
            auto stop_name = std::string(Trim(sub_str.substr(m_pos + 1)));
            // Удаляем префикс "to " из имени остановки
            if (stop_name.substr(0, 3) == "to ") {
                stop_name = stop_name.substr(3);
            }
            distances[stop_name] = dist;
        }
        if (pos == str.npos) break;
        str.remove_prefix(pos + 1);
    }
    return distances;
}

std::pair<std::string_view, std::string_view> SplitLatLngAndDistances(std::string_view description) {
    auto pos = description.find(',');
    auto coords_end = description.find_first_of(',', pos + 1);
    auto distances_str = description.substr(coords_end + 1);
    return {description.substr(0, coords_end), distances_str};
}
} // namespace detail

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return detail::Split(route, '>');
    }

    auto stops = detail::Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description.command == "Stop") {
        auto [lat_lng, distances_str] = detail::SplitLatLngAndDistances(command_description.description);
        auto distances = detail::ParseStopDistances(distances_str);
        stop_distances_[command_description.id] = distances;
        commands_.push_back(command_description);
    } else if (command_description.command == "Bus") {
        commands_.push_back(command_description);
    }
}

void InputReader::ApplyCommands(Transport::TransportCatalogue& catalogue) const {
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            auto [lat, lng] = detail::ParseCoordinates(command.description);
            catalogue.AddStop(command.id, {lat, lng});
        }
    }

    for (const auto& [stop_name, distances] : stop_distances_) {
        const TransportCatalog::Transport::Stop* from = catalogue.FindStop(stop_name);
        for (const auto& [to_name, distance] : distances) {
            const TransportCatalog::Transport::Stop* to = catalogue.FindStop(to_name);
            catalogue.SetDistance(from, to, distance);
        }
    }

    for (const auto& command : commands_) {
        if (command.command == "Bus") {
            auto stops = ParseRoute(command.description);
            std::vector<std::string> stop_names(stops.begin(), stops.end());
            bool is_circular = command.description.find('>') != std::string::npos;
            if (is_circular && stop_names.front() != stop_names.back()) {
                stop_names.push_back(stop_names.front());
            }
            catalogue.AddBus(command.id, stop_names, is_circular);
        }
    }
}
} // namespace Input
} // namespace TransportCatalog