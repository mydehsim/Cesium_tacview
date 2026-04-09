#include "ScenarioManager.h"
#include "app/AppState.h"
#include "simulation/TrackRecorder.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFileInfo>

ScenarioManager::ScenarioManager(QObject *parent)
    : QObject(parent)
{
}

bool ScenarioManager::saveScenario(const QString &filePath, const AppState *state,
                                   const TrackRecorder *recorder)
{
    QJsonObject root;
    root[QStringLiteral("version")] = QStringLiteral("1.0");
    root[QStringLiteral("name")] = QFileInfo(filePath).baseName();
    root[QStringLiteral("created")] = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Aircraft
    QJsonArray acArr;
    const auto &allAc = state->aircraftManager()->allAircraft();
    for (auto it = allAc.cbegin(); it != allAc.cend(); ++it)
    {
        acArr.append(it.value().toJson());
    }
    root[QStringLiteral("aircraft")] = acArr;

    // Routes
    QJsonArray rtArr;
    const auto &allRt = state->routeManager()->allRoutes();
    for (auto it = allRt.cbegin(); it != allRt.cend(); ++it)
    {
        rtArr.append(it.value().toJson());
    }
    root[QStringLiteral("routes")] = rtArr;

    // Recording (if provided and has data)
    if (recorder && recorder->snapshotCount() > 0)
    {
        QJsonArray recArr;
        for (const auto &snap : recorder->snapshots())
        {
            QJsonObject snapObj;
            snapObj[QStringLiteral("tick")] = static_cast<qint64>(snap.tick);
            snapObj[QStringLiteral("timestamp")] = snap.timestamp;

            QJsonObject acStates;
            for (auto it = snap.aircraftStates.cbegin(); it != snap.aircraftStates.cend(); ++it)
            {
                const auto &s = it.value();
                QJsonObject o;
                o[QStringLiteral("lat")] = s.lat;
                o[QStringLiteral("lon")] = s.lon;
                o[QStringLiteral("alt")] = s.alt;
                o[QStringLiteral("heading")] = s.heading;
                o[QStringLiteral("speed")] = s.speed;
                o[QStringLiteral("verticalSpeed")] = s.verticalSpeed;
                o[QStringLiteral("roll")] = s.roll;
                o[QStringLiteral("pitch")] = s.pitch;
                o[QStringLiteral("magneticHeading")] = s.magneticHeading;
                o[QStringLiteral("groundTrack")] = s.groundTrack;
                o[QStringLiteral("groundSpeed")] = s.groundSpeed;
                o[QStringLiteral("turnRate")] = s.turnRate;
                o[QStringLiteral("gLoad")] = s.gLoad;
                acStates[it.key()] = o;
            }
            snapObj[QStringLiteral("aircraft")] = acStates;
            recArr.append(snapObj);
        }
        root[QStringLiteral("recording")] = recArr;
    }

    // Write file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_lastError = QStringLiteral("Cannot open file for writing: %1").arg(filePath);
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    emit logMessage(QStringLiteral("[Scenario] Saved: %1").arg(filePath));
    return true;
}

bool ScenarioManager::loadScenario(const QString &filePath, AppState *state)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_lastError = QStringLiteral("Cannot open file: %1").arg(filePath);
        return false;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    file.close();

    if (parseErr.error != QJsonParseError::NoError)
    {
        m_lastError = QStringLiteral("JSON parse error: %1").arg(parseErr.errorString());
        return false;
    }

    QJsonObject root = doc.object();

    // Clear existing state
    for (const QString &id : state->aircraftManager()->aircraftIds())
        state->aircraftManager()->removeAircraft(id);
    for (const QString &id : state->routeManager()->routeIds())
        state->routeManager()->removeRoute(id);
    state->selectionManager()->clearSelection();

    // Load aircraft
    const QJsonArray acArr = root.value(QStringLiteral("aircraft")).toArray();
    for (const QJsonValue &val : acArr)
    {
        AircraftState ac = AircraftState::fromJson(val.toObject());
        state->aircraftManager()->createAircraft(ac);
    }

    // Load routes
    const QJsonArray rtArr = root.value(QStringLiteral("routes")).toArray();
    for (const QJsonValue &val : rtArr)
    {
        QJsonObject rtObj = val.toObject();
        RouteState route;
        route.id = rtObj.value(QStringLiteral("id")).toString();
        route.aircraftId = rtObj.value(QStringLiteral("aircraftId")).toString();
        route.loopMode = rtObj.value(QStringLiteral("loopMode")).toBool();
        route.visible = rtObj.value(QStringLiteral("visible")).toBool(true);
        route.currentWaypointIndex = rtObj.value(QStringLiteral("currentWaypointIndex")).toInt();

        const QString colorStr = rtObj.value(QStringLiteral("color")).toString();
        if (!colorStr.isEmpty())
            route.color = QColor(colorStr);

        const QJsonArray wpArr = rtObj.value(QStringLiteral("waypoints")).toArray();
        for (const QJsonValue &wpVal : wpArr)
        {
            route.waypoints.append(Waypoint::fromJson(wpVal.toObject()));
        }

        state->routeManager()->createRoute(route);
    }

    emit logMessage(QStringLiteral("[Scenario] Loaded: %1 (%2 aircraft, %3 routes)")
                        .arg(filePath)
                        .arg(acArr.size())
                        .arg(rtArr.size()));
    return true;
}

