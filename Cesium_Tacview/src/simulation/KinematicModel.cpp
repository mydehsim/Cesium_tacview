#include "KinematicModel.h"
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════
// Great-circle destination point (Vincenty-style on sphere)
//   lat1, lon1 in degrees; bearingDeg in degrees; distMeters in meters
//   outputs lat2Deg, lon2Deg in degrees
// ═══════════════════════════════════════════════════════════════════
void KinematicModel::greatCircleDestination(double lat1Deg, double lon1Deg,
                                            double bearingDeg, double distMeters,
                                            double &lat2Deg, double &lon2Deg)
{
    double lat1 = lat1Deg * DEG_TO_RAD;
    double lon1 = lon1Deg * DEG_TO_RAD;
    double brng = bearingDeg * DEG_TO_RAD;
    double delta = distMeters / EARTH_RADIUS; // angular distance

    double sinLat1 = std::sin(lat1);
    double cosLat1 = std::cos(lat1);
    double sinDelta = std::sin(delta);
    double cosDelta = std::cos(delta);

    double lat2 = std::asin(sinLat1 * cosDelta + cosLat1 * sinDelta * std::cos(brng));
    double lon2 = lon1 + std::atan2(
                             std::sin(brng) * sinDelta * cosLat1,
                             cosDelta - sinLat1 * std::sin(lat2));

    lat2Deg = lat2 * RAD_TO_DEG;
    lon2Deg = lon2 * RAD_TO_DEG;

    // Normalize
    lat2Deg = std::clamp(lat2Deg, -90.0, 90.0);
    while (lon2Deg > 180.0)
        lon2Deg -= 360.0;
    while (lon2Deg < -180.0)
        lon2Deg += 360.0;
}

// ═══════════════════════════════════════════════════════════════════
// Forward azimuth (initial bearing) between two points, degrees [0,360)
// ═══════════════════════════════════════════════════════════════════
double KinematicModel::forwardAzimuth(double lat1, double lon1,
                                      double lat2, double lon2)
{
    double dLat1 = lat1 * DEG_TO_RAD;
    double dLat2 = lat2 * DEG_TO_RAD;
    double dLon = (lon2 - lon1) * DEG_TO_RAD;

    double y = std::sin(dLon) * std::cos(dLat2);
    double x = std::cos(dLat1) * std::sin(dLat2) -
               std::sin(dLat1) * std::cos(dLat2) * std::cos(dLon);

    double bearing = std::atan2(y, x) * RAD_TO_DEG;
    while (bearing < 0.0)
        bearing += 360.0;
    return bearing;
}

// ═══════════════════════════════════════════════════════════════════
// Haversine distance (meters) between two lat/lon in degrees
// ═══════════════════════════════════════════════════════════════════
double KinematicModel::haversineDistance(double lat1, double lon1,
                                         double lat2, double lon2)
{
    double dLat = (lat2 - lat1) * DEG_TO_RAD;
    double dLon = (lon2 - lon1) * DEG_TO_RAD;
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1 * DEG_TO_RAD) * std::cos(lat2 * DEG_TO_RAD) *
                   std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS * c;
}

// ═══════════════════════════════════════════════════════════════════
// Magnetic declination — simplified tilted dipole model (WMM 2025)
//   Returns declination in degrees (positive = East)
// ═══════════════════════════════════════════════════════════════════
double KinematicModel::magneticDeclination(double lat, double lon)
{
    double latRad = lat * DEG_TO_RAD;
    double lonRad = lon * DEG_TO_RAD;
    double poleLatRad = MAG_POLE_LAT * DEG_TO_RAD;
    double poleLonRad = MAG_POLE_LON * DEG_TO_RAD;

    // Geomagnetic colatitude
    double sinGeoLat = std::sin(latRad) * std::sin(poleLatRad) +
                       std::cos(latRad) * std::cos(poleLatRad) *
                           std::cos(lonRad - poleLonRad);
    sinGeoLat = std::clamp(sinGeoLat, -1.0, 1.0);
    double geoLat = std::asin(sinGeoLat);
    double cosGeoLat = std::cos(geoLat);

    // Near magnetic poles, declination is undefined
    if (std::abs(cosGeoLat) < 0.01)
        return 0.0;

    double sinDec = std::sin(lonRad - poleLonRad) * std::cos(poleLatRad) / cosGeoLat;
    sinDec = std::clamp(sinDec, -1.0, 1.0);

    return std::asin(sinDec) * RAD_TO_DEG;
}

