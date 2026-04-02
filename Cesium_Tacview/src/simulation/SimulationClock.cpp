#include "SimulationClock.h"

SimulationClock::SimulationClock(QObject *parent)
    : QObject(parent)
{
    recalcDt();
}

void SimulationClock::advance()
{
    ++m_currentTick;
}

void SimulationClock::setTickRate(int hz)
{
    if (hz < 1) hz = 1;
    if (hz > 120) hz = 120;
    m_tickRate = hz;
    recalcDt();
    emit tickRateChanged(hz);
}

void SimulationClock::setTimeScale(double scale)
{
    if (scale < 0.0) scale = 0.0;
    if (scale > 100.0) scale = 100.0;
    m_timeScale = scale;
    recalcDt();
}

void SimulationClock::setPaused(bool paused)
{
    if (m_paused == paused) return;
    m_paused = paused;
    emit pausedChanged(paused);
}

void SimulationClock::reset()
{
    m_currentTick = 0;
}

void SimulationClock::recalcDt()
{
    m_dt = m_timeScale / static_cast<double>(m_tickRate);
}
