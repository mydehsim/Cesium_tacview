#include "KinematicModel.h"
#include <cmath>
#include <algorithm>

void KinematicModel::update(AircraftState &ac, double dt)
{
    if (dt <= 0.0 || ac.controlMode == ControlMode::IDLE)
        return;

    // ═══════════════════════════════════════════════════════════
    // 1. Speed-dependent banked turn
    //    Turn radius R = v² / (g · tan(φ))
    //    Turn rate   ω = g · tan(φ) / v   (rad/s)
    //    Max turn rate limited by MAX_BANK_ANGLE
    // ═══════════════════════════════════════════════════════════

    double headingDiff = ac.targetHeading - ac.heading;
    // Normalize to [-180, 180]
    while (headingDiff > 180.0)
        headingDiff -= 360.0;
    while (headingDiff < -180.0)
        headingDiff += 360.0;

    // Calculate maximum turn rate from max bank angle at current speed
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

    // Desired turn rate to reach target heading (clamped by physics)
    double desiredTurnRate = std::clamp(headingDiff / dt, -maxTurnRateDeg, maxTurnRateDeg);

    // Apply heading change
    double actualTurnRate = desiredTurnRate;
    if (std::abs(headingDiff) <= std::abs(actualTurnRate * dt))
    {
        ac.heading = ac.targetHeading;
        actualTurnRate = headingDiff / dt; // actual rate used this frame
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

    // ═══════════════════════════════════════════════════════════
    // 2. Bank angle (roll) from actual turn rate
    //    φ = atan(v · ω / g)
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
    // 3. Speed interpolation toward target
    // ═══════════════════════════════════════════════════════════

    double speedDiff = ac.targetSpeed - ac.speed;
    double maxAccel = ACCEL_RATE * dt;
    if (std::abs(speedDiff) <= maxAccel)
        ac.speed = ac.targetSpeed;
    else
        ac.speed += (speedDiff > 0 ? maxAccel : -maxAccel);
    ac.speed = std::clamp(ac.speed, 0.0, MAX_SPEED);

    // ═══════════════════════════════════════════════════════════
    // 4. Altitude interpolation toward target + pitch calculation
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

    // Pitch from vertical speed / forward speed
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
    // 5. Position update (great-circle approximation)
    // ═══════════════════════════════════════════════════════════

    double headingRad = ac.heading * DEG_TO_RAD;
    double latRad = ac.lat * DEG_TO_RAD;

    double dLat = (ac.speed * std::cos(headingRad) * dt) / EARTH_RADIUS;
    double dLon = (ac.speed * std::sin(headingRad) * dt) / (EARTH_RADIUS * std::cos(latRad));

    ac.lat += dLat * RAD_TO_DEG;
    ac.lon += dLon * RAD_TO_DEG;

    ac.lat = std::clamp(ac.lat, -90.0, 90.0);
    while (ac.lon > 180.0)
        ac.lon -= 360.0;
    while (ac.lon < -180.0)
        ac.lon += 360.0;

    // Sync yaw = heading
    ac.yaw = ac.heading;
}
