#include "RouteEditorPanel.h"
#include "app/AppState.h"
#include "bridge/CesiumBridge.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

RouteEditorPanel::RouteEditorPanel(AppState *appState, CesiumBridge *bridge,
                                   QWidget *parent)
    : QDockWidget(tr("Route Editor"), parent), m_appState(appState), m_bridge(bridge)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);

    m_routeCombo = new QComboBox;
    layout->addWidget(m_routeCombo);

    m_waypointList = new QListWidget;
    m_waypointList->setDragDropMode(QAbstractItemView::InternalMove);
    layout->addWidget(m_waypointList);

    auto *btnLayout = new QHBoxLayout;
    m_addBtn = new QPushButton(tr("+ WP"));
    m_removeBtn = new QPushButton(tr("- WP"));
    m_applyBtn = new QPushButton(tr("Apply"));
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_applyBtn);
    layout->addLayout(btnLayout);

    setWidget(container);

    connect(m_routeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RouteEditorPanel::onRouteSelected);
    connect(m_addBtn, &QPushButton::clicked, this, &RouteEditorPanel::onAddWaypoint);
    connect(m_removeBtn, &QPushButton::clicked, this, &RouteEditorPanel::onRemoveWaypoint);
    connect(m_applyBtn, &QPushButton::clicked, this, &RouteEditorPanel::onApplyRoute);

    connect(appState->routeManager(), &RouteManager::stateChanged, this, &RouteEditorPanel::refresh);
}

void RouteEditorPanel::refresh()
{
    QString current = m_routeCombo->currentData().toString();
    m_routeCombo->clear();

    for (const QString &id : m_appState->routeManager()->routeIds())
    {
        m_routeCombo->addItem(id, id);
    }

    int idx = m_routeCombo->findData(current);
    if (idx >= 0)
        m_routeCombo->setCurrentIndex(idx);

    onRouteSelected(m_routeCombo->currentIndex());
}

void RouteEditorPanel::onRouteSelected(int index)
{
    m_waypointList->clear();
    if (index < 0)
        return;

    QString routeId = m_routeCombo->currentData().toString();
    const RouteState *route = m_appState->routeManager()->route(routeId);
    if (!route)
        return;

    for (int i = 0; i < route->waypoints.size(); ++i)
    {
        const auto &wp = route->waypoints[i];
        QString text = QStringLiteral("WP%1: %2, %3 @ %4m")
                           .arg(i + 1)
                           .arg(wp.lat, 0, 'f', 4)
                           .arg(wp.lon, 0, 'f', 4)
                           .arg(wp.alt, 0, 'f', 0);
        if (!wp.name.isEmpty())
            text = wp.name + QStringLiteral(" — ") + text;
        m_waypointList->addItem(text);
    }
}

void RouteEditorPanel::onAddWaypoint()
{
    QString routeId = m_routeCombo->currentData().toString();
    if (routeId.isEmpty())
        return;

    Waypoint wp;
    wp.name = QStringLiteral("New WP");
    wp.lat = 41.0;
    wp.lon = 29.0;
    wp.alt = 300;
    m_appState->routeManager()->addWaypoint(routeId, wp);
    m_bridge->pushFullSync();
}

void RouteEditorPanel::onRemoveWaypoint()
{
    QString routeId = m_routeCombo->currentData().toString();
    int row = m_waypointList->currentRow();
    if (routeId.isEmpty() || row < 0)
        return;

    m_appState->routeManager()->removeWaypoint(routeId, row);
    m_bridge->pushFullSync();
}

void RouteEditorPanel::onApplyRoute()
{
    QString routeId = m_routeCombo->currentData().toString();
    QString acId = m_appState->selectionManager()->selectedEntityId();
    if (routeId.isEmpty() || acId.isEmpty())
        return;

    AircraftState *ac = m_appState->aircraftManager()->aircraft(acId);
    if (!ac)
        return;

    ac->currentRouteId = routeId;
    ac->controlMode = ControlMode::AUTOPILOT;

    RouteState *route = m_appState->routeManager()->route(routeId);
    if (route)
    {
        route->aircraftId = acId;
        route->currentWaypointIndex = 0;
    }

    m_bridge->pushFullSync();
}
