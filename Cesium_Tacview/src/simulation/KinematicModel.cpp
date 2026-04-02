#include "KinematicModel.h"
#include <cmath>
#include <algorithm>

void KinematicModel::update(AircraftState &ac, double dt)
{
    if (dt <= 0.0 || ac.controlMode == ControlMode::IDLE)
        return;

    // --- Heading interpolation toward target ---
    double headingDiff = ac.targetHeading - ac.heading;
    // Normalize to [-180, 180]
    while (headingDiff > 180.0)  headingDiff -= 360.0;
    while (headingDiff < -180.0) headingDiff += 360.0;

    double maxTurn = TURN_RATE * dt;
    if (std::abs(headingDiff) <= maxTurn) {
        ac.heading = ac.targetHeading;
    } else {
        ac.heading += (headingDiff > 0 ? maxTurn : -maxTurn);
    }
    // Normalize heading to [0, 360)
    while (ac.heading < 0)    ac.heading += 360.0;
    while (ac.heading >= 360) ac.heading -= 360.0;

    // --- Speed interpolation toward target ---
    double speedDiff = ac.targetSpeed - ac.speed;
    double maxAccel = ACCEL_RATE * dt;
    if (std::abs(speedDiff) <= maxAccel) {
        ac.speed = ac.targetSpeed;
    } else {
        ac.speed += (speedDiff > 0 ? maxAccel : -maxAccel);
    }
    ac.speed = std::clamp(ac.speed, 0.0, MAX_SPEED);

    // --- Altitude interpolation toward target ---
    double altDiff = ac.targetAlt - ac.alt;
    double maxClimb = CLIMB_RATE * dt;
    if (std::abs(altDiff) <= maxClimb) {
        ac.alt = ac.targetAlt;
        ac.verticalSpeed = 0.0;
    } else {
        double vs = (altDiff > 0 ? CLIMB_RATE : -CLIMB_RATE);
        ac.alt += vs * dt;
        ac.verticalSpeed = vs;
    }

    // --- Position update ---
    double headingRad = ac.heading * DEG_TO_RAD;
    double latRad = ac.lat * DEG_TO_RAD;

    double dLat = (ac.speed * std::cos(headingRad) * dt) / EARTH_RADIUS;
    double dLon = (ac.speed * std::sin(headingRad) * dt) / (EARTH_RADIUS * std::cos(latRad));

    ac.lat += dLat * RAD_TO_DEG;
    ac.lon += dLon * RAD_TO_DEG;

    // Clamp latitude
    ac.lat = std::clamp(ac.lat, -90.0, 90.0);

    // Wrap longitude
    while (ac.lon > 180.0)  ac.lon -= 360.0;
    while (ac.lon < -180.0) ac.lon += 360.0;
}
