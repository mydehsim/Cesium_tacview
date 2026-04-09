#ifndef TRACKRECORDER_H
#define TRACKRECORDER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include "core/AircraftState.h"

struct TickSnapshot
{
    quint64 tick = 0;
    qint64 timestamp = 0; // ms since epoch
    QMap<QString, AircraftState> aircraftStates;
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
