#ifndef AIRCRAFTLISTPANEL_H
#define AIRCRAFTLISTPANEL_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>

class AppState;
class CesiumBridge;

class AircraftListPanel : public QDockWidget
{
    Q_OBJECT
public:
    explicit AircraftListPanel(AppState *appState, CesiumBridge *bridge,
                               QWidget *parent = nullptr);

public slots:
    void refresh();

signals:
    void createAircraftRequested();
    void deleteAircraftRequested(const QString &id);

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onCreateClicked();
    void onDeleteClicked();

private:
    AppState *m_appState;
    CesiumBridge *m_bridge;
    QTreeWidget *m_tree;
    QPushButton *m_createBtn;
    QPushButton *m_deleteBtn;
};

#endif // AIRCRAFTLISTPANEL_H
