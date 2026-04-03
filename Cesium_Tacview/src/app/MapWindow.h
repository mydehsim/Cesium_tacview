#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include <QMainWindow>

class QWebEngineView;
class QWebEnginePage;
class AppState;
class CesiumBridge;
class SimulationLogPanel;
class MapToolbar;

class MapWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MapWindow(AppState *appState, CesiumBridge *bridge, QWidget *parent = nullptr);
    ~MapWindow() override;

    QWebEngineView *webView() { return m_webView; }
    MapToolbar *toolbar() { return m_toolbar; }

    void moveToScreen(int screenIndex);
    void loadCesium();  // Call after Vite dev server is ready
    void showLoading(); // Show waiting message

signals:
    void logMessage(const QString &msg);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();
    void repositionToolbar();

    AppState *m_appState;
    CesiumBridge *m_bridge;
    QWebEngineView *m_webView;
    SimulationLogPanel *m_logPanel;
    MapToolbar *m_toolbar;
};

#endif // MAPWINDOW_H
