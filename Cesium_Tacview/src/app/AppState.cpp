#include "AppState.h"
#include "simulation/TrackRecorder.h"
#include "simulation/PlaybackEngine.h"

AppState::AppState(QObject *parent)
    : QObject(parent),
      m_aircraftMgr(new AircraftManager(this)),
      m_routeMgr(new RouteManager(this)),
      m_selectionMgr(new SelectionManager(this)),
      m_trackRecorder(new TrackRecorder(this)),
      m_playbackEngine(new PlaybackEngine(this, this))
{
}

void AppState::setMode(AppMode mode)
{
    if (m_mode != mode)
    {
        m_mode = mode;
        emit modeChanged(mode);
    }
}

void AppState::clearAll()
{
    for (const QString &id : m_aircraftMgr->aircraftIds())
        m_aircraftMgr->removeAircraft(id);
    for (const QString &id : m_routeMgr->routeIds())
        m_routeMgr->removeRoute(id);
    m_selectionMgr->clearSelection();
    m_trackRecorder->clear();
    m_playbackEngine->stop();
    m_mode = AppMode::SIMULATION;
}
