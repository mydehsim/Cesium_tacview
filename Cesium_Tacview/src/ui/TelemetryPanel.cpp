#include "TelemetryPanel.h"
#include "app/AppState.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>

TelemetryPanel::TelemetryPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

QWidget *TelemetryPanel::createGauge(const QString &label, QProgressBar *&bar, QLabel *&valLabel)
{
    auto *w = new QWidget;
    auto *layout = new QHBoxLayout(w);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto *lbl = new QLabel(label);
    lbl->setFixedWidth(60);
    layout->addWidget(lbl);

    bar = new QProgressBar;
    bar->setTextVisible(false);
    bar->setFixedHeight(14);
    layout->addWidget(bar, 1);

    valLabel = new QLabel(QStringLiteral("—"));
    valLabel->setFixedWidth(80);
    valLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(valLabel);

    return w;
}

void TelemetryPanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    m_titleLabel = new QLabel(QStringLiteral("<b>Telemetry</b> — No aircraft selected"));
    layout->addWidget(m_titleLabel);

    // Position info
    m_posLabel = new QLabel(QStringLiteral("Pos: —"));
    m_posLabel->setStyleSheet(QStringLiteral("color: #888; font-size: 10px;"));
    layout->addWidget(m_posLabel);

    // Mode + Route row
    auto *infoRow = new QHBoxLayout;
    m_modeLabel = new QLabel(QStringLiteral("Mode: —"));
    m_modeLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    infoRow->addWidget(m_modeLabel);
    m_routeLabel = new QLabel(QStringLiteral("Route: —"));
    infoRow->addWidget(m_routeLabel);
    infoRow->addStretch();
    layout->addLayout(infoRow);

    // Gauges
    layout->addWidget(createGauge(QStringLiteral("Alt"), m_altBar, m_altVal));
    m_altBar->setRange(0, 20000);

    layout->addWidget(createGauge(QStringLiteral("Speed"), m_speedBar, m_speedVal));
    m_speedBar->setRange(0, 400);

    layout->addWidget(createGauge(QStringLiteral("Heading"), m_headingBar, m_headingVal));
    m_headingBar->setRange(0, 360);

    layout->addWidget(createGauge(QStringLiteral("V/S"), m_vsBar, m_vsVal));
    m_vsBar->setRange(-100, 100); // -100 to +100 mapped to 0-200

    layout->addWidget(createGauge(QStringLiteral("Roll"), m_rollBar, m_rollVal));
    m_rollBar->setRange(0, 180); // 0-180 mapped from -90 to +90

    layout->addWidget(createGauge(QStringLiteral("G-Load"), m_gLoadBar, m_gLoadVal));
    m_gLoadBar->setRange(0, 90); // 0-90 mapped from 0 to 9.0

    layout->addStretch();
}

void TelemetryPanel::update(const AppState *state)
{
    const QString selectedId = state->selectionManager()->selectedEntityId();
    if (selectedId.isEmpty())
    {
        m_titleLabel->setText(QStringLiteral("<b>Telemetry</b> — No aircraft selected"));
        return;
    }

    const AircraftState *ac = state->aircraftManager()->aircraft(selectedId);
    if (!ac)
        return;

    m_titleLabel->setText(QStringLiteral("<b>Telemetry</b> — %1 (%2)")
                              .arg(ac->callSign, ac->id));

    m_posLabel->setText(QStringLiteral("Pos: %1°N %2°E")
                            .arg(ac->lat, 0, 'f', 4)
                            .arg(ac->lon, 0, 'f', 4));

    m_modeLabel->setText(QStringLiteral("Mode: %1").arg(controlModeToString(ac->controlMode)));
    // Color-code mode
    switch (ac->controlMode)
    {
    case ControlMode::AUTOPILOT:
        m_modeLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #2196F3;"));
        break;
    case ControlMode::MANUAL:
        m_modeLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #FF9800;"));
        break;
    case ControlMode::IDLE:
        m_modeLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #9E9E9E;"));
        break;
    case ControlMode::SCRIPTED:
        m_modeLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #4CAF50;"));
        break;
    }

    m_routeLabel->setText(ac->currentRouteId.isEmpty()
                              ? QStringLiteral("Route: none")
                              : QStringLiteral("Route: %1").arg(ac->currentRouteId));

    // Altitude
    m_altBar->setValue(static_cast<int>(ac->alt));
    m_altVal->setText(QStringLiteral("%1 m").arg(ac->alt, 0, 'f', 0));

    // Speed
    m_speedBar->setValue(static_cast<int>(ac->speed));
    m_speedVal->setText(QStringLiteral("%1 m/s").arg(ac->speed, 0, 'f', 1));

    // Heading
    m_headingBar->setValue(static_cast<int>(ac->heading));
    m_headingVal->setText(QStringLiteral("%1°").arg(ac->heading, 0, 'f', 1));

    // Vertical speed: map [-100, +100] to bar range [0, 200]
    int vsBarVal = static_cast<int>(ac->verticalSpeed) + 100;
    m_vsBar->setValue(qBound(0, vsBarVal, 200));
    m_vsVal->setText(QStringLiteral("%1 m/s").arg(ac->verticalSpeed, 0, 'f', 1));

    // Roll: map [-90, +90] to bar range [0, 180]
    int rollBarVal = static_cast<int>(ac->roll) + 90;
    m_rollBar->setValue(qBound(0, rollBarVal, 180));
    m_rollVal->setText(QStringLiteral("%1°").arg(ac->roll, 0, 'f', 1));

    // G-Load: map [0, 9.0] to bar range [0, 90]
    int gBarVal = static_cast<int>(ac->gLoad * 10.0);
    m_gLoadBar->setValue(qBound(0, gBarVal, 90));
    m_gLoadVal->setText(QStringLiteral("%1 G").arg(ac->gLoad, 0, 'f', 2));
}

void TelemetryPanel::clear()
{
    m_titleLabel->setText(QStringLiteral("<b>Telemetry</b> — No aircraft selected"));
    m_posLabel->setText(QStringLiteral("Pos: —"));
    m_modeLabel->setText(QStringLiteral("Mode: —"));
    m_routeLabel->setText(QStringLiteral("Route: —"));
    m_altBar->setValue(0);
    m_speedBar->setValue(0);
    m_headingBar->setValue(0);
    m_vsBar->setValue(100);
    m_rollBar->setValue(90);
    m_gLoadBar->setValue(10);
    m_altVal->setText(QStringLiteral("—"));
    m_speedVal->setText(QStringLiteral("—"));
    m_headingVal->setText(QStringLiteral("—"));
    m_vsVal->setText(QStringLiteral("—"));
    m_rollVal->setText(QStringLiteral("—"));
    m_gLoadVal->setText(QStringLiteral("—"));
}
