#ifndef AUTOPILOTCONTROLLER_H
#define AUTOPILOTCONTROLLER_H

#include "core/AircraftState.h"
#include "core/RouteState.h"

class AutopilotController
{
public:
    // Update aircraft targets to follow route. Returns true if route completed.
    static bool update(AircraftState &ac, RouteState &route, double dt);

    static constexpr double WAYPOINT_REACH_THRESHOLD = 200.0; // meters
    static constexpr double CRUISE_SPEED = 100.0;             // m/s default

private:
    static double bearingBetween(double lat1, double lon1, double lat2, double lon2);
    static double distanceBetween(double lat1, double lon1, double lat2, double lon2);
};

#endif // AUTOPILOTCONTROLLER_H
