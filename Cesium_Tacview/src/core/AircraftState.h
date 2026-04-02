#ifndef AIRCRAFTSTATE_H
#define AIRCRAFTSTATE_H

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

enum class ControlMode
{
    IDLE,
    MANUAL,
    AUTOPILOT,
    SCRIPTED
};

inline QString controlModeToString(ControlMode m)
{
    switch (m)
    {
    case ControlMode::IDLE:
        return QStringLiteral("IDLE");
    case ControlMode::MANUAL:
        return QStringLiteral("MANUAL");
    case ControlMode::AUTOPILOT:
        return QStringLiteral("AUTOPILOT");
    case ControlMode::SCRIPTED:
        return QStringLiteral("SCRIPTED");
    }
    return QStringLiteral("IDLE");
}

struct AircraftState
{
    QString id;
    QString callSign;
    QString type = QStringLiteral("Fighter");
    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    double heading = 0.0;       // degrees
    double speed = 0.0;         // m/s
    double verticalSpeed = 0.0; // m/s
    double roll = 0.0;          // degrees (bank angle, +right)
    double pitch = 0.0;         // degrees (nose up positive)
    double yaw = 0.0;           // degrees (same as heading for now)
    ControlMode controlMode = ControlMode::IDLE;
    QString currentRouteId;
    QString modelUri = QStringLiteral("/models/f16-c_falcon.glb");
    bool visible = true;

    // Targets for ManualController
    double targetHeading = 0.0;
    double targetSpeed = 0.0;
    double targetAlt = 0.0;

    // Trail (last N positions for path rendering)
    struct TrailPoint
    {
        double lat;
        double lon;
        double alt;
    };
    QVector<TrailPoint> trail;
    static constexpr int MAX_TRAIL = 500;

    void addTrailPoint()
    {
        trail.append({lat, lon, alt});
        if (trail.size() > MAX_TRAIL)
            trail.removeFirst();
    }

    QJsonObject toJson() const
    {
        QJsonObject o;
        o[QStringLiteral("id")] = id;
        o[QStringLiteral("callSign")] = callSign;
        o[QStringLiteral("type")] = type;
        o[QStringLiteral("lat")] = lat;
        o[QStringLiteral("lon")] = lon;
        o[QStringLiteral("alt")] = alt;
        o[QStringLiteral("heading")] = heading;
        o[QStringLiteral("speed")] = speed;
        o[QStringLiteral("verticalSpeed")] = verticalSpeed;
        o[QStringLiteral("roll")] = roll;
        o[QStringLiteral("pitch")] = pitch;
        o[QStringLiteral("yaw")] = yaw;
        o[QStringLiteral("controlMode")] = controlModeToString(controlMode);
        o[QStringLiteral("currentRouteId")] = currentRouteId;
        o[QStringLiteral("modelUri")] = modelUri;
        o[QStringLiteral("visible")] = visible;
        return o;
    }

    QJsonObject toDeltaJson() const
    {
        QJsonObject o;
        o[QStringLiteral("lat")] = lat;
        o[QStringLiteral("lon")] = lon;
        o[QStringLiteral("alt")] = alt;
        o[QStringLiteral("heading")] = heading;
        o[QStringLiteral("speed")] = speed;
        o[QStringLiteral("roll")] = roll;
        o[QStringLiteral("pitch")] = pitch;
        o[QStringLiteral("yaw")] = yaw;
        return o;
    }
};

#endif // AIRCRAFTSTATE_H