bool ScenarioManager::exportRecording(const QString &filePath, const TrackRecorder *recorder)
{
    if (!recorder || recorder->snapshotCount() == 0)
    {
        m_lastError = QStringLiteral("No recording data to export");
        return false;
    }

    QJsonObject root;
    root[QStringLiteral("version")] = QStringLiteral("1.0");
    root[QStringLiteral("type")] = QStringLiteral("recording");
    root[QStringLiteral("snapshotCount")] = recorder->snapshotCount();

    QJsonArray recArr;
    for (const auto &snap : recorder->snapshots())
    {
        QJsonObject snapObj;
        snapObj[QStringLiteral("tick")] = static_cast<qint64>(snap.tick);
        snapObj[QStringLiteral("timestamp")] = snap.timestamp;

        QJsonObject acStates;
        for (auto it = snap.aircraftStates.cbegin(); it != snap.aircraftStates.cend(); ++it)
        {
            const auto &s = it.value();
            QJsonObject o;
            o[QStringLiteral("lat")] = s.lat;
            o[QStringLiteral("lon")] = s.lon;
            o[QStringLiteral("alt")] = s.alt;
            o[QStringLiteral("heading")] = s.heading;
            o[QStringLiteral("speed")] = s.speed;
            o[QStringLiteral("verticalSpeed")] = s.verticalSpeed;
            o[QStringLiteral("roll")] = s.roll;
            o[QStringLiteral("pitch")] = s.pitch;
            o[QStringLiteral("magneticHeading")] = s.magneticHeading;
            o[QStringLiteral("groundTrack")] = s.groundTrack;
            o[QStringLiteral("groundSpeed")] = s.groundSpeed;
            o[QStringLiteral("turnRate")] = s.turnRate;
            o[QStringLiteral("gLoad")] = s.gLoad;
            acStates[it.key()] = o;
        }
        snapObj[QStringLiteral("aircraft")] = acStates;
        recArr.append(snapObj);
    }
    root[QStringLiteral("recording")] = recArr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_lastError = QStringLiteral("Cannot open file for writing: %1").arg(filePath);
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.close();

    emit logMessage(QStringLiteral("[Scenario] Recording exported: %1 (%2 snapshots)")
                        .arg(filePath)
                        .arg(recorder->snapshotCount()));
    return true;
}

bool ScenarioManager::importRecording(const QString &filePath, TrackRecorder *recorder)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_lastError = QStringLiteral("Cannot open file: %1").arg(filePath);
        return false;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    file.close();

    if (parseErr.error != QJsonParseError::NoError)
    {
        m_lastError = QStringLiteral("JSON parse error: %1").arg(parseErr.errorString());
        return false;
    }

    QJsonObject root = doc.object();
    const QJsonArray recArr = root.value(QStringLiteral("recording")).toArray();

    recorder->clear();
    auto &snapshots = recorder->snapshots();

    for (const QJsonValue &val : recArr)
    {
        QJsonObject snapObj = val.toObject();
        TickSnapshot snap;
        snap.tick = static_cast<quint64>(snapObj.value(QStringLiteral("tick")).toInteger());
        snap.timestamp = snapObj.value(QStringLiteral("timestamp")).toInteger();

        const QJsonObject acStates = snapObj.value(QStringLiteral("aircraft")).toObject();
        for (auto it = acStates.constBegin(); it != acStates.constEnd(); ++it)
        {
            QJsonObject o = it.value().toObject();
            CompactAircraftSnapshot cs;
            cs.lat = o.value(QStringLiteral("lat")).toDouble();
            cs.lon = o.value(QStringLiteral("lon")).toDouble();
            cs.alt = o.value(QStringLiteral("alt")).toDouble();
            cs.heading = o.value(QStringLiteral("heading")).toDouble();
            cs.speed = o.value(QStringLiteral("speed")).toDouble();
            cs.verticalSpeed = o.value(QStringLiteral("verticalSpeed")).toDouble();
            cs.roll = o.value(QStringLiteral("roll")).toDouble();
            cs.pitch = o.value(QStringLiteral("pitch")).toDouble();
            cs.magneticHeading = o.value(QStringLiteral("magneticHeading")).toDouble();
            cs.groundTrack = o.value(QStringLiteral("groundTrack")).toDouble();
            cs.groundSpeed = o.value(QStringLiteral("groundSpeed")).toDouble();
            cs.turnRate = o.value(QStringLiteral("turnRate")).toDouble();
            cs.gLoad = o.value(QStringLiteral("gLoad")).toDouble();
            snap.aircraftStates.insert(it.key(), cs);
        }

        snapshots.append(std::move(snap));
    }

    emit logMessage(QStringLiteral("[Scenario] Recording imported: %1 (%2 snapshots)")
                        .arg(filePath)
                        .arg(snapshots.size()));
    return true;
}
