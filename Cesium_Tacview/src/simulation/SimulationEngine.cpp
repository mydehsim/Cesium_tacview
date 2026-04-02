#include "SimulationEngine.h"
#include "KinematicModel.h"
#include "AutopilotController.h"
#include "app/AppState.h"

SimulationEngine::SimulationEngine(AppState *appState, QObject *parent)
    : QObject(parent)
    , m_appState(appState)
{
    connect(&m_timer, &QTimer::timeout, this, &SimulationEngine::tick);
    m_timer.setTimerType(Qt::PreciseTimer);
}

void SimulationEngine::start()
{
    int intervalMs = 1000 / m_clock.tickRate();
    m_timer.start(intervalMs);
    m_clock.setPaused(false);
    emit logMessage(QStringLiteral("[Sim] Started at %1 Hz").arg(m_clock.tickRate()));
}

void SimulationEngine::stop()
{
    m_timer.stop();
    m_clock.reset();
    emit logMessage(QStringLiteral("[Sim] Stopped"));
}

void SimulationEngine::pause()
{
    m_clock.setPaused(true);
    emit logMessage(QStringLiteral("[Sim] Paused"));
}

void SimulationEngine::resume()
{
    m_clock.setPaused(false);
    emit logMessage(QStringLiteral("[Sim] Resumed"));
}

void SimulationEngine::togglePause()
{
    if (m_clock.isPaused())
        resume();
    else
        pause();
}

void SimulationEngine::tick()
{
    if (m_clock.isPaused())
        return;

    m_clock.advance();
    double dt = m_clock.dt();

    auto *acMgr = m_appState->aircraftManager();
    auto *rtMgr = m_appState->routeManager();
    const QString controlTarget = m_appState->selectionManager()->activeControlTarget();

    // Update each aircraft
    for (const QString &id : acMgr->aircraftIds()) {
        AircraftState *ac = acMgr->aircraft(id);
        if (!ac) continue;

        switch (ac->controlMode) {
        case ControlMode::MANUAL:
            if (id == controlTarget) {
                ManualController::update(*ac, dt, m_inputState);
            }
            KinematicModel::update(*ac, dt);
            break;

        case ControlMode::AUTOPILOT: {
            RouteState *route = rtMgr->route(ac->currentRouteId);
            if (route) {
                AutopilotController::update(*ac, *route, dt);
            }
            KinematicModel::update(*ac, dt);
            break;
        }

        case ControlMode::SCRIPTED:
            KinematicModel::update(*ac, dt);
            break;

        case ControlMode::IDLE:
            break;
        }

        ac->addTrailPoint();
    }

    emit tickCompleted(m_clock.currentTick());
}
