#include "PlaybackEngine.h"
#include "app/AppState.h"

PlaybackEngine::PlaybackEngine(AppState *appState, QObject *parent)
    : QObject(parent), m_appState(appState)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PlaybackEngine::advanceFrame);
}

void PlaybackEngine::loadRecording(const QVector<TickSnapshot> &snapshots)
{
    stop();
    m_snapshots = snapshots;
    m_currentFrame = 0;
    emit logMessage(QStringLiteral("[Playback] Loaded %1 frames").arg(m_snapshots.size()));
}

void PlaybackEngine::play()
{
    if (m_snapshots.isEmpty())
        return;

    if (m_currentFrame >= m_snapshots.size())
        m_currentFrame = 0;

    m_state = State::PLAYING;
    int interval = static_cast<int>(BASE_INTERVAL_MS / m_speedMultiplier);
    if (interval < 5)
        interval = 5;
    m_timer.start(interval);

    emit stateChanged(m_state);
    emit logMessage(QStringLiteral("[Playback] Playing at %.1fx").arg(m_speedMultiplier));
}

void PlaybackEngine::pause()
{
    m_timer.stop();
    m_state = State::PAUSED;
    emit stateChanged(m_state);
    emit logMessage(QStringLiteral("[Playback] Paused at frame %1").arg(m_currentFrame));
}

void PlaybackEngine::stop()
{
    m_timer.stop();
    m_currentFrame = 0;
    m_state = State::STOPPED;
    emit stateChanged(m_state);
    emit frameChanged(0, m_snapshots.size());
}

void PlaybackEngine::seek(int frame)
{
    if (frame < 0)
        frame = 0;
    if (frame >= m_snapshots.size())
        frame = m_snapshots.size() - 1;
    if (m_snapshots.isEmpty())
        return;

    m_currentFrame = frame;
    applyFrame(m_currentFrame);
    emit frameChanged(m_currentFrame, m_snapshots.size());
}

void PlaybackEngine::setSpeed(double multiplier)
{
    m_speedMultiplier = qBound(0.1, multiplier, 16.0);
    if (m_timer.isActive())
    {
        int interval = static_cast<int>(BASE_INTERVAL_MS / m_speedMultiplier);
        if (interval < 5)
            interval = 5;
        m_timer.setInterval(interval);
    }
    emit logMessage(QStringLiteral("[Playback] Speed: %.1fx").arg(m_speedMultiplier));
}

void PlaybackEngine::advanceFrame()
{
    if (m_currentFrame >= m_snapshots.size())
    {
        stop();
        emit playbackFinished();
        return;
    }

    applyFrame(m_currentFrame);
    emit frameChanged(m_currentFrame, m_snapshots.size());
    ++m_currentFrame;
}

void PlaybackEngine::applyFrame(int frame)
{
    if (frame < 0 || frame >= m_snapshots.size())
        return;

    const TickSnapshot &snap = m_snapshots.at(frame);
    auto *acMgr = m_appState->aircraftManager();

    // Update existing aircraft or create them
    for (auto it = snap.aircraftStates.cbegin(); it != snap.aircraftStates.cend(); ++it)
    {
        AircraftState *existing = acMgr->aircraft(it.key());
        if (existing)
        {
            // Copy position/orientation data, preserve trail
            const AircraftState &src = it.value();
            existing->lat = src.lat;
            existing->lon = src.lon;
            existing->alt = src.alt;
            existing->heading = src.heading;
            existing->speed = src.speed;
            existing->verticalSpeed = src.verticalSpeed;
            existing->roll = src.roll;
            existing->pitch = src.pitch;
            existing->magneticHeading = src.magneticHeading;
            existing->groundTrack = src.groundTrack;
            existing->groundSpeed = src.groundSpeed;
            existing->turnRate = src.turnRate;
            existing->gLoad = src.gLoad;
            existing->addTrailPoint();
        }
        else
        {
            // Aircraft appeared in recording but doesn't exist — create it
            AircraftState ac = it.value();
            acMgr->createAircraft(ac);
        }
    }
}
