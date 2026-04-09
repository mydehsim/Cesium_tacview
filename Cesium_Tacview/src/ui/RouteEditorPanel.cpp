#include "RouteEditorPanel.h"
#include "app/AppState.h"
#include "bridge/CesiumBridge.h"
#include "simulation/KinematicModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

RouteEditorPanel::RouteEditorPanel(AppState *appState, CesiumBridge *bridge,
                                   QWidget *parent)
    : QDockWidget(tr("Route Editor"), parent), m_appState(appState), m_bridge(bridge)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);

    // Route selector row
    auto *routeRow = new QHBoxLayout;
    routeRow->addWidget(new QLabel(tr("Route:")));
    m_routeCombo = new QComboBox;
    routeRow->addWidget(m_routeCombo, 1);
    m_newRouteBtn = new QPushButton(tr("+ New"));
    m_newRouteBtn->setMaximumWidth(60);
    routeRow->addWidget(m_newRouteBtn);
    layout->addLayout(routeRow);

    // Loop toggle + distance label
    auto *infoRow = new QHBoxLayout;
    m_loopBtn = new QPushButton(tr("🔁 Loop: OFF"));
    m_loopBtn->setCheckable(true);
    infoRow->addWidget(m_loopBtn);
    m_distanceLabel = new QLabel(tr("Distance: —"));
    m_distanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoRow->addWidget(m_distanceLabel);
    layout->addLayout(infoRow);

    // Waypoint list
    m_waypointList = new QListWidget;
    m_waypointList->setDragDropMode(QAbstractItemView::InternalMove);
    m_waypointList->setAlternatingRowColors(true);
    layout->addWidget(m_waypointList);

    // Reorder buttons
    auto *reorderRow = new QHBoxLayout;
    m_upBtn = new QPushButton(tr("↑ Up"));
    m_downBtn = new QPushButton(tr("↓ Down"));
    reorderRow->addWidget(m_upBtn);
    reorderRow->addWidget(m_downBtn);
    reorderRow->addStretch();
    layout->addLayout(reorderRow);

    // Action buttons
    auto *btnLayout = new QHBoxLayout;
    m_addBtn = new QPushButton(tr("+ WP"));
    m_removeBtn = new QPushButton(tr("- WP"));
    m_applyBtn = new QPushButton(tr("⇒ Assign to Aircraft"));
    m_applyBtn->setToolTip(tr("Assign this route to the selected aircraft and start AUTOPILOT"));
    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addWidget(m_applyBtn);
    layout->addLayout(btnLayout);

    setWidget(container);

    connect(m_routeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RouteEditorPanel::onRouteSelected);
    connect(m_newRouteBtn, &QPushButton::clicked, this, &RouteEditorPanel::onNewRoute);
    connect(m_addBtn, &QPushButton::clicked, this, &RouteEditorPanel::onAddWaypoint);
    connect(m_removeBtn, &QPushButton::clicked, this, &RouteEditorPanel::onRemoveWaypoint);
    connect(m_applyBtn, &QPushButton::clicked, this, &RouteEditorPanel::onApplyRoute);
    connect(m_loopBtn, &QPushButton::toggled, this, &RouteEditorPanel::onLoopToggled);
    connect(m_upBtn, &QPushButton::clicked, this, &RouteEditorPanel::onMoveUp);
    connect(m_downBtn, &QPushButton::clicked, this, &RouteEditorPanel::onMoveDown);

    connect(appState->routeManager(), &RouteManager::stateChanged, this, &RouteEditorPanel::refresh);
}

void RouteEditorPanel::refresh()
{
    QString current = m_routeCombo->currentData().toString();
    m_routeCombo->blockSignals(true);
    m_routeCombo->clear();

    for (const QString &id : m_appState->routeManager()->routeIds())
    {
        const RouteState *rt = m_appState->routeManager()->route(id);
        QString label = id + QStringLiteral(" (%1 WP)").arg(rt ? rt->waypoints.size() : 0);
        m_routeCombo->addItem(label, id);
    }

    int idx = m_routeCombo->findData(current);
    if (idx >= 0)
        m_routeCombo->setCurrentIndex(idx);
    m_routeCombo->blockSignals(false);

    onRouteSelected(m_routeCombo->currentIndex());
}

void RouteEditorPanel::onRouteSelected(int index)
{
    m_waypointList->clear();
    m_distanceLabel->setText(tr("Distance: —"));
    if (index < 0)
        return;

    QString routeId = m_routeCombo->currentData().toString();
    const RouteState *route = m_appState->routeManager()->route(routeId);
    if (!route)
        return;

    // Update loop button
    m_loopBtn->blockSignals(true);
    m_loopBtn->setChecked(route->loopMode);
    m_loopBtn->setText(route->loopMode ? tr("🔁 Loop: ON") : tr("🔁 Loop: OFF"));
    m_loopBtn->blockSignals(false);

    double totalDist = 0.0;
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
        if (wp.speedOverride >= 0)
            text += QStringLiteral(" [%1 m/s]").arg(wp.speedOverride, 0, 'f', 0);
        m_waypointList->addItem(text);

        // Accumulate distance between consecutive waypoints
        if (i > 0)
        {
            const auto &prev = route->waypoints[i - 1];
            totalDist += KinematicModel::haversineDistance(prev.lat, prev.lon, wp.lat, wp.lon);
        }
    }

    // Show total route distance
    if (totalDist > 1000.0)
        m_distanceLabel->setText(QStringLiteral("Distance: %1 km").arg(totalDist / 1000.0, 0, 'f', 1));
    else
        m_distanceLabel->setText(QStringLiteral("Distance: %1 m").arg(totalDist, 0, 'f', 0));
}

void RouteEditorPanel::onNewRoute()
{
    int nextId = m_appState->routeManager()->routeIds().size() + 1;
    RouteState route;
    route.id = QStringLiteral("route_%1").arg(nextId);
    route.loopMode = false;
    m_appState->routeManager()->createRoute(route);
    m_bridge->pushFullSync();

    // Select the new route
    refresh();
    int idx = m_routeCombo->findData(route.id);
    if (idx >= 0)
        m_routeCombo->setCurrentIndex(idx);
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

void RouteEditorPanel::onLoopToggled(bool checked)
{
    QString routeId = m_routeCombo->currentData().toString();
    if (routeId.isEmpty())
        return;

    m_appState->routeManager()->setLoopMode(routeId, checked);
    m_loopBtn->setText(checked ? tr("🔁 Loop: ON") : tr("🔁 Loop: OFF"));
    m_bridge->pushFullSync();
}

void RouteEditorPanel::onMoveUp()
{
    QString routeId = m_routeCombo->currentData().toString();
    int row = m_waypointList->currentRow();
    if (routeId.isEmpty() || row <= 0)
        return;

    m_appState->routeManager()->reorderWaypoint(routeId, row, row - 1);
    m_bridge->pushFullSync();
    // Re-select the moved item
    refresh();
    m_waypointList->setCurrentRow(row - 1);
}

void RouteEditorPanel::onMoveDown()
{
    QString routeId = m_routeCombo->currentData().toString();
    int row = m_waypointList->currentRow();
    const RouteState *rt = m_appState->routeManager()->route(routeId);
    if (routeId.isEmpty() || !rt || row < 0 || row >= rt->waypoints.size() - 1)
        return;

    m_appState->routeManager()->reorderWaypoint(routeId, row, row + 1);
    m_bridge->pushFullSync();
    // Re-select the moved item
    refresh();
    m_waypointList->setCurrentRow(row + 1);
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
