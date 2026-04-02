#include "CesiumBridge.h"
#include "StateSerializer.h"
#include "EventParser.h"
#include "app/AppState.h"

#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

CesiumBridge::CesiumBridge(AppState *appState, QObject *parent)
    : QObject(parent)
    , m_appState(appState)
    , m_channel(new QWebChannel(this))
{
    // Register this object so JS can call slots and receive signals
    m_channel->registerObject(QStringLiteral("qtBridge"), this);
}

void CesiumBridge::attachToPage(QWebEnginePage *page)
{
    page->setWebChannel(m_channel);

    // Inject qwebchannel.js before any page script runs.
    // This is necessary because qrc:// URIs don't resolve when
    // the page is loaded from http:// (Vite dev server).
    QFile f(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
    if (f.open(QIODevice::ReadOnly)) {
        QWebEngineScript script;
        script.setName(QStringLiteral("qwebchannel"));
        script.setSourceCode(QString::fromUtf8(f.readAll()));
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setWorldId(QWebEngineScript::MainWorld);
        script.setRunsOnSubFrames(false);
        page->scripts().insert(script);
        qDebug() << "[CesiumBridge] qwebchannel.js injected via QWebEngineScript";
    } else {
        qWarning() << "[CesiumBridge] Failed to load qrc:///qtwebchannel/qwebchannel.js";
    }
}

void CesiumBridge::pushFullSync()
{
    if (!m_cesiumReady) return;

    QJsonObject msg = StateSerializer::serializeFullState(m_appState, m_tickCounter);
    sendJson(msg);
    emit logMessage(QStringLiteral("[Bridge] Full sync pushed (tick %1)").arg(m_tickCounter));
}

void CesiumBridge::pushDelta()
{
    if (!m_cesiumReady) return;

    ++m_tickCounter;
    QJsonObject msg = StateSerializer::serializeDelta(m_appState, m_tickCounter);
    sendJson(msg);
}

void CesiumBridge::pushCommand(const QJsonObject &cmd)
{
    if (!m_cesiumReady) return;
    sendJson(cmd);
    emit logMessage(QStringLiteral("[Bridge] Command: %1").arg(cmd.value(QStringLiteral("type")).toString()));
}

void CesiumBridge::onCesiumEvent(const QString &jsonStr)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[Bridge] Invalid JSON from Cesium:" << err.errorString();
        return;
    }

    QJsonObject event = doc.object();
    emit logMessage(QStringLiteral("[Bridge←Cesium] %1").arg(event.value(QStringLiteral("type")).toString()));
    handleEvent(event);
}

void CesiumBridge::onCesiumReady()
{
    m_cesiumReady = true;
    emit cesiumReady();
    emit logMessage(QStringLiteral("[Bridge] Cesium viewer ready"));

    // Send initial full state
    pushFullSync();
}

void CesiumBridge::handleEvent(const QJsonObject &event)
{
    const QString type = event.value(QStringLiteral("type")).toString();
    const QJsonObject payload = event.value(QStringLiteral("payload")).toObject();

    if (type == QLatin1String("EVT_ENTITY_CLICKED")) {
        const QString entityId = payload.value(QStringLiteral("entityId")).toString();
        const QString entityType = payload.value(QStringLiteral("entityType")).toString();
        emit entityClicked(entityId, entityType);

        // Auto-select in state
        if (entityType == QLatin1String("aircraft")) {
            m_appState->selectionManager()->selectEntity(entityId, SelectionType::AIRCRAFT);
            pushFullSync();
        }
    }
    else if (type == QLatin1String("EVT_MAP_CLICKED")) {
        const double lat = payload.value(QStringLiteral("lat")).toDouble();
        const double lon = payload.value(QStringLiteral("lon")).toDouble();
        const double alt = payload.value(QStringLiteral("alt")).toDouble();
        emit mapClicked(lat, lon, alt);
    }
    else if (type == QLatin1String("EVT_WAYPOINT_MOVED")) {
        const QString routeId = payload.value(QStringLiteral("routeId")).toString();
        const int wpIdx = payload.value(QStringLiteral("waypointIndex")).toInt();
        const QJsonObject pos = payload.value(QStringLiteral("newPosition")).toObject();
        const double lat = pos.value(QStringLiteral("lat")).toDouble();
        const double lon = pos.value(QStringLiteral("lon")).toDouble();
        const double alt = pos.value(QStringLiteral("alt")).toDouble();

        // Apply to authoritative state
        m_appState->routeManager()->moveWaypoint(routeId, wpIdx, lat, lon, alt);
        emit waypointMoved(routeId, wpIdx, lat, lon, alt);
        pushFullSync();
    }
    else if (type == QLatin1String("EVT_WAYPOINT_ADDED")) {
        const double lat = payload.value(QStringLiteral("lat")).toDouble();
        const double lon = payload.value(QStringLiteral("lon")).toDouble();
        const double alt = payload.value(QStringLiteral("alt")).toDouble();
        emit waypointAdded(lat, lon, alt);
    }
    else if (type == QLatin1String("REQ_FULL_SYNC")) {
        pushFullSync();
    }
}

void CesiumBridge::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    emit sendToCesium(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}
