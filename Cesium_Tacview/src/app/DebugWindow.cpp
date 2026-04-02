#include "DebugWindow.h"
#include "app/AppState.h"
#include "ui/AircraftInspector.h"
#include "ui/SimulationLogPanel.h"
#include "ui/CommandHistoryPanel.h"

#include <QLabel>

DebugWindow::DebugWindow(AppState *appState, QWidget *parent)
    : QMainWindow(parent)
    , m_appState(appState)
{
    setWindowTitle(QStringLiteral("Cesium Tacview — Debug Console"));
    resize(800, 600);

    // Central placeholder
    auto *central = new QLabel(QStringLiteral("Debug & Command Console"));
    central->setAlignment(Qt::AlignCenter);
    setCentralWidget(central);

    m_inspector = new AircraftInspector(appState, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_inspector);

    m_logPanel = new SimulationLogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);

    m_cmdPanel = new CommandHistoryPanel(this);
    addDockWidget(Qt::RightDockWidgetArea, m_cmdPanel);
}
