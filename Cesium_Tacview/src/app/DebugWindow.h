#ifndef DEBUGWINDOW_H
#define DEBUGWINDOW_H

#include <QMainWindow>

class AppState;
class AircraftInspector;
class SimulationLogPanel;
class CommandHistoryPanel;

class DebugWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit DebugWindow(AppState *appState, QWidget *parent = nullptr);

    SimulationLogPanel  *logPanel()  { return m_logPanel; }
    CommandHistoryPanel *cmdPanel()  { return m_cmdPanel; }
    AircraftInspector   *inspector() { return m_inspector; }

private:
    AppState            *m_appState;
    AircraftInspector   *m_inspector;
    SimulationLogPanel  *m_logPanel;
    CommandHistoryPanel *m_cmdPanel;
};

#endif // DEBUGWINDOW_H
