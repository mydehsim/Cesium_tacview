#include "InputManager.h"
#include "KeyBindings.h"
#include "app/AppState.h"
#include "simulation/SimulationEngine.h"
#include "bridge/CesiumBridge.h"
#include "bridge/StateSerializer.h"
#include <QJsonObject>

InputManager::InputManager(AppState *appState, SimulationEngine *engine,
                           CesiumBridge *bridge, QObject *parent)
    : QObject(parent), m_appState(appState), m_engine(engine), m_bridge(bridge)
{
}

void InputManager::keyPressed(int key)
{
    m_pressedKeys.insert(key);

    // Update engine input state for continuous keys (thread-safe)
    InputState input;
    input.turnLeft = m_pressedKeys.contains(KeyBindings::TurnLeft) || m_pressedKeys.contains(KeyBindings::TurnLeft2);
    input.turnRight = m_pressedKeys.contains(KeyBindings::TurnRight) || m_pressedKeys.contains(KeyBindings::TurnRight2);
    input.speedUp = m_pressedKeys.contains(KeyBindings::SpeedUp) || m_pressedKeys.contains(KeyBindings::SpeedUp2);
    input.speedDown = m_pressedKeys.contains(KeyBindings::SpeedDown) || m_pressedKeys.contains(KeyBindings::SpeedDown2);
    input.climbUp = m_pressedKeys.contains(KeyBindings::ClimbUp);
    input.climbDown = m_pressedKeys.contains(KeyBindings::ClimbDown);
    m_engine->setInputState(input);

    // Auto-switch to MANUAL mode on WASD input
    if (input.turnLeft || input.turnRight || input.speedUp ||
        input.speedDown || input.climbUp || input.climbDown)
    {
        const QString target = m_appState->selectionManager()->activeControlTarget();
        if (!target.isEmpty())
        {
            AircraftState *ac = m_appState->aircraftManager()->aircraft(target);
            if (ac && ac->controlMode != ControlMode::MANUAL)
            {
                ac->controlMode = ControlMode::MANUAL;
                emit logMessage(QStringLiteral("Manual override: %1").arg(target));
            }
        }
    }

    // One-shot actions
    handleAction(key);
}

void InputManager::keyReleased(int key)
{
    m_pressedKeys.remove(key);

    InputState input;
    input.turnLeft = m_pressedKeys.contains(KeyBindings::TurnLeft) || m_pressedKeys.contains(KeyBindings::TurnLeft2);
    input.turnRight = m_pressedKeys.contains(KeyBindings::TurnRight) || m_pressedKeys.contains(KeyBindings::TurnRight2);
    input.speedUp = m_pressedKeys.contains(KeyBindings::SpeedUp) || m_pressedKeys.contains(KeyBindings::SpeedUp2);
    input.speedDown = m_pressedKeys.contains(KeyBindings::SpeedDown) || m_pressedKeys.contains(KeyBindings::SpeedDown2);
    input.climbUp = m_pressedKeys.contains(KeyBindings::ClimbUp);
    input.climbDown = m_pressedKeys.contains(KeyBindings::ClimbDown);
    m_engine->setInputState(input);
}

InputState InputManager::currentInputState() const
{
    InputState input;
    input.turnLeft = m_pressedKeys.contains(KeyBindings::TurnLeft) || m_pressedKeys.contains(KeyBindings::TurnLeft2);
    input.turnRight = m_pressedKeys.contains(KeyBindings::TurnRight) || m_pressedKeys.contains(KeyBindings::TurnRight2);
    input.speedUp = m_pressedKeys.contains(KeyBindings::SpeedUp) || m_pressedKeys.contains(KeyBindings::SpeedUp2);
    input.speedDown = m_pressedKeys.contains(KeyBindings::SpeedDown) || m_pressedKeys.contains(KeyBindings::SpeedDown2);
    input.climbUp = m_pressedKeys.contains(KeyBindings::ClimbUp);
    input.climbDown = m_pressedKeys.contains(KeyBindings::ClimbDown);
    return input;
}

void InputManager::handleAction(int key)
{
    if (key == KeyBindings::ToggleMode)
    {
        const QString target = m_appState->selectionManager()->activeControlTarget();
        if (!target.isEmpty())
        {
            AircraftState *ac = m_appState->aircraftManager()->aircraft(target);
            if (ac)
            {
                switch (ac->controlMode)
                {
                case ControlMode::IDLE:
                    ac->controlMode = ControlMode::MANUAL;
                    break;
                case ControlMode::MANUAL:
                    ac->controlMode = ControlMode::AUTOPILOT;
                    break;
                case ControlMode::AUTOPILOT:
                    ac->controlMode = ControlMode::MANUAL;
                    break;
                default:
                    ac->controlMode = ControlMode::MANUAL;
                    break;
                }
                emit logMessage(QStringLiteral("Mode: %1 → %2")
                                    .arg(target, controlModeToString(ac->controlMode)));
            }
        }
    }
    else if (key == KeyBindings::NextAircraft)
    {
        selectNextAircraft();
    }
    else if (key == KeyBindings::FocusCamera)
    {
        const QString sel = m_appState->selectionManager()->selectedEntityId();
        if (!sel.isEmpty())
        {
            QJsonObject payload;
            payload[QStringLiteral("entityId")] = sel;
            m_bridge->pushCommand(StateSerializer::createCommand(
                QStringLiteral("CMD_CAMERA_TRACK"), payload));
        }
    }
    else if (key == KeyBindings::TogglePause)
    {
        m_engine->togglePause();
    }
    else if (key == KeyBindings::ClearSelection)
    {
        m_appState->selectionManager()->clearSelection();
        m_bridge->pushFullSync();
    }
}

void InputManager::selectNextAircraft()
{
    QStringList ids = m_appState->aircraftManager()->aircraftIds();
    if (ids.isEmpty())
        return;

    QString current = m_appState->selectionManager()->selectedEntityId();
    int idx = ids.indexOf(current);
    int next = (idx + 1) % ids.size();

    m_appState->selectionManager()->selectEntity(ids[next], SelectionType::AIRCRAFT);
    m_bridge->pushFullSync();
    emit logMessage(QStringLiteral("Selected: %1").arg(ids[next]));
}
