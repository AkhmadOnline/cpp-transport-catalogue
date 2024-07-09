#pragma once

namespace Geo {

struct Coordinates {
    double lat;
    double lng;
};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo