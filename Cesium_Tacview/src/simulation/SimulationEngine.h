#ifndef SIMULATIONENGINE_H
#define SIMULATIONENGINE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include "SimulationClock.h"
#include "ManualController.h"

class AppState;

class SimulationEngine : public QObject
{
    Q_OBJECT
public:
    explicit SimulationEngine(AppState *appState, QObject *parent = nullptr);

    SimulationClock *clock() { return &m_clock; }

    // Thread-safe input accessors
    InputState inputState() const
    {
        QMutexLocker lk(&m_inputMutex);
        return m_inputState;
    }
    void setInputState(const InputState &s)
    {
        QMutexLocker lk(&m_inputMutex);
        m_inputState = s;
    }

public slots:
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
    mutable QMutex m_inputMutex;
};

#endif // SIMULATIONENGINE_H
