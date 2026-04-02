#include "AutopilotController.h"
#include "KinematicModel.h"
#include <cmath>

bool AutopilotController::update(AircraftState &ac, RouteState &route, double /*dt*/)
{
    if (route.waypoints.isEmpty())
        return true;

    int idx = route.currentWaypointIndex;
    if (idx >= route.waypoints.size())
    {
        if (route.loopMode)
        {
            route.currentWaypointIndex = 0;
            idx = 0;
        }
        else
        {
            ac.controlMode = ControlMode::IDLE;
            ac.targetSpeed = 0;
            return true;
        }
    }

    const Waypoint &wp = route.waypoints[idx];

    // Calculate bearing and distance to target waypoint
    double bearing = bearingBetween(ac.lat, ac.lon, wp.lat, wp.lon);
    double dist = distanceBetween(ac.lat, ac.lon, wp.lat, wp.lon);

    // Set targets
    ac.targetHeading = bearing;
    ac.targetAlt = wp.alt;
    ac.targetSpeed = (wp.speedOverride >= 0) ? wp.speedOverride : CRUISE_SPEED;

    // Check if waypoint reached
    if (dist < WAYPOINT_REACH_THRESHOLD)
    {
        route.currentWaypointIndex++;
    }

    return false;
}

double AutopilotController::bearingBetween(double lat1, double lon1,
                                           double lat2, double lon2)
{
    double dLat1 = lat1 * KinematicModel::DEG_TO_RAD;
    double dLat2 = lat2 * KinematicModel::DEG_TO_RAD;
    double dLon = (lon2 - lon1) * KinematicModel::DEG_TO_RAD;

    double y = std::sin(dLon) * std::cos(dLat2);
    double x = std::cos(dLat1) * std::sin(dLat2) - std::sin(dLat1) * std::cos(dLat2) * std::cos(dLon);

    double bearing = std::atan2(y, x) * KinematicModel::RAD_TO_DEG;
    while (bearing < 0)
        bearing += 360.0;
    return bearing;
}

double AutopilotController::distanceBetween(double lat1, double lon1,
                                            double lat2, double lon2)
{
    double dLat = (lat2 - lat1) * KinematicModel::DEG_TO_RAD;
    double dLon = (lon2 - lon1) * KinematicModel::DEG_TO_RAD;
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) + std::cos(lat1 * KinematicModel::DEG_TO_RAD) *
                                                             std::cos(lat2 * KinematicModel::DEG_TO_RAD) *
                                                             std::sin(dLon / 2) * std::sin(dLon / 2);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return KinematicModel::EARTH_RADIUS * c;
}
