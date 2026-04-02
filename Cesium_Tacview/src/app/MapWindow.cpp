#include "MapWindow.h"
#include "AppState.h"
#include "bridge/CesiumBridge.h"
#include "ui/SimulationLogPanel.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QApplication>
#include <QScreen>
#include <QStatusBar>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>

// Custom page that captures JS console messages
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

MapWindow::MapWindow(AppState *appState, CesiumBridge *bridge, QWidget *parent)
    : QMainWindow(parent), m_appState(appState), m_bridge(bridge)
{
    setupUi();
    setWindowTitle(QStringLiteral("Cesium Tacview — Map"));
}

MapWindow::~MapWindow() = default;

void MapWindow::setupUi()
{
    // === Full-screen CesiumJS view ===
    m_webView = new QWebEngineView;
    setCentralWidget(m_webView);

    // WebGL/GPU settings BEFORE any URL load
    auto *settings = m_webView->settings();
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);

    // Profile-level settings + 1 GB disk cache
    auto *profile = QWebEngineProfile::defaultProfile();
    profile->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(1024 * 1024 * 1024);
    profile->setPersistentStoragePath(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/webengine"));

    // Log panel (small, at bottom of map window)
    m_logPanel = new SimulationLogPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_logPanel);
    m_logPanel->setMaximumHeight(120);
    m_logPanel->hide(); // Hidden by default, can be toggled from menu

    // Custom page with JS console capture
    auto *page = new CesiumPage(profile, m_webView);
    page->logPanel = m_logPanel;
    m_webView->setPage(page);

    // Attach QWebChannel bridge to this page
    m_bridge->attachToPage(m_webView->page());

    // Load Qt-optimized Cesium from Vite dev server
    m_webView->setUrl(QUrl(QStringLiteral("http://localhost:5173/index-qt.html")));

    // StatusBar with FPS and bridge status
    statusBar()->showMessage(QStringLiteral("Loading Cesium..."));

    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            statusBar()->showMessage(QStringLiteral("Cesium loaded — Waiting for viewer init..."));
        } else {
            statusBar()->showMessage(QStringLiteral("ERROR: Failed to load Cesium page"));
        }
    });
}

void MapWindow::moveToScreen(int screenIndex)
{
    const QList<QScreen *> screens = QApplication::screens();
    if (screenIndex < 0 || screenIndex >= screens.size())
        return;

    QScreen *target = screens.at(screenIndex);
    const QRect geom = target->availableGeometry();
    setScreen(target);
    setGeometry(geom);
    showMaximized();

    emit logMessage(QStringLiteral("Map window moved to screen %1 (%2)")
                        .arg(screenIndex)
                        .arg(target->name()));
}
