#include "MainWindow.h"
#include "MapWindow.h"
#include "ViteProcess.h"
#include "AppState.h"
#include "bridge/CesiumBridge.h"
#include "bridge/StateSerializer.h"
#include "simulation/SimulationEngine.h"
#include "input/InputManager.h"
#include "ui/AircraftListPanel.h"
#include "ui/AircraftInspector.h"
#include "ui/RouteEditorPanel.h"
#include "ui/SimulationLogPanel.h"
#include "ui/CommandHistoryPanel.h"
#include "ui/MapToolbar.h"
#include "core/RouteState.h"

#include <QKeyEvent>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QTabWidget>
#include <QApplication>
#include <QScreen>
#include <QLabel>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Core systems
    m_appState = new AppState(this);
    m_bridge = new CesiumBridge(m_appState, this);
    m_simEngine = new SimulationEngine(m_appState, this);
    m_inputManager = new InputManager(m_appState, m_simEngine, m_bridge, this);

    // Vite dev server — auto-start before loading Cesium
    QString webDir = QStringLiteral(WEB_CONTENT_PATH);
    m_viteProcess = new ViteProcess(webDir, this);

    // Map window (separate, goes to second screen)
    m_mapWindow = new MapWindow(m_appState, m_bridge, nullptr);

    setupUi();
    setupConnections();
    setupDualScreen();

    // Start Vite — when ready, loadCesium() is called via onViteReady()
    m_viteProcess->start();

    setWindowTitle(QStringLiteral("Cesium Tacview — Command & Control"));
    resize(1200, 900);
}

MainWindow::~MainWindow()
{
    // Stop Vite dev server
    if (m_viteProcess)
        m_viteProcess->stop();

    // MapWindow is not a child (nullptr parent), delete explicitly
    delete m_mapWindow;
}

void MainWindow::setupUi()
{
    // ============================================================
    // Central Widget: Splitter with panels (NO QWebEngineView here)
    // ============================================================
    auto *centralSplitter = new QSplitter(Qt::Horizontal);

    // Left column: Aircraft list + Route editor (stacked)
    auto *leftSplitter = new QSplitter(Qt::Vertical);

    m_aircraftPanel = new AircraftListPanel(m_appState, m_bridge, this);
    m_inspectorPanel = new AircraftInspector(m_appState, this);
    m_routePanel = new RouteEditorPanel(m_appState, m_bridge, this);

    leftSplitter->addWidget(m_aircraftPanel);
    leftSplitter->addWidget(m_inspectorPanel);
    leftSplitter->addWidget(m_routePanel);
    leftSplitter->setStretchFactor(0, 3);
    leftSplitter->setStretchFactor(1, 2);
    leftSplitter->setStretchFactor(2, 2);

    // Right column: Log + command history (tabbed)
    auto *rightSplitter = new QSplitter(Qt::Vertical);

    auto *logTabs = new QTabWidget;
    m_logPanel = new SimulationLogPanel(this);
    m_cmdHistoryPanel = new CommandHistoryPanel(this);
    logTabs->addTab(m_logPanel, QStringLiteral("Simulation Log"));
    logTabs->addTab(m_cmdHistoryPanel, QStringLiteral("Command History"));

    // Placeholder for future panels (scenario, network log)
    auto *statusLabel = new QLabel(QStringLiteral(
        "<h2>Cesium Tacview</h2>"
        "<p>Command & Control Console</p>"
        "<hr>"
        "<p><b>Keyboard:</b></p>"
        "<p>W/A/S/D — Manual aircraft control</p>"
        "<p>Q/E — Climb / Descend</p>"
        "<p>Space — Toggle MANUAL/AUTOPILOT</p>"
        "<p>Tab — Next aircraft</p>"
        "<p>F — Camera follow</p>"
        "<p>P — Pause/Resume</p>"
        "<p>Ctrl+N — New aircraft</p>"
        "<p>Delete — Remove selected</p>"));
    statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    statusLabel->setMargin(12);
    statusLabel->setWordWrap(true);

    rightSplitter->addWidget(statusLabel);
    rightSplitter->addWidget(logTabs);
    rightSplitter->setStretchFactor(0, 1);
    rightSplitter->setStretchFactor(1, 2);

    centralSplitter->addWidget(leftSplitter);
    centralSplitter->addWidget(rightSplitter);
    centralSplitter->setStretchFactor(0, 1);
    centralSplitter->setStretchFactor(1, 1);

    setCentralWidget(centralSplitter);

    // --- Menu Bar ---
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    auto *simMenu = menuBar()->addMenu(tr("&Simulation"));
    simMenu->addAction(tr("&Start"), m_simEngine, &SimulationEngine::start);
    simMenu->addAction(tr("&Pause"), m_simEngine, &SimulationEngine::togglePause);
    simMenu->addAction(tr("Sto&p"), m_simEngine, &SimulationEngine::stop);

    auto *acMenu = menuBar()->addMenu(tr("&Aircraft"));
    acMenu->addAction(tr("&Create Aircraft"), this, &MainWindow::onCreateAircraft, QKeySequence(QStringLiteral("Ctrl+N")));

    auto *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(tr("Show &Map Window"), m_mapWindow, &QWidget::show);
    viewMenu->addAction(tr("Map &Fullscreen"), m_mapWindow, &QWidget::showFullScreen);

    // --- Toolbar ---
    auto *toolbar = addToolBar(tr("Simulation"));
    toolbar->addAction(tr("▶ Start"), m_simEngine, &SimulationEngine::start);
    toolbar->addAction(tr("⏸ Pause"), m_simEngine, &SimulationEngine::togglePause);
    toolbar->addAction(tr("⏹ Stop"), m_simEngine, &SimulationEngine::stop);
    toolbar->addSeparator();
    toolbar->addAction(tr("+ Aircraft"), this, &MainWindow::onCreateAircraft);

    // --- Status Bar ---
    statusBar()->showMessage(tr("Ready — Waiting for Cesium map..."));
}

