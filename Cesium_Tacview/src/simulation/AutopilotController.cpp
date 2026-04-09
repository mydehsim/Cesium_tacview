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

    // Calculate bearing and distance to target waypoint using KinematicModel utilities
    double bearing = KinematicModel::forwardAzimuth(ac.lat, ac.lon, wp.lat, wp.lon);
    double dist = KinematicModel::haversineDistance(ac.lat, ac.lon, wp.lat, wp.lon);

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
