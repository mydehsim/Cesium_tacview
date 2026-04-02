#ifndef ROUTEEDITORPANEL_H
#define ROUTEEDITORPANEL_H

#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>

class AppState;
class CesiumBridge;

class RouteEditorPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit RouteEditorPanel(AppState *appState, CesiumBridge *bridge,
                              QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onNewRoute();
    void onAddWaypoint();
    void onRemoveWaypoint();
    void onApplyRoute();
    void onRouteSelected(int index);
    void onLoopToggled(bool checked);

private:
    AppState *m_appState;
    CesiumBridge *m_bridge;
    QComboBox *m_routeCombo;
    QListWidget *m_waypointList;
    QPushButton *m_newRouteBtn;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_loopBtn;
};

#endif // ROUTEEDITORPANEL_H