void MainWindow::setupConnections()
{
    // Bridge events
    connect(m_bridge, &CesiumBridge::cesiumReady, this, &MainWindow::onCesiumReady);
    connect(m_bridge, &CesiumBridge::logMessage, m_logPanel, &SimulationLogPanel::appendLog);
    connect(m_bridge, &CesiumBridge::logMessage, m_cmdHistoryPanel, &CommandHistoryPanel::addCommand);

    connect(m_bridge, &CesiumBridge::entityClicked, this, [this](const QString &id, const QString &) {
        m_logPanel->appendLog(QStringLiteral("Entity clicked: %1").arg(id));
    });

    // Map click → add waypoint to active route
    connect(m_bridge, &CesiumBridge::mapClicked, this, &MainWindow::onMapClicked);

    // Simulation tick → push delta to Cesium
    connect(m_simEngine, &SimulationEngine::tickCompleted, this, &MainWindow::onTickCompleted);
    connect(m_simEngine, &SimulationEngine::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // Input manager logs
    connect(m_inputManager, &InputManager::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // Aircraft panel actions
    connect(m_aircraftPanel, &AircraftListPanel::createAircraftRequested, this, &MainWindow::onCreateAircraft);
    connect(m_aircraftPanel, &AircraftListPanel::deleteAircraftRequested, this, &MainWindow::onDeleteAircraft);

    // MapWindow log forwarding
    connect(m_mapWindow, &MapWindow::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // Vite process
    connect(m_viteProcess, &ViteProcess::ready, this, &MainWindow::onViteReady);
    connect(m_viteProcess, &ViteProcess::errorOccurred, this, &MainWindow::onViteError);
    connect(m_viteProcess, &ViteProcess::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // MapToolbar "Fly" pressed → ensure simulation engine is running
    connect(m_mapWindow->toolbar(), &MapToolbar::requestSimStart, this, [this]() {
        if (!m_simEngine->isRunning())
            m_simEngine->start();
    });
}

void MainWindow::setupDualScreen()
{
    const QList<QScreen *> screens = QApplication::screens();

    if (screens.size() >= 2) {
        // Two monitors: MainWindow on screen 0, MapWindow on screen 1
        const QRect screen0 = screens.at(0)->availableGeometry();
        setGeometry(screen0);
        showMaximized();

        m_mapWindow->moveToScreen(1);

        m_logPanel->appendLog(QStringLiteral("Dual-screen: Control on %1, Map on %2")
                                  .arg(screens.at(0)->name(), screens.at(1)->name()));
    } else {
        // Single monitor: side by side (60/40 split)
        const QRect avail = screens.at(0)->availableGeometry();
        const int splitX = avail.width() * 2 / 5;

        setGeometry(avail.x(), avail.y(), splitX, avail.height());
        show();

        m_mapWindow->setGeometry(avail.x() + splitX, avail.y(),
                                 avail.width() - splitX, avail.height());
        m_mapWindow->show();

        m_logPanel->appendLog(QStringLiteral("Single-screen: Side-by-side layout"));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Don't intercept if web view has focus for text input etc.
    if (!event->isAutoRepeat())
    {
        m_inputManager->keyPressed(event->key());
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat())
    {
        m_inputManager->keyReleased(event->key());
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::onCesiumReady()
{
    statusBar()->showMessage(tr("Cesium ready — Simulation idle"));
    m_logPanel->appendLog(QStringLiteral("Cesium viewer connected and ready"));

    createDemonstrationScenario();

    // Auto-start simulation so demo aircraft move immediately
    m_simEngine->start();

    // Refresh map toolbar so combos are populated with demo data
    m_mapWindow->toolbar()->refresh();
}

void MainWindow::onViteReady()
{
    m_logPanel->appendLog(QStringLiteral("Vite dev server ready — loading Cesium"));
    statusBar()->showMessage(tr("Vite ready — Loading Cesium..."));
    m_mapWindow->loadCesium();
}

void MainWindow::onViteError(const QString &msg)
{
    m_logPanel->appendLog(QStringLiteral("Vite ERROR: %1").arg(msg));
    statusBar()->showMessage(QStringLiteral("Vite Error: %1").arg(msg));
}

void MainWindow::onTickCompleted(quint64 tick)
{
    // Push delta state to Cesium every tick
    m_bridge->pushDelta();

    // Update inspector every 5 ticks (~4 times/sec)
    if (tick % 5 == 0)
    {
        m_inspectorPanel->refresh();
        statusBar()->showMessage(QStringLiteral("Tick: %1 | Aircraft: %2 | Selected: %3")
                                     .arg(tick)
                                     .arg(m_appState->aircraftManager()->count())
                                     .arg(m_appState->selectionManager()->selectedEntityId()));
    }
}

void MainWindow::onCreateAircraft()
{
    bool ok;
    QString callSign = QInputDialog::getText(this, tr("Create Aircraft"),
                                             tr("Call Sign:"), QLineEdit::Normal,
                                             QStringLiteral("ALPHA-%1").arg(m_nextAircraftId, 2, 10, QLatin1Char('0')),
                                             &ok);
    if (!ok || callSign.isEmpty())
        return;

    AircraftState ac;
    ac.id = QStringLiteral("ac_%1").arg(m_nextAircraftId++);
    ac.callSign = callSign;
    ac.type = QStringLiteral("Fighter");
    ac.lat = 41.0 + (m_nextAircraftId * 0.05);
    ac.lon = 29.0 + (m_nextAircraftId * 0.05);
    ac.alt = 5000;
    ac.heading = 0;
    ac.targetHeading = 0;
    ac.targetSpeed = 0;
    ac.targetAlt = 5000;
    ac.controlMode = ControlMode::IDLE;

    m_appState->aircraftManager()->createAircraft(ac);

    // Auto-select newly created aircraft
    m_appState->selectionManager()->selectEntity(ac.id, SelectionType::AIRCRAFT);
    m_bridge->pushFullSync();

    // Fly camera to new aircraft so it's visible
    QJsonObject flyPayload;
    flyPayload[QStringLiteral("lat")] = ac.lat;
    flyPayload[QStringLiteral("lon")] = ac.lon;
    flyPayload[QStringLiteral("alt")] = ac.alt + 5000;
    flyPayload[QStringLiteral("duration")] = 1.5;
    m_bridge->pushCommand(StateSerializer::createCommand(QStringLiteral("CMD_CAMERA_FLY_TO"), flyPayload));

    m_logPanel->appendLog(QStringLiteral("Created aircraft: %1 (%2)").arg(ac.id, ac.callSign));
}

void MainWindow::onDeleteAircraft(const QString &id)
{
    auto ret = QMessageBox::question(this, tr("Delete Aircraft"),
                                     tr("Delete aircraft %1?").arg(id));
    if (ret != QMessageBox::Yes)
        return;

    m_appState->aircraftManager()->removeAircraft(id);
    if (m_appState->selectionManager()->selectedEntityId() == id)
        m_appState->selectionManager()->clearSelection();
    m_bridge->pushFullSync();
    m_logPanel->appendLog(QStringLiteral("Deleted aircraft: %1").arg(id));
}

void MainWindow::onMapClicked(double lat, double lon, double /*alt*/)
{
    // Map clicks are now handled by MapToolbar on the map window.
    // This slot only logs.
    m_logPanel->appendLog(QStringLiteral("Map clicked: (%1, %2)")
                              .arg(lat, 0, 'f', 4)
                              .arg(lon, 0, 'f', 4));
}

void MainWindow::createDemonstrationScenario()
{
    // Create a couple of demo aircraft
    AircraftState ac1;
    ac1.id = QStringLiteral("ac_1");
    ac1.callSign = QStringLiteral("ALPHA-01");
    ac1.type = QStringLiteral("Fighter");
    ac1.lat = 41.0082;
    ac1.lon = 28.9784;
    ac1.alt = 5000;
    ac1.heading = 45;
    ac1.targetHeading = 45;
    ac1.speed = 100;
    ac1.targetSpeed = 100;
    ac1.targetAlt = 5000;
    ac1.controlMode = ControlMode::AUTOPILOT;
    ac1.currentRouteId = QStringLiteral("route_1");
    m_appState->aircraftManager()->createAircraft(ac1);

    AircraftState ac2;
    ac2.id = QStringLiteral("ac_2");
    ac2.callSign = QStringLiteral("BRAVO-02");
    ac2.type = QStringLiteral("Transport");
    ac2.lat = 40.9;
    ac2.lon = 28.8;
    ac2.alt = 3000;
    ac2.heading = 90;
    ac2.targetHeading = 90;
    ac2.speed = 80;
    ac2.targetSpeed = 80;
    ac2.targetAlt = 3000;
    ac2.modelUri = QStringLiteral("/models/aircraft.glb");
    ac2.controlMode = ControlMode::AUTOPILOT;
    ac2.currentRouteId = QStringLiteral("route_1");
    m_appState->aircraftManager()->createAircraft(ac2);

    // Create a demo route
    RouteState route;
    route.id = QStringLiteral("route_1");
    route.aircraftId = QStringLiteral("ac_1");
    route.loopMode = true;
    route.waypoints = {
        {41.0082, 28.9784, 5000, QStringLiteral("Istanbul")},
        {41.1, 28.95, 5100, QStringLiteral("WP2")},
        {37.9667, 34.6781, 5200, QStringLiteral("Nigde")}};
    m_appState->routeManager()->createRoute(route);

    m_nextAircraftId = 3;

    m_logPanel->appendLog(QStringLiteral("Demo scenario loaded: 2 aircraft, 1 route"));
    m_bridge->pushFullSync();
}
