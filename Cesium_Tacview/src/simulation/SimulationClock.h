#ifndef SIMULATIONCLOCK_H
#define SIMULATIONCLOCK_H

#include <QObject>

class SimulationClock : public QObject
{
    Q_OBJECT
public:
    explicit SimulationClock(QObject *parent = nullptr);

    quint64 currentTick() const { return m_currentTick; }
    int     tickRate()    const { return m_tickRate; }
    double  timeScale()   const { return m_timeScale; }
    bool    isPaused()    const { return m_paused; }
    double  dt()          const { return m_dt; }

    void advance();
    void setTickRate(int hz);
    void setTimeScale(double scale);
    void setPaused(bool paused);
    void reset();

signals:
    void pausedChanged(bool paused);
    void tickRateChanged(int hz);

private:
    void recalcDt();

    quint64 m_currentTick = 0;
    int     m_tickRate    = 20;     // Hz
    double  m_timeScale   = 1.0;
    bool    m_paused      = false;
    double  m_dt          = 0.05;   // seconds per tick
};

#endif // SIMULATIONCLOCK_H
