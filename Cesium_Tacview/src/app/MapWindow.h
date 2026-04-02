#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include <QMainWindow>

class QWebEngineView;
class QWebEnginePage;
class AppState;
class CesiumBridge;
class SimulationLogPanel;

class MapWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MapWindow(AppState *appState, CesiumBridge *bridge, QWidget *parent = nullptr);
    ~MapWindow() override;

    QWebEngineView *webView() { return m_webView; }

    void moveToScreen(int screenIndex);

signals:
    void logMessage(const QString &msg);

private:
    void setupUi();

    AppState *m_appState;
    CesiumBridge *m_bridge;
    QWebEngineView *m_webView;
    SimulationLogPanel *m_logPanel;
};

#endif // MAPWINDOW_H
