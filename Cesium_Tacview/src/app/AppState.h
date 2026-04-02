#ifndef APPSTATE_H
#define APPSTATE_H

#include <QObject>
#include "core/AircraftManager.h"
#include "core/RouteManager.h"
#include "core/SelectionManager.h"

class AppState : public QObject
{
    Q_OBJECT
public:
    explicit AppState(QObject *parent = nullptr);

    AircraftManager  *aircraftManager()  { return m_aircraftMgr; }
    RouteManager     *routeManager()     { return m_routeMgr; }
    SelectionManager *selectionManager() { return m_selectionMgr; }

    const AircraftManager  *aircraftManager()  const { return m_aircraftMgr; }
    const RouteManager     *routeManager()     const { return m_routeMgr; }
    const SelectionManager *selectionManager() const { return m_selectionMgr; }

signals:
    void fullSyncRequired();

private:
    AircraftManager  *m_aircraftMgr;
    RouteManager     *m_routeMgr;
    SelectionManager *m_selectionMgr;
};

#endif // APPSTATE_H
