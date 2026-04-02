#ifndef MAPTOOLBAR_H
#define MAPTOOLBAR_H

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

class AppState;
class CesiumBridge;

// Floating overlay toolbar on the map window — user-friendly waypoint/route controls
class MapToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit MapToolbar(AppState *appState, CesiumBridge *bridge, QWidget *parent);

    bool isWaypointMode() const { return m_waypointMode; }

signals:
    void logMessage(const QString &msg);

public slots:
    void refresh();
    void onMapClicked(double lat, double lon, double alt);

private slots:
    void toggleWaypointMode();
    void onNewRoute();
    void onAssignRoute();
    void onClearRoute();
    void onStartAircraft();
    void onStopAircraft();

private:
    void updateButtonStates();
    QString ensureActiveRoute();

    AppState *m_appState;
    CesiumBridge *m_bridge;

    // Toolbar buttons
    QPushButton *m_wpModeBtn;
    QPushButton *m_newRouteBtn;
    QPushButton *m_assignBtn;
    QPushButton *m_clearBtn;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;

    // Info
    QComboBox *m_routeCombo;
    QComboBox *m_aircraftCombo;
    QLabel *m_statusLabel;

    bool m_waypointMode = false;
};

#endif // MAPTOOLBAR_H
