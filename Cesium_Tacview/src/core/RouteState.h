#ifndef ROUTESTATE_H
#define ROUTESTATE_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>

struct Waypoint {
    double  lat  = 0.0;
    double  lon  = 0.0;
    double  alt  = 300.0;
    QString name;
    double  speedOverride = -1.0; // negative = use aircraft speed

    QJsonObject toJson() const {
        QJsonObject o;
        o[QStringLiteral("lat")]  = lat;
        o[QStringLiteral("lon")]  = lon;
        o[QStringLiteral("alt")]  = alt;
        o[QStringLiteral("name")] = name;
        if (speedOverride >= 0)
            o[QStringLiteral("speedOverride")] = speedOverride;
        return o;
    }

    static Waypoint fromJson(const QJsonObject &o) {
        Waypoint w;
        w.lat  = o.value(QStringLiteral("lat")).toDouble();
        w.lon  = o.value(QStringLiteral("lon")).toDouble();
        w.alt  = o.value(QStringLiteral("alt")).toDouble(300.0);
        w.name = o.value(QStringLiteral("name")).toString();
        w.speedOverride = o.value(QStringLiteral("speedOverride")).toDouble(-1.0);
        return w;
    }
};

struct RouteState {
    QString          id;
    QString          aircraftId;
    QVector<Waypoint> waypoints;
    QColor           color     = Qt::yellow;
    bool             visible   = true;
    bool             loopMode  = false;
    int              currentWaypointIndex = 0;

    QJsonObject toJson() const {
        QJsonObject o;
        o[QStringLiteral("id")]         = id;
        o[QStringLiteral("aircraftId")] = aircraftId;
        o[QStringLiteral("visible")]    = visible;
        o[QStringLiteral("loopMode")]   = loopMode;
        o[QStringLiteral("color")]      = color.name();
        o[QStringLiteral("currentWaypointIndex")] = currentWaypointIndex;

        QJsonArray wpArr;
        for (const auto &wp : waypoints)
            wpArr.append(wp.toJson());
        o[QStringLiteral("waypoints")] = wpArr;
        return o;
    }
};

#endif // ROUTESTATE_H
