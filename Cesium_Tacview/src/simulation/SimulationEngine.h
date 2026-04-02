#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include <QObject>
#include <QTimer>
#include "SimulationClock.h"
#include "ManualController.h"

class AppState;

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimulationEngine(AppState *appState, QObject *parent = nullptr);

    SimulationClock *clock() { return &m_clock; }
    const InputState &inputState() const { return m_inputState; }
    InputState &inputState() { return m_inputState; }

    void start();
    void stop();
    void pause();
    void resume();
    void togglePause();

    bool isRunning() const { return m_timer.isActive(); }

signals:
    void tickCompleted(quint64 tick);
    void logMessage(const QString &msg);

private slots:
    void tick();

private:
    AppState *m_appState;
    QTimer m_timer;
    SimulationClock m_clock;
    InputState m_inputState;
};

#endif // SIMULATIONENGINE_H
