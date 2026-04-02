#include "RouteManager.h"

RouteManager::RouteManager(QObject *parent)
    : QObject(parent)
{
}

bool RouteManager::createRoute(const RouteState &route)
{
    if (m_routes.contains(route.id))
        return false;
    m_routes.insert(route.id, route);
    emit routeCreated(route.id);
    emit stateChanged();
    return true;
}

bool RouteManager::removeRoute(const QString &id)
{
    if (!m_routes.contains(id))
        return false;
    m_routes.remove(id);
    emit routeRemoved(id);
    emit stateChanged();
    return true;
}

RouteState *RouteManager::route(const QString &id)
{
    auto it = m_routes.find(id);
    return it != m_routes.end() ? &it.value() : nullptr;
}

const RouteState *RouteManager::route(const QString &id) const
{
    auto it = m_routes.find(id);
    return it != m_routes.end() ? &it.value() : nullptr;
}

QStringList RouteManager::routeIds() const
{
    return m_routes.keys();
}

const QMap<QString, RouteState> &RouteManager::allRoutes() const
{
    return m_routes;
}

bool RouteManager::addWaypoint(const QString &routeId, const Waypoint &wp)
{
    auto *r = route(routeId);
    if (!r)
        return false;
    r->waypoints.append(wp);
    emit routeUpdated(routeId);
    emit stateChanged();
    return true;
}

bool RouteManager::removeWaypoint(const QString &routeId, int index)
{
    auto *r = route(routeId);
    if (!r || index < 0 || index >= r->waypoints.size())
        return false;
    r->waypoints.removeAt(index);
    emit routeUpdated(routeId);
    emit stateChanged();
    return true;
}

bool RouteManager::moveWaypoint(const QString &routeId, int index,
                                double lat, double lon, double alt)
{
    auto *r = route(routeId);
    if (!r || index < 0 || index >= r->waypoints.size())
        return false;
    r->waypoints[index].lat = lat;
    r->waypoints[index].lon = lon;
    r->waypoints[index].alt = alt;
    emit routeUpdated(routeId);
    emit stateChanged();
    return true;
}
