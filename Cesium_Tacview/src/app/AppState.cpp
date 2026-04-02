#include "AppState.h"

AppState::AppState(QObject *parent)
    : QObject(parent)
    , m_aircraftMgr(new AircraftManager(this))
    , m_routeMgr(new RouteManager(this))
    , m_selectionMgr(new SelectionManager(this))
{
}
