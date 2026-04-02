#include "ManualController.h"

void ManualController::update(AircraftState &ac, double dt, const InputState &input)
{
    // Heading target
    if (input.turnLeft)
        ac.targetHeading -= HEADING_CHANGE_RATE * dt;
    if (input.turnRight)
        ac.targetHeading += HEADING_CHANGE_RATE * dt;

    // Normalize target heading
    while (ac.targetHeading < 0)
        ac.targetHeading += 360.0;
    while (ac.targetHeading >= 360)
        ac.targetHeading -= 360.0;

    // Speed target
    if (input.speedUp)
        ac.targetSpeed += SPEED_CHANGE_RATE * dt;
    if (input.speedDown)
        ac.targetSpeed -= SPEED_CHANGE_RATE * dt;

    if (ac.targetSpeed < 0.0)
        ac.targetSpeed = 0.0;
    if (ac.targetSpeed > 1000.0)
        ac.targetSpeed = 1000.0;

    // Altitude target
    if (input.climbUp)
        ac.targetAlt += ALT_CHANGE_RATE * dt;
    if (input.climbDown)
        ac.targetAlt -= ALT_CHANGE_RATE * dt;

    if (ac.targetAlt < 0.0)
        ac.targetAlt = 0.0;
}
