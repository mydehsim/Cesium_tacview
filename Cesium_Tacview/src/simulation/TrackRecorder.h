#ifndef TRACKRECORDER_H
#define TRACKRECORDER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include "core/AircraftState.h"

// Compact snapshot: only the fields needed for playback (~112 bytes vs ~2KB full AircraftState)
struct CompactAircraftSnapshot
{
    double lat, lon, alt;
    double heading, speed, verticalSpeed;
    double roll, pitch, yaw;
    double magneticHeading, groundTrack, groundSpeed;
    double turnRate, gLoad;
    ControlMode controlMode;

    static CompactAircraftSnapshot fromAircraft(const AircraftState &ac)
    {
        return {ac.lat, ac.lon, ac.alt,
                ac.heading, ac.speed, ac.verticalSpeed,
                ac.roll, ac.pitch, ac.yaw,
                ac.magneticHeading, ac.groundTrack, ac.groundSpeed,
                ac.turnRate, ac.gLoad, ac.controlMode};
    }

    void applyTo(AircraftState &ac) const
    {
        ac.lat = lat;
        ac.lon = lon;
        ac.alt = alt;
        ac.heading = heading;
        ac.speed = speed;
        ac.verticalSpeed = verticalSpeed;
        ac.roll = roll;
        ac.pitch = pitch;
        ac.yaw = yaw;
        ac.magneticHeading = magneticHeading;
        ac.groundTrack = groundTrack;
        ac.groundSpeed = groundSpeed;
        ac.turnRate = turnRate;
        ac.gLoad = gLoad;
        ac.controlMode = controlMode;
    }
};

struct TickSnapshot
{
    quint64 tick = 0;
    qint64 timestamp = 0; // ms since epoch
    QMap<QString, CompactAircraftSnapshot> aircraftStates;
};

class TrackRecorder : public QObject
{
    Q_OBJECT
public:
    explicit TrackRecorder(QObject *parent = nullptr);

    bool isRecording() const { return m_recording; }
    int snapshotCount() const { return m_snapshots.size(); }
    const QVector<TickSnapshot> &snapshots() const { return m_snapshots; }
    QVector<TickSnapshot> &snapshots() { return m_snapshots; }

    void capture(quint64 tick, const QMap<QString, AircraftState> &aircraft);

    static constexpr int MAX_SNAPSHOTS = 36000; // 30 min @ 20Hz

public slots:
    void start();
    void stop();
    void clear();

signals:
    void recordingChanged(bool recording);

private:
    bool m_recording = false;
    QVector<TickSnapshot> m_snapshots;
};

#endif // TRACKRECORDER_H
