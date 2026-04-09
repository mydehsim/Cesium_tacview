#ifndef PLAYBACKCONTROLPANEL_H
#define PLAYBACKCONTROLPANEL_H

#include <QWidget>

class QSlider;
class QLabel;
class QPushButton;
class QComboBox;
class PlaybackEngine;
class TrackRecorder;

class PlaybackControlPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackControlPanel(PlaybackEngine *engine, TrackRecorder *recorder,
                                  QWidget *parent = nullptr);

public slots:
    void onStateChanged(int state);
    void onFrameChanged(int frame, int total);

signals:
    void logMessage(const QString &msg);
    void recordToggled(bool recording);

private slots:
    void onPlayPause();
    void onStop();
    void onRecord();
    void onSpeedChanged(int index);
    void onSliderMoved(int value);

private:
    void setupUi();
    void updateTimeLabel(int frame, int total);

    PlaybackEngine *m_engine;
    TrackRecorder *m_recorder;

    QPushButton *m_playPauseBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_recordBtn;
    QSlider *m_seekSlider;
    QLabel *m_timeLabel;
    QComboBox *m_speedCombo;
};

#endif // PLAYBACKCONTROLPANEL_H
