#ifndef CESIUMBRIDGE_H
#define CESIUMBRIDGE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>

class AppState;
class QWebChannel;
class QWebEnginePage;

class CesiumBridge : public QObject
{
    Q_OBJECT
public:
    explicit CesiumBridge(AppState *appState, QObject *parent = nullptr);

    void attachToPage(QWebEnginePage *page);

    // Push state to Cesium (called after state changes)
    void pushFullSync();
    void pushDelta();
    void pushCommand(const QJsonObject &cmd);

public slots:
    // Called from JavaScript via QWebChannel
    void onCesiumEvent(const QString &jsonStr);
    void onCesiumReady();

signals:
    // Signal to JS (connected via QWebChannel property)
    void sendToCesium(const QString &jsonStr);

    // Internal signals for Qt UI
    void entityClicked(const QString &entityId, const QString &entityType);
    void entityCtrlClicked(const QString &entityId, const QString &entityType);
    void mapClicked(double lat, double lon, double alt);
    void waypointMoved(const QString &routeId, int waypointIndex, double lat, double lon, double alt);
    void waypointAdded(double lat, double lon, double alt);
    void cesiumReady();
    void logMessage(const QString &msg);

private:
    void handleEvent(const QJsonObject &event);
    void sendJson(const QJsonObject &obj);

    AppState *m_appState;
    QWebChannel *m_channel;
    bool m_cesiumReady = false;
    quint64 m_tickCounter = 0;
};

#endif // CESIUMBRIDGE_H
