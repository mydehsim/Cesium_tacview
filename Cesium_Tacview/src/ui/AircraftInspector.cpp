#include "AircraftInspector.h"
#include "app/AppState.h"

AircraftInspector::AircraftInspector(AppState *appState, QWidget *parent)
    : QDockWidget(tr("Aircraft Inspector"), parent), m_appState(appState)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget;
    auto *layout = new QFormLayout(container);

    m_idLabel = new QLabel(QStringLiteral("-"));
    m_callSignLabel = new QLabel(QStringLiteral("-"));
    m_typeLabel = new QLabel(QStringLiteral("-"));
    m_latLabel = new QLabel(QStringLiteral("-"));
    m_lonLabel = new QLabel(QStringLiteral("-"));
    m_altLabel = new QLabel(QStringLiteral("-"));
    m_headingLabel = new QLabel(QStringLiteral("-"));
    m_speedLabel = new QLabel(QStringLiteral("-"));
    m_modeLabel = new QLabel(QStringLiteral("-"));
    m_routeLabel = new QLabel(QStringLiteral("-"));

    layout->addRow(tr("ID:"), m_idLabel);
    layout->addRow(tr("CallSign:"), m_callSignLabel);
    layout->addRow(tr("Type:"), m_typeLabel);
    layout->addRow(tr("Lat:"), m_latLabel);
    layout->addRow(tr("Lon:"), m_lonLabel);
    layout->addRow(tr("Alt:"), m_altLabel);
    layout->addRow(tr("Heading:"), m_headingLabel);
    layout->addRow(tr("Speed:"), m_speedLabel);
    layout->addRow(tr("Mode:"), m_modeLabel);
    layout->addRow(tr("Route:"), m_routeLabel);

    setWidget(container);

    connect(appState->selectionManager(), &SelectionManager::selectionChanged,
            this, [this](const QString &, SelectionType)
            { if (isVisible()) refresh(); });
}

void AircraftInspector::refresh()
{
    const QString sel = m_appState->selectionManager()->selectedEntityId();
    const AircraftState *ac = m_appState->aircraftManager()->aircraft(sel);

    if (!ac)
    {
        m_idLabel->setText(QStringLiteral("-"));
        m_callSignLabel->setText(QStringLiteral("-"));
        m_typeLabel->setText(QStringLiteral("-"));
        m_latLabel->setText(QStringLiteral("-"));
        m_lonLabel->setText(QStringLiteral("-"));
        m_altLabel->setText(QStringLiteral("-"));
        m_headingLabel->setText(QStringLiteral("-"));
        m_speedLabel->setText(QStringLiteral("-"));
        m_modeLabel->setText(QStringLiteral("-"));
        m_routeLabel->setText(QStringLiteral("-"));
        return;
    }

    m_idLabel->setText(ac->id);
    m_callSignLabel->setText(ac->callSign);
    m_typeLabel->setText(ac->type);
    m_latLabel->setText(QString::number(ac->lat, 'f', 6));
    m_lonLabel->setText(QString::number(ac->lon, 'f', 6));
    m_altLabel->setText(QString::number(ac->alt, 'f', 1) + QStringLiteral(" m"));
    m_headingLabel->setText(QString::number(ac->heading, 'f', 1) + QStringLiteral("°"));
    m_speedLabel->setText(QString::number(ac->speed, 'f', 1) + QStringLiteral(" m/s"));
    m_modeLabel->setText(controlModeToString(ac->controlMode));
    m_routeLabel->setText(ac->currentRouteId.isEmpty() ? QStringLiteral("-") : ac->currentRouteId);
}
