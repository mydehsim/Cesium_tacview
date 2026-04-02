#include "MainWindow.h"
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

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QUrl>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>

// Custom page that captures JS console messages into the simulation log
class CesiumPage : public QWebEnginePage
{
public:
    using QWebEnginePage::QWebEnginePage;
    SimulationLogPanel *logPanel = nullptr;

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                 const QString &message, int lineNumber,
                                 const QString &sourceID) override
    {
        Q_UNUSED(level)
        QString logMsg = QStringLiteral("[JS:%1:%2] %3")
            .arg(sourceID.section(QLatin1Char('/'), -1))
            .arg(lineNumber)
            .arg(message);
        qDebug().noquote() << logMsg;
        if (logPanel)
            logPanel->appendLog(logMsg);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Core systems
    m_appState = new AppState(this);
    m_bridge   = new CesiumBridge(m_appState, this);
    m_simEngine = new SimulationEngine(m_appState, this);
    m_inputManager = new InputManager(m_appState, m_simEngine, m_bridge, this);

    setupUi();
    setupConnections();

    setWindowTitle(QStringLiteral("Cesium Tacview — Flight Simulation"));
    resize(1600, 900);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    // --- Central Widget: CesiumJS via QWebEngineView ---
    m_webView = new QWebEngineView;
    setCentralWidget(m_webView);

    // IMPORTANT: Configure WebGL/GPU settings BEFORE loading any URL
    auto *settings = m_webView->settings();
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);

    // Enable profile-level WebGL + disk cache for Cesium tiles
    auto *profile = QWebEngineProfile::defaultProfile();
    profile->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(1024 * 1024 * 1024); // 1 GB tile cache

    // Persistent storage for Cesium ion tokens etc.
    profile->setPersistentStoragePath(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/webengine"));

    // Use custom page that captures JS console output
    auto *page = new CesiumPage(profile, m_webView);
    m_webView->setPage(page);

    // --- Dock Panels (create BEFORE page->logPanel assignment) ---
    m_logPanel = new SimulationLogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);

    m_cmdHistoryPanel = new CommandHistoryPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_cmdHistoryPanel);
    tabifyDockWidget(m_logPanel, m_cmdHistoryPanel);
    m_logPanel->raise();

    // Now wire the log panel to the page
    page->logPanel = m_logPanel;

    // Attach QWebChannel to the web page
    m_bridge->attachToPage(m_webView->page());

    // Load existing Cesium web app from Vite dev server
    m_webView->setUrl(QUrl(QStringLiteral("http://localhost:5173")));

    // Inject CesiumJS performance tuning after page finishes loading
    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (!ok) return;
        // Wait for Cesium viewer to initialize, then tune performance
        QTimer::singleShot(3000, this, [this]() {
            const QString perfScript = QStringLiteral(R"JS(
                (function() {
                    if (typeof viewer === 'undefined') {
                        console.log('[Qt] viewer not found, skipping perf tuning');
                        return;
                    }
                    // Terrain: higher error tolerance = less tile requests = faster
                    viewer.scene.globe.maximumScreenSpaceError = 4;  // default=2

                    // Disable expensive post-processing effects
                    viewer.scene.fxaa = false;
                    viewer.scene.fog.enabled = false;
                    viewer.scene.globe.showGroundAtmosphere = false;
                    viewer.scene.skyAtmosphere.show = false;

                    // Reduce shadow/lighting overhead
                    viewer.shadows = false;
                    viewer.scene.globe.enableLighting = false;

                    // Render on demand (not continuous) when idle
                    viewer.scene.requestRenderMode = true;
                    viewer.scene.maximumRenderTimeChange = 0.0;

                    // Tileset performance
                    viewer.scene.globe.tileCacheSize = 1000;

                    console.log('[Qt] CesiumJS performance tuning applied');
                })();
            )JS");
            m_webView->page()->runJavaScript(perfScript);
        });
    });

    // --- Menu Bar ---
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    auto *simMenu = menuBar()->addMenu(tr("&Simulation"));
    simMenu->addAction(tr("&Start"), m_simEngine, &SimulationEngine::start);
    simMenu->addAction(tr("&Pause"), m_simEngine, &SimulationEngine::togglePause);
    simMenu->addAction(tr("Sto&p"), m_simEngine, &SimulationEngine::stop);

    auto *acMenu = menuBar()->addMenu(tr("&Aircraft"));
    acMenu->addAction(tr("&Create Aircraft"), this, &MainWindow::onCreateAircraft, QKeySequence(QStringLiteral("Ctrl+N")));

    // --- Toolbar ---
    auto *toolbar = addToolBar(tr("Simulation"));
    toolbar->addAction(tr("▶ Start"), m_simEngine, &SimulationEngine::start);
    toolbar->addAction(tr("⏸ Pause"), m_simEngine, &SimulationEngine::togglePause);
    toolbar->addAction(tr("⏹ Stop"), m_simEngine, &SimulationEngine::stop);
    toolbar->addSeparator();
    toolbar->addAction(tr("+ Aircraft"), this, &MainWindow::onCreateAircraft);

    // --- Dock Panels ---
    m_aircraftPanel = new AircraftListPanel(m_appState, m_bridge, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_aircraftPanel);

    m_inspectorPanel = new AircraftInspector(m_appState, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_inspectorPanel);

    m_routePanel = new RouteEditorPanel(m_appState, m_bridge, this);
    addDockWidget(Qt::LeftDockWidgetArea, m_routePanel);

    // --- Status Bar ---
    statusBar()->showMessage(tr("Ready — Waiting for Cesium…"));
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

    // Simulation tick → push delta to Cesium
    connect(m_simEngine, &SimulationEngine::tickCompleted, this, &MainWindow::onTickCompleted);
    connect(m_simEngine, &SimulationEngine::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // Input manager logs
    connect(m_inputManager, &InputManager::logMessage, m_logPanel, &SimulationLogPanel::appendLog);

    // Aircraft panel actions
    connect(m_aircraftPanel, &AircraftListPanel::createAircraftRequested, this, &MainWindow::onCreateAircraft);
    connect(m_aircraftPanel, &AircraftListPanel::deleteAircraftRequested, this, &MainWindow::onDeleteAircraft);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Don't intercept if web view has focus for text input etc.
    if (!event->isAutoRepeat()) {
        m_inputManager->keyPressed(event->key());
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (!event->isAutoRepeat()) {
        m_inputManager->keyReleased(event->key());
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::onCesiumReady()
{
    statusBar()->showMessage(tr("Cesium ready — Simulation idle"));
    m_logPanel->appendLog(QStringLiteral("Cesium viewer connected and ready"));

    createDemonstrationScenario();
}

void MainWindow::onTickCompleted(quint64 tick)
{
    // Push delta state to Cesium every tick
    m_bridge->pushDelta();

    // Update inspector every 5 ticks (~4 times/sec)
    if (tick % 5 == 0) {
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
    if (!ok || callSign.isEmpty()) return;

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
    m_bridge->pushFullSync();

    m_logPanel->appendLog(QStringLiteral("Created aircraft: %1 (%2)").arg(ac.id, ac.callSign));
}

void MainWindow::onDeleteAircraft(const QString &id)
{
    auto ret = QMessageBox::question(this, tr("Delete Aircraft"),
                                     tr("Delete aircraft %1?").arg(id));
    if (ret != QMessageBox::Yes) return;

    m_appState->aircraftManager()->removeAircraft(id);
    if (m_appState->selectionManager()->selectedEntityId() == id)
        m_appState->selectionManager()->clearSelection();
    m_bridge->pushFullSync();
    m_logPanel->appendLog(QStringLiteral("Deleted aircraft: %1").arg(id));
}

void MainWindow::createDemonstrationScenario()
{
    // Create a couple of demo aircraft
    AircraftState ac1;
    ac1.id = QStringLiteral("ac_1");
    ac1.callSign = QStringLiteral("ALPHA-01");
    ac1.type = QStringLiteral("Fighter");
    ac1.lat = 41.0082; ac1.lon = 28.9784; ac1.alt = 5000;
    ac1.heading = 45; ac1.targetHeading = 45;
    ac1.speed = 100; ac1.targetSpeed = 100;
    ac1.targetAlt = 5000;
    ac1.controlMode = ControlMode::IDLE;
    m_appState->aircraftManager()->createAircraft(ac1);

    AircraftState ac2;
    ac2.id = QStringLiteral("ac_2");
    ac2.callSign = QStringLiteral("BRAVO-02");
    ac2.type = QStringLiteral("Transport");
    ac2.lat = 40.9; ac2.lon = 28.8; ac2.alt = 3000;
    ac2.heading = 90; ac2.targetHeading = 90;
    ac2.speed = 80; ac2.targetSpeed = 80;
    ac2.targetAlt = 3000;
    ac2.modelUri = QStringLiteral("/models/aircraft.glb");
    ac2.controlMode = ControlMode::IDLE;
    m_appState->aircraftManager()->createAircraft(ac2);

    // Create a demo route
    RouteState route;
    route.id = QStringLiteral("route_1");
    route.aircraftId = QStringLiteral("ac_1");
    route.waypoints = {
        {41.0082, 28.9784, 5000, QStringLiteral("Istanbul")},
        {41.1, 28.95, 5100, QStringLiteral("WP2")},
        {37.9667, 34.6781, 5200, QStringLiteral("Nigde")}
    };
    m_appState->routeManager()->createRoute(route);

    m_nextAircraftId = 3;

    m_logPanel->appendLog(QStringLiteral("Demo scenario loaded: 2 aircraft, 1 route"));
    m_bridge->pushFullSync();
}
