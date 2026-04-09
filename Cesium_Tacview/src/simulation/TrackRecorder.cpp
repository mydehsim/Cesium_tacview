#include "TrackRecorder.h"
#include <QDateTime>

TrackRecorder::TrackRecorder(QObject *parent)
    : QObject(parent)
{
}

void TrackRecorder::start()
{
    if (m_recording)
        return;
    m_recording = true;
    emit recordingChanged(true);
}

void TrackRecorder::stop()
{
    if (!m_recording)
        return;
    m_recording = false;
    emit recordingChanged(false);
}

void TrackRecorder::clear()
{
    m_snapshots.clear();
    m_snapshots.squeeze();
}

void TrackRecorder::capture(quint64 tick, const QMap<QString, AircraftState> &aircraft)
{
    if (!m_recording)
        return;

    // Enforce max recording length
    if (m_snapshots.size() >= MAX_SNAPSHOTS)
    {
        stop();
        return;
    }

    TickSnapshot snap;
    snap.tick = tick;
    snap.timestamp = QDateTime::currentMSecsSinceEpoch();

    // Deep-copy aircraft states (without trail data to save memory)
    for (auto it = aircraft.cbegin(); it != aircraft.cend(); ++it)
    {
        AircraftState acCopy = it.value();
        acCopy.trail.clear(); // Don't record trail — reconstructed on playback
        snap.aircraftStates.insert(it.key(), acCopy);
    }

    m_snapshots.append(std::move(snap));
}