// ═══════════════════════════════════════════════════════════════════
// Main kinematic update — 6-DOF aircraft state integration
// ═══════════════════════════════════════════════════════════════════
void KinematicModel::update(AircraftState &ac, double dt)
{
    if (dt <= 0.0 || ac.controlMode == ControlMode::IDLE)
        return;

    // Save previous position for ground track calculation
    double prevLat = ac.lat;
    double prevLon = ac.lon;

    // ═══════════════════════════════════════════════════════════
    // 1. HEADING — Speed-dependent banked turn
    //    Turn radius R = v² / (g · tan(φ))
    //    Turn rate   ω = g · tan(φ) / v   (rad/s)
    // ═══════════════════════════════════════════════════════════

    double headingDiff = ac.targetHeading - ac.heading;
    // Normalize to [-180, 180]
    while (headingDiff > 180.0)
        headingDiff -= 360.0;
    while (headingDiff < -180.0)
        headingDiff += 360.0;

    // Maximum turn rate from max bank angle at current speed
    double maxTurnRateDeg;
    if (ac.speed > MIN_TURN_SPEED)
    {
        double maxBankRad = MAX_BANK_ANGLE * DEG_TO_RAD;
        double maxTurnRateRad = GRAVITY * std::tan(maxBankRad) / ac.speed;
        maxTurnRateDeg = maxTurnRateRad * RAD_TO_DEG;
    }
    else
    {
        maxTurnRateDeg = FALLBACK_TURN_RATE;
    }

    // Desired turn rate clamped by physics
    double desiredTurnRate = std::clamp(headingDiff / dt, -maxTurnRateDeg, maxTurnRateDeg);
    double actualTurnRate = desiredTurnRate;

    if (std::abs(headingDiff) <= std::abs(actualTurnRate * dt))
    {
        ac.heading = ac.targetHeading;
        actualTurnRate = headingDiff / dt;
    }
    else
    {
        ac.heading += actualTurnRate * dt;
    }

    // Normalize heading to [0, 360)
    while (ac.heading < 0)
        ac.heading += 360.0;
    while (ac.heading >= 360)
        ac.heading -= 360.0;

    // Store turn rate for telemetry
    ac.turnRate = actualTurnRate;

    // ═══════════════════════════════════════════════════════════
    // 2. BANK ANGLE (ROLL) from actual turn rate
    //    φ = atan(v · ω / g) — coordinated turn bank angle
    //    Smoothly interpolate toward required bank angle
    // ═══════════════════════════════════════════════════════════

    double requiredRoll = 0.0;
    if (ac.speed > MIN_TURN_SPEED)
    {
        double turnRateRad = actualTurnRate * DEG_TO_RAD;
        requiredRoll = std::atan2(ac.speed * turnRateRad, GRAVITY) * RAD_TO_DEG;
    }

    double rollDiff = requiredRoll - ac.roll;
    double maxRollChange = BANK_RATE * dt;
    if (std::abs(rollDiff) <= maxRollChange)
        ac.roll = requiredRoll;
    else
        ac.roll += (rollDiff > 0 ? maxRollChange : -maxRollChange);

    // ═══════════════════════════════════════════════════════════
    // 3. SPEED — interpolation toward target with acceleration limit
    // ═══════════════════════════════════════════════════════════

    double speedDiff = ac.targetSpeed - ac.speed;
    double maxAccel = ACCEL_RATE * dt;
    if (std::abs(speedDiff) <= maxAccel)
        ac.speed = ac.targetSpeed;
    else
        ac.speed += (speedDiff > 0 ? maxAccel : -maxAccel);
    ac.speed = std::clamp(ac.speed, 0.0, MAX_SPEED);

    // ═══════════════════════════════════════════════════════════
    // 4. ALTITUDE — interpolation + pitch from flight path angle
    //    γ (flight path angle) = asin(Vv / V)
    // ═══════════════════════════════════════════════════════════

    double altDiff = ac.targetAlt - ac.alt;
    double maxClimb = CLIMB_RATE * dt;
    if (std::abs(altDiff) <= maxClimb)
    {
        ac.alt = ac.targetAlt;
        ac.verticalSpeed = 0.0;
    }
    else
    {
        double vs = (altDiff > 0 ? CLIMB_RATE : -CLIMB_RATE);
        ac.alt += vs * dt;
        ac.verticalSpeed = vs;
    }

    // Pitch from flight path angle
    double requiredPitch = 0.0;
    if (ac.speed > MIN_TURN_SPEED)
        requiredPitch = std::asin(std::clamp(ac.verticalSpeed / ac.speed, -1.0, 1.0)) * RAD_TO_DEG;

    double pitchDiff = requiredPitch - ac.pitch;
    double maxPitchChange = PITCH_RATE * dt;
    if (std::abs(pitchDiff) <= maxPitchChange)
        ac.pitch = requiredPitch;
    else
        ac.pitch += (pitchDiff > 0 ? maxPitchChange : -maxPitchChange);

    // ═══════════════════════════════════════════════════════════
    // 5. POSITION — Great-circle destination formula
    //    Standard spherical earth model (WGS-84 mean radius)
    // ═══════════════════════════════════════════════════════════

    double distTraveled = ac.speed * dt;
    if (distTraveled > 0.001)
    {
        greatCircleDestination(ac.lat, ac.lon, ac.heading, distTraveled,
                               ac.lat, ac.lon);
    }

    // ═══════════════════════════════════════════════════════════
    // 6. DERIVED TELEMETRY
    //    - Yaw (= heading for coordinated flight)
    //    - Ground track (actual direction of movement)
    //    - Magnetic heading (true heading - declination)
    //    - G-load from bank angle: n = 1/cos(φ)
    // ═══════════════════════════════════════════════════════════

    ac.yaw = ac.heading;

    // Ground track: azimuth of actual movement vector
    double movedDist = haversineDistance(prevLat, prevLon, ac.lat, ac.lon);
    if (movedDist > 0.1)
    {
        ac.groundTrack = forwardAzimuth(prevLat, prevLon, ac.lat, ac.lon);
    }
    // else keep previous ground track

    ac.groundSpeed = ac.speed; // no wind model

    // Magnetic heading
    double decl = magneticDeclination(ac.lat, ac.lon);
    ac.magneticHeading = ac.heading - decl;
    while (ac.magneticHeading < 0)
        ac.magneticHeading += 360.0;
    while (ac.magneticHeading >= 360)
        ac.magneticHeading -= 360.0;

    // G-load in coordinated turn
    double rollRad = ac.roll * DEG_TO_RAD;
    double cosRoll = std::cos(rollRad);
    ac.gLoad = (std::abs(cosRoll) > 0.01) ? 1.0 / cosRoll : 1.0;
}
