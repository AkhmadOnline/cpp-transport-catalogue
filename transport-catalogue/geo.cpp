#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>

namespace Geo {

double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from.lat == to.lat && from.lng == to.lng) {
        return 0.0;
    }
    static const double dr = M_PI / 180.0;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * 6371000;
}
    
}  // namespace geo