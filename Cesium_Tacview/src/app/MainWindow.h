#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QWebEngineView;
class AppState;
class CesiumBridge;
class SimulationEngine;
class InputManager;
class AircraftListPanel;
class AircraftInspector;
class RouteEditorPanel;
class SimulationLogPanel;
class CommandHistoryPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    AppState         *appState()  { return m_appState; }
    CesiumBridge     *bridge()    { return m_bridge; }
    SimulationEngine *simEngine() { return m_simEngine; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onCesiumReady();
    void onTickCompleted(quint64 tick);
    void onCreateAircraft();
    void onDeleteAircraft(const QString &id);

private:
    void setupUi();
    void setupConnections();
    void createDemonstrationScenario();

    // Core
    AppState         *m_appState;
    CesiumBridge     *m_bridge;
    SimulationEngine *m_simEngine;
    InputManager     *m_inputManager;

    // Widgets
    QWebEngineView   *m_webView;

    // Panels
    AircraftListPanel   *m_aircraftPanel;
    AircraftInspector   *m_inspectorPanel;
    RouteEditorPanel    *m_routePanel;
    SimulationLogPanel  *m_logPanel;
    CommandHistoryPanel *m_cmdHistoryPanel;

    int m_nextAircraftId = 1;
};

#endif // MAINWINDOW_H
