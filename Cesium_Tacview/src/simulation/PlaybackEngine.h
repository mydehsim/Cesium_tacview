#ifndef PLAYBACKENGINE_H
#define PLAYBACKENGINE_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include "simulation/TrackRecorder.h"

class AppState;

class PlaybackEngine : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackEngine(AppState *appState, QObject *parent = nullptr);

    enum class State
    {
        STOPPED,
        PLAYING,
        PAUSED
    };
    Q_ENUM(State)

    void loadRecording(const QVector<TickSnapshot> &snapshots);
    bool hasRecording() const { return !m_snapshots.isEmpty(); }
    int totalFrames() const { return m_snapshots.size(); }
    int currentFrame() const { return m_currentFrame; }
    State state() const { return m_state; }
    double speedMultiplier() const { return m_speedMultiplier; }

public slots:
    void play();
    void pause();
    void stop();
    void seek(int frame);
    void setSpeed(double multiplier);

signals:
    void stateChanged(PlaybackEngine::State state);
    void frameChanged(int frame, int total);
    void playbackFinished();
    void logMessage(const QString &msg);

private slots:
    void advanceFrame();

private:
    void applyFrame(int frame);

    AppState *m_appState;
    QTimer m_timer;
    QVector<TickSnapshot> m_snapshots;
    int m_currentFrame = 0;
    State m_state = State::STOPPED;
    double m_speedMultiplier = 1.0;
    static constexpr int BASE_INTERVAL_MS = 50; // 20 Hz base
};

#endif // PLAYBACKENGINE_H
