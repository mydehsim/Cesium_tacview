#ifndef MANUALCONTROLLER_H
#define MANUALCONTROLLER_H

#include "core/AircraftState.h"

struct InputState
{
    bool turnLeft = false;
    bool turnRight = false;
    bool speedUp = false;
    bool speedDown = false;
    bool climbUp = false;
    bool climbDown = false;
};

class ManualController
{
public:
    static void update(AircraftState &ac, double dt, const InputState &input);

    static constexpr double HEADING_CHANGE_RATE = 45.0; // deg/s via input
    static constexpr double SPEED_CHANGE_RATE = 20.0;   // m/s^2 via input
    static constexpr double ALT_CHANGE_RATE = 15.0;     // m/s via input
};

#endif // MANUALCONTROLLER_H
