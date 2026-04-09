#ifndef ROUTEMANAGER_H
#define ROUTEMANAGER_H

#include <QObject>
#include <QMap>
#include "core/RouteState.h"

class RouteManager : public QObject
{
    Q_OBJECT
public:
    explicit RouteManager(QObject *parent = nullptr);

    bool createRoute(const RouteState &route);
    bool removeRoute(const QString &id);
    RouteState *route(const QString &id);
    const RouteState *route(const QString &id) const;
    QStringList routeIds() const;
    const QMap<QString, RouteState> &allRoutes() const;

    // Waypoint operations
    bool addWaypoint(const QString &routeId, const Waypoint &wp);
    bool insertWaypoint(const QString &routeId, int index, const Waypoint &wp);
    bool removeWaypoint(const QString &routeId, int index);
    bool moveWaypoint(const QString &routeId, int index, double lat, double lon, double alt);
    bool reorderWaypoint(const QString &routeId, int fromIndex, int toIndex);
    bool setWaypointSpeed(const QString &routeId, int index, double speed);
    bool setLoopMode(const QString &routeId, bool loop);

    void emitChanged() { emit stateChanged(); }

signals:
    void routeCreated(const QString &id);
    void routeRemoved(const QString &id);
    void routeUpdated(const QString &id);
    void stateChanged();

private:
    QMap<QString, RouteState> m_routes;
};

#endif // ROUTEMANAGER_H
