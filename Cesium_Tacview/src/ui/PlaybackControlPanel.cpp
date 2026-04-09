#include "PlaybackControlPanel.h"
#include "simulation/PlaybackEngine.h"
#include "simulation/TrackRecorder.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>

PlaybackControlPanel::PlaybackControlPanel(PlaybackEngine *engine, TrackRecorder *recorder,
                                           QWidget *parent)
    : QWidget(parent), m_engine(engine), m_recorder(recorder)
{
    setupUi();

    connect(m_engine, &PlaybackEngine::stateChanged, this, [this](PlaybackEngine::State s)
            { onStateChanged(static_cast<int>(s)); });
    connect(m_engine, &PlaybackEngine::frameChanged, this, &PlaybackControlPanel::onFrameChanged);
    connect(m_recorder, &TrackRecorder::recordingChanged, this, [this](bool rec)
            {
        m_recordBtn->setText(rec ? QStringLiteral("⏺ REC") : QStringLiteral("⏺ Record"));
        m_recordBtn->setStyleSheet(rec ? QStringLiteral("color: red; font-weight: bold;") : QString()); });
}

void PlaybackControlPanel::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *titleLabel = new QLabel(QStringLiteral("<b>Playback / Recording</b>"));
    mainLayout->addWidget(titleLabel);

    // Transport controls row
    auto *transportRow = new QHBoxLayout;

    m_recordBtn = new QPushButton(QStringLiteral("⏺ Record"));
    m_recordBtn->setToolTip(QStringLiteral("Start/stop recording"));
    m_recordBtn->setFixedWidth(80);
    transportRow->addWidget(m_recordBtn);

    transportRow->addSpacing(8);

    m_playPauseBtn = new QPushButton(QStringLiteral("▶ Play"));
    m_playPauseBtn->setFixedWidth(70);
    transportRow->addWidget(m_playPauseBtn);

    m_stopBtn = new QPushButton(QStringLiteral("⏹ Stop"));
    m_stopBtn->setFixedWidth(60);
    transportRow->addWidget(m_stopBtn);

    transportRow->addSpacing(8);

    auto *speedLabel = new QLabel(QStringLiteral("Speed:"));
    transportRow->addWidget(speedLabel);

    m_speedCombo = new QComboBox;
    m_speedCombo->addItem(QStringLiteral("0.25x"), 0.25);
    m_speedCombo->addItem(QStringLiteral("0.5x"), 0.5);
    m_speedCombo->addItem(QStringLiteral("1x"), 1.0);
    m_speedCombo->addItem(QStringLiteral("2x"), 2.0);
    m_speedCombo->addItem(QStringLiteral("4x"), 4.0);
    m_speedCombo->addItem(QStringLiteral("8x"), 8.0);
    m_speedCombo->setCurrentIndex(2); // 1x default
    m_speedCombo->setFixedWidth(60);
    transportRow->addWidget(m_speedCombo);

    transportRow->addStretch();
    mainLayout->addLayout(transportRow);

    // Seek slider row
    auto *seekRow = new QHBoxLayout;

    m_seekSlider = new QSlider(Qt::Horizontal);
    m_seekSlider->setRange(0, 0);
    seekRow->addWidget(m_seekSlider, 1);

    m_timeLabel = new QLabel(QStringLiteral("00:00 / 00:00"));
    m_timeLabel->setFixedWidth(100);
    seekRow->addWidget(m_timeLabel);

    mainLayout->addLayout(seekRow);

    // Connections
    connect(m_playPauseBtn, &QPushButton::clicked, this, &PlaybackControlPanel::onPlayPause);
    connect(m_stopBtn, &QPushButton::clicked, this, &PlaybackControlPanel::onStop);
    connect(m_recordBtn, &QPushButton::clicked, this, &PlaybackControlPanel::onRecord);
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PlaybackControlPanel::onSpeedChanged);
    connect(m_seekSlider, &QSlider::sliderMoved, this, &PlaybackControlPanel::onSliderMoved);
}

void PlaybackControlPanel::onPlayPause()
{
    if (m_engine->state() == PlaybackEngine::State::PLAYING)
    {
        m_engine->pause();
    }
    else
    {
        // If recording is active, load it into playback first
        if (!m_engine->hasRecording() && m_recorder->snapshotCount() > 0)
        {
            m_recorder->stop();
            m_engine->loadRecording(m_recorder->snapshots());
        }
        m_engine->play();
    }
}

void PlaybackControlPanel::onStop()
{
    m_engine->stop();
}

void PlaybackControlPanel::onRecord()
{
    if (m_recorder->isRecording())
    {
        m_recorder->stop();
        emit logMessage(QStringLiteral("Recording stopped: %1 frames").arg(m_recorder->snapshotCount()));
    }
    else
    {
        m_recorder->clear();
        m_recorder->start();
        emit logMessage(QStringLiteral("Recording started"));
    }
    emit recordToggled(m_recorder->isRecording());
}

void PlaybackControlPanel::onSpeedChanged(int index)
{
    double speed = m_speedCombo->itemData(index).toDouble();
    m_engine->setSpeed(speed);
}

void PlaybackControlPanel::onSliderMoved(int value)
{
    m_engine->seek(value);
}

void PlaybackControlPanel::onStateChanged(int state)
{
    auto s = static_cast<PlaybackEngine::State>(state);
    switch (s)
    {
    case PlaybackEngine::State::PLAYING:
        m_playPauseBtn->setText(QStringLiteral("⏸ Pause"));
        break;
    case PlaybackEngine::State::PAUSED:
        m_playPauseBtn->setText(QStringLiteral("▶ Play"));
        break;
    case PlaybackEngine::State::STOPPED:
        m_playPauseBtn->setText(QStringLiteral("▶ Play"));
        break;
    }
}

void PlaybackControlPanel::onFrameChanged(int frame, int total)
{
    m_seekSlider->setRange(0, qMax(0, total - 1));
    if (!m_seekSlider->isSliderDown())
        m_seekSlider->setValue(frame);
    updateTimeLabel(frame, total);
}

void PlaybackControlPanel::updateTimeLabel(int frame, int total)
{
    // At 20Hz: frame / 20 = seconds
    auto formatTime = [](int frames) -> QString
    {
        int totalSec = frames / 20;
        int min = totalSec / 60;
        int sec = totalSec % 60;
        return QStringLiteral("%1:%2").arg(min, 2, 10, QLatin1Char('0')).arg(sec, 2, 10, QLatin1Char('0'));
    };
    m_timeLabel->setText(QStringLiteral("%1 / %2").arg(formatTime(frame), formatTime(total)));
}
