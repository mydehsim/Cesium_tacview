#ifndef AIRCRAFTSTATE_H
#define AIRCRAFTSTATE_H

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <array>

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
    double heading = 0.0;       // degrees [0,360) true heading (nose direction)
    double speed = 0.0;         // m/s (true airspeed)
    double verticalSpeed = 0.0; // m/s (positive up)
    double roll = 0.0;          // degrees (bank angle, +right wing down)
    double pitch = 0.0;         // degrees (flight path angle, +nose up)
    double yaw = 0.0;           // degrees (= heading for coordinated flight)
    ControlMode controlMode = ControlMode::IDLE;
    QString currentRouteId;
    QString modelUri = QStringLiteral("/models/f16-c_falcon.glb");
    bool visible = true;

    // Derived telemetry (computed by KinematicModel)
    double magneticHeading = 0.0; // degrees [0,360) magnetic heading
    double groundTrack = 0.0;     // degrees [0,360) actual direction of movement
    double groundSpeed = 0.0;     // m/s speed over ground
    double turnRate = 0.0;        // degrees/sec (positive = turning right)
    double gLoad = 1.0;           // g-force (1.0 = level flight)

    // Targets for ManualController
    double targetHeading = 0.0;
    double targetSpeed = 0.0;
    double targetAlt = 0.0;

    // Trail — O(1) ring buffer for path rendering
    struct TrailPoint
    {
        double lat;
        double lon;
        double alt;
    };
    static constexpr int MAX_TRAIL = 500;

    void addTrailPoint()
    {
        m_trailBuf[m_trailHead] = {lat, lon, alt};
        m_trailHead = (m_trailHead + 1) % MAX_TRAIL;
        if (m_trailCount < MAX_TRAIL)
            ++m_trailCount;
    }

    int trailCount() const { return m_trailCount; }

    void clearTrail()
    {
        m_trailHead = 0;
        m_trailCount = 0;
    }

    // Dirty flag for delta serialization — set by physics, cleared by serializer
    bool dirty = false;

private:
    std::array<TrailPoint, MAX_TRAIL> m_trailBuf{};
    int m_trailHead = 0;
    int m_trailCount = 0;

public:
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
        o[QStringLiteral("magneticHeading")] = magneticHeading;
        o[QStringLiteral("groundTrack")] = groundTrack;
        o[QStringLiteral("groundSpeed")] = groundSpeed;
        o[QStringLiteral("turnRate")] = turnRate;
        o[QStringLiteral("gLoad")] = gLoad;
        o[QStringLiteral("controlMode")] = controlModeToString(controlMode);
        o[QStringLiteral("currentRouteId")] = currentRouteId;
        o[QStringLiteral("modelUri")] = modelUri;
        o[QStringLiteral("visible")] = visible;
        return o;
    }

    // Slim delta for Cesium rendering — only fields needed for 3D display
    QJsonObject toRenderDelta() const
    {
        QJsonObject o;
        o[QStringLiteral("lat")] = lat;
        o[QStringLiteral("lon")] = lon;
        o[QStringLiteral("alt")] = alt;
        o[QStringLiteral("heading")] = heading;
        o[QStringLiteral("speed")] = speed;
        o[QStringLiteral("verticalSpeed")] = verticalSpeed;
        o[QStringLiteral("roll")] = roll;
        o[QStringLiteral("pitch")] = pitch;
        return o;
    }

    // Full delta with telemetry — backward compatible with old toDeltaJson()
    QJsonObject toDeltaJson() const
    {
        QJsonObject o = toRenderDelta();
        o[QStringLiteral("magneticHeading")] = magneticHeading;
        o[QStringLiteral("groundTrack")] = groundTrack;
        o[QStringLiteral("groundSpeed")] = groundSpeed;
        o[QStringLiteral("turnRate")] = turnRate;
        o[QStringLiteral("gLoad")] = gLoad;
        return o;
    }

    // JSON for scenario file save/load
    static AircraftState fromJson(const QJsonObject &o)
    {
        AircraftState ac;
        ac.id = o.value(QStringLiteral("id")).toString();
        ac.callSign = o.value(QStringLiteral("callSign")).toString();
        ac.type = o.value(QStringLiteral("type")).toString(QStringLiteral("Fighter"));
        ac.lat = o.value(QStringLiteral("lat")).toDouble();
        ac.lon = o.value(QStringLiteral("lon")).toDouble();
        ac.alt = o.value(QStringLiteral("alt")).toDouble();
        ac.heading = o.value(QStringLiteral("heading")).toDouble();
        ac.speed = o.value(QStringLiteral("speed")).toDouble();
        ac.verticalSpeed = o.value(QStringLiteral("verticalSpeed")).toDouble();
        ac.roll = o.value(QStringLiteral("roll")).toDouble();
        ac.pitch = o.value(QStringLiteral("pitch")).toDouble();
        ac.modelUri = o.value(QStringLiteral("modelUri")).toString(QStringLiteral("/models/f16-c_falcon.glb"));
        ac.visible = o.value(QStringLiteral("visible")).toBool(true);
        ac.currentRouteId = o.value(QStringLiteral("currentRouteId")).toString();
        ac.targetHeading = ac.heading;
        ac.targetSpeed = ac.speed;
        ac.targetAlt = ac.alt;
        const QString modeStr = o.value(QStringLiteral("controlMode")).toString(QStringLiteral("IDLE"));
        if (modeStr == QLatin1String("MANUAL"))
            ac.controlMode = ControlMode::MANUAL;
        else if (modeStr == QLatin1String("AUTOPILOT"))
            ac.controlMode = ControlMode::AUTOPILOT;
        else if (modeStr == QLatin1String("SCRIPTED"))
            ac.controlMode = ControlMode::SCRIPTED;
        else
            ac.controlMode = ControlMode::IDLE;
        return ac;
    }
};

#endif // AIRCRAFTSTATE_H
