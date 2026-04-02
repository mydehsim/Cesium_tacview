#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <Qt>

namespace KeyBindings {
    constexpr int TurnLeft      = Qt::Key_A;
    constexpr int TurnLeft2     = Qt::Key_Left;
    constexpr int TurnRight     = Qt::Key_D;
    constexpr int TurnRight2    = Qt::Key_Right;
    constexpr int SpeedUp       = Qt::Key_W;
    constexpr int SpeedUp2      = Qt::Key_Up;
    constexpr int SpeedDown     = Qt::Key_S;
    constexpr int SpeedDown2    = Qt::Key_Down;
    constexpr int ClimbUp       = Qt::Key_Q;
    constexpr int ClimbDown     = Qt::Key_E;
    constexpr int ToggleMode    = Qt::Key_Space;
    constexpr int NextAircraft  = Qt::Key_Tab;
    constexpr int FocusCamera   = Qt::Key_F;
    constexpr int TogglePause   = Qt::Key_P;
    constexpr int ClearSelection = Qt::Key_Escape;
}

#endif // KEYBINDINGS_H
