#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

class AppState;
class CesiumBridge;
class SimulationEngine;
class InputManager;
class MapWindow;
class ViteProcess;
class AircraftListPanel;
class AircraftInspector;
class RouteEditorPanel;
class SimulationLogPanel;
class CommandHistoryPanel;
class PlaybackControlPanel;
class TelemetryPanel;
class ScenarioManager;
class TrackRecorder;
class PlaybackEngine;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    AppState *appState() { return m_appState; }
    CesiumBridge *bridge() { return m_bridge; }
    SimulationEngine *simEngine() { return m_simEngine; }
    MapWindow *mapWindow() { return m_mapWindow; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onCesiumReady();
    void onTickCompleted(quint64 tick);
    void onCreateAircraft();
    void onDeleteAircraft(const QString &id);
    void onMapClicked(double lat, double lon, double alt);
    void onSaveScenario();
    void onLoadScenario();
    void onExportRecording();
    void onImportRecording();

private:
    void setupUi();
    void setupConnections();
    void setupDualScreen();
    void createDemonstrationScenario();
    void onViteReady();
    void onViteError(const QString &msg);

    // Core
    AppState *m_appState;
    CesiumBridge *m_bridge;
    SimulationEngine *m_simEngine;
    QThread m_simThread;
    InputManager *m_inputManager;
    ViteProcess *m_viteProcess;
    ScenarioManager *m_scenarioManager;

    // Map (separate window on second screen)
    MapWindow *m_mapWindow;

    // Panels
    AircraftListPanel *m_aircraftPanel;
    AircraftInspector *m_inspectorPanel;
    RouteEditorPanel *m_routePanel;
    SimulationLogPanel *m_logPanel;
    CommandHistoryPanel *m_cmdHistoryPanel;
    PlaybackControlPanel *m_playbackPanel;
    TelemetryPanel *m_telemetryPanel;

    int m_nextAircraftId = 1;
};

#endif // MAINWINDOW_H
