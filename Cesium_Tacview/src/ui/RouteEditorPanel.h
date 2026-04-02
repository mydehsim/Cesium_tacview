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
    void onAddWaypoint();
    void onRemoveWaypoint();
    void onApplyRoute();
    void onRouteSelected(int index);

private:
    AppState     *m_appState;
    CesiumBridge *m_bridge;
    QComboBox    *m_routeCombo;
    QListWidget  *m_waypointList;
    QPushButton  *m_addBtn;
    QPushButton  *m_removeBtn;
    QPushButton  *m_applyBtn;
};

#endif // ROUTEEDITORPANEL_H
