#ifndef AIRCRAFTINSPECTOR_H
#define AIRCRAFTINSPECTOR_H

#include <QDockWidget>
#include <QLabel>
#include <QFormLayout>

class AppState;

class AircraftInspector : public QDockWidget
{
    Q_OBJECT
public:
    explicit AircraftInspector(AppState *appState, QWidget *parent = nullptr);

public slots:
    void refresh();

private:
    AppState *m_appState;

    QLabel *m_idLabel;
    QLabel *m_callSignLabel;
    QLabel *m_typeLabel;
    QLabel *m_latLabel;
    QLabel *m_lonLabel;
    QLabel *m_altLabel;
    QLabel *m_headingLabel;
    QLabel *m_speedLabel;
    QLabel *m_modeLabel;
    QLabel *m_routeLabel;
};

#endif // AIRCRAFTINSPECTOR_H
