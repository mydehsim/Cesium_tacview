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
    bool removeWaypoint(const QString &routeId, int index);
    bool moveWaypoint(const QString &routeId, int index, double lat, double lon, double alt);

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
