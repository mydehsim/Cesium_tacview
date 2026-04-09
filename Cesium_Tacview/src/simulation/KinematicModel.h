#ifndef KINEMATICMODEL_H
#define KINEMATICMODEL_H

#include "core/AircraftState.h"

class KinematicModel
{
public:
    // Update aircraft position based on current heading/speed for dt seconds
    static void update(AircraftState &ac, double dt);

    // Magnetic declination (simplified dipole model, WMM 2025 epoch)
    static double magneticDeclination(double lat, double lon);

    // Great-circle destination point given start lat/lon (deg), bearing (deg), distance (m)
    static void greatCircleDestination(double lat1Deg, double lon1Deg,
                                       double bearingDeg, double distMeters,
                                       double &lat2Deg, double &lon2Deg);

    // Forward azimuth (bearing) between two points (degrees)
    static double forwardAzimuth(double lat1, double lon1, double lat2, double lon2);

    // Haversine distance between two points (meters)
    static double haversineDistance(double lat1, double lon1, double lat2, double lon2);

    // Constants
    static constexpr double EARTH_RADIUS = 6371000.0; // meters
    static constexpr double DEG_TO_RAD = 0.017453292519943295;
    static constexpr double RAD_TO_DEG = 57.29577951308232;
    static constexpr double GRAVITY = 9.81;           // m/s²
    static constexpr double MAX_SPEED = 1000.0;       // m/s (~Mach 3)
    static constexpr double MAX_BANK_ANGLE = 55.0;    // degrees (fighter max bank)
    static constexpr double BANK_RATE = 40.0;         // degrees/second (roll rate)
    static constexpr double PITCH_RATE = 15.0;        // degrees/second (pitch rate)
    static constexpr double ACCEL_RATE = 10.0;        // m/s²
    static constexpr double CLIMB_RATE = 20.0;        // m/s
    static constexpr double MIN_TURN_SPEED = 5.0;     // m/s (below this, use fallback)
    static constexpr double FALLBACK_TURN_RATE = 3.0; // deg/s for very slow aircraft

    // Magnetic pole position (WMM 2025 epoch)
    static constexpr double MAG_POLE_LAT = 80.7;  // degrees N
    static constexpr double MAG_POLE_LON = -72.7; // degrees (W is negative)
};

#endif // KINEMATICMODEL_H
