#pragma once

#include <iosfwd>
#include <string_view>
#include <string>

#include "transport_catalogue.h"

namespace TransportCatalog {

namespace Stat {

void ParseAndPrintStat(const Transport::TransportCatalogue& transport_catalogue, std::string_view request,
                       std::ostream& output);
} // namespace Stat
} // namespace TransportCatalog