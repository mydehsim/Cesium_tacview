#include "MapToolbar.h"
#include "app/AppState.h"
#include "bridge/CesiumBridge.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

MapToolbar::MapToolbar(AppState *appState, CesiumBridge *bridge, QWidget *parent)
    : QWidget(parent), m_appState(appState), m_bridge(bridge)
{
    setAttribute(Qt::WA_TranslucentBackground, false);

    // ── Dark semi-transparent style ──
    setStyleSheet(QStringLiteral(
        "MapToolbar {"
        "  background: rgba(15, 20, 30, 230);"
        "  border-radius: 10px;"
        "  border: 1px solid rgba(80, 100, 140, 100);"
        "}"
        "QPushButton {"
        "  background: rgba(50, 60, 80, 200);"
        "  color: #ddd;"
        "  border: 1px solid rgba(120, 140, 180, 100);"
        "  border-radius: 5px;"
        "  padding: 7px 14px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  min-width: 60px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(60, 80, 120, 220);"
        "  color: #fff;"
        "}"
        "QPushButton:checked {"
        "  background: rgba(40, 120, 200, 230);"
        "  color: #fff;"
        "  border: 2px solid #4a9eff;"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(30, 50, 80, 250);"
        "}"
        "QPushButton#startBtn {"
        "  background: rgba(20, 120, 60, 200);"
        "  font-size: 14px;"
        "  min-width: 80px;"
        "}"
        "QPushButton#startBtn:hover {"
        "  background: rgba(30, 150, 80, 230);"
        "}"
        "QPushButton#stopBtn {"
        "  background: rgba(150, 40, 40, 200);"
        "  font-size: 14px;"
        "  min-width: 80px;"
        "}"
        "QPushButton#stopBtn:hover {"
        "  background: rgba(180, 50, 50, 230);"
        "}"
        "QComboBox {"
        "  background: rgba(40, 50, 70, 200);"
        "  color: #eee;"
        "  border: 1px solid rgba(120, 140, 180, 100);"
        "  border-radius: 5px;"
        "  padding: 5px 10px;"
        "  font-size: 13px;"
        "  min-width: 140px;"
        "  min-height: 24px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: rgba(25, 30, 45, 245);"
        "  color: #eee;"
        "  selection-background-color: rgba(40, 120, 200, 200);"
        "  font-size: 13px;"
        "  padding: 2px;"
        "}"
        "QLabel {"
        "  color: #99a;"
        "  font-size: 12px;"
        "}"
        "QLabel#statusLabel {"
        "  color: #8cf;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "}"
        "QFrame#separator {"
        "  background: rgba(100, 120, 160, 80);"
        "  max-width: 1px;"
        "  min-height: 24px;"
        "}"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 10, 12, 8);
    mainLayout->setSpacing(8);

    // ── Row 1: Route/Aircraft selectors + WP Mode ──
    auto *selectorRow = new QHBoxLayout;
    selectorRow->setSpacing(8);

    m_wpModeBtn = new QPushButton(QStringLiteral("📍 WP Mode"));
    m_wpModeBtn->setCheckable(true);
    m_wpModeBtn->setToolTip(tr("Toggle: click map to add waypoints"));

    auto *sep1 = new QFrame;
    sep1->setObjectName(QStringLiteral("separator"));
    sep1->setFrameShape(QFrame::VLine);

    auto *routeLabel = new QLabel(QStringLiteral("Route:"));
    m_routeCombo = new QComboBox;
    m_routeCombo->setToolTip(tr("Select active route"));
    m_routeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_newRouteBtn = new QPushButton(QStringLiteral("+ Route"));
    m_newRouteBtn->setToolTip(tr("Create a new empty route"));

    auto *sep2 = new QFrame;
    sep2->setObjectName(QStringLiteral("separator"));
    sep2->setFrameShape(QFrame::VLine);

    auto *acLabel = new QLabel(QStringLiteral("Aircraft:"));
    m_aircraftCombo = new QComboBox;
    m_aircraftCombo->setToolTip(tr("Select aircraft"));
    m_aircraftCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    selectorRow->addWidget(m_wpModeBtn);
    selectorRow->addWidget(sep1);
    selectorRow->addWidget(routeLabel);
    selectorRow->addWidget(m_routeCombo);
    selectorRow->addWidget(m_newRouteBtn);
    selectorRow->addWidget(sep2);
    selectorRow->addWidget(acLabel);
    selectorRow->addWidget(m_aircraftCombo);
    mainLayout->addLayout(selectorRow);

    // ── Row 2: Action buttons ──
    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(8);

    m_assignBtn = new QPushButton(QStringLiteral("⇒ Assign"));
    m_assignBtn->setToolTip(tr("Assign selected route to selected aircraft"));

    m_clearBtn = new QPushButton(QStringLiteral("✕ Clear"));
    m_clearBtn->setToolTip(tr("Remove all waypoints from current route"));

    auto *sep3 = new QFrame;
    sep3->setObjectName(QStringLiteral("separator"));
    sep3->setFrameShape(QFrame::VLine);

    m_startBtn = new QPushButton(QStringLiteral("▶ Fly"));
    m_startBtn->setObjectName(QStringLiteral("startBtn"));
    m_startBtn->setToolTip(tr("Start aircraft on route (AUTOPILOT)"));

    m_stopBtn = new QPushButton(QStringLiteral("⏹ Stop"));
    m_stopBtn->setObjectName(QStringLiteral("stopBtn"));
    m_stopBtn->setToolTip(tr("Stop aircraft (IDLE)"));

    // Status at right
    m_statusLabel = new QLabel;
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    actionRow->addWidget(m_assignBtn);
    actionRow->addWidget(m_clearBtn);
    actionRow->addWidget(sep3);
    actionRow->addWidget(m_startBtn);
    actionRow->addWidget(m_stopBtn);
    actionRow->addSpacing(12);
    actionRow->addWidget(m_statusLabel);
    mainLayout->addLayout(actionRow);

    // ── Drop shadow ──
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 120));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    // ── Connections ──
    connect(m_wpModeBtn, &QPushButton::clicked, this, &MapToolbar::toggleWaypointMode);
    connect(m_newRouteBtn, &QPushButton::clicked, this, &MapToolbar::onNewRoute);
    connect(m_assignBtn, &QPushButton::clicked, this, &MapToolbar::onAssignRoute);
    connect(m_clearBtn, &QPushButton::clicked, this, &MapToolbar::onClearRoute);
    connect(m_startBtn, &QPushButton::clicked, this, &MapToolbar::onStartAircraft);
    connect(m_stopBtn, &QPushButton::clicked, this, &MapToolbar::onStopAircraft);

    // Sync combos when aircraft/route change
    connect(m_aircraftCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int)
            { updateButtonStates(); });
    connect(m_routeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int)
            { updateButtonStates(); });

    // Auto-refresh on state changes
    connect(appState->aircraftManager(), &AircraftManager::stateChanged, this, &MapToolbar::refresh);
    connect(appState->routeManager(), &RouteManager::stateChanged, this, &MapToolbar::refresh);
    connect(appState->selectionManager(), &SelectionManager::selectionChanged,
            this, [this](const QString &, SelectionType)
            { refresh(); });

    setFixedHeight(95);
}

void MapToolbar::refresh()
{
    // Preserve selections
    QString prevRoute = m_routeCombo->currentData().toString();
    QString prevAc = m_aircraftCombo->currentData().toString();

    // Populate routes
    m_routeCombo->blockSignals(true);
    m_routeCombo->clear();
    for (const QString &id : m_appState->routeManager()->routeIds())
    {
        const RouteState *rt = m_appState->routeManager()->route(id);
        QString label = id + QStringLiteral(" (%1 WP)").arg(rt ? rt->waypoints.size() : 0);
        m_routeCombo->addItem(label, id);
    }
    int ri = m_routeCombo->findData(prevRoute);
    if (ri >= 0)
        m_routeCombo->setCurrentIndex(ri);
    m_routeCombo->blockSignals(false);

    // Populate aircraft
    m_aircraftCombo->blockSignals(true);
    m_aircraftCombo->clear();
    const auto &all = m_appState->aircraftManager()->allAircraft();
    for (auto it = all.cbegin(); it != all.cend(); ++it)
    {
        m_aircraftCombo->addItem(it.value().callSign, it.key());
    }
    // Auto-select currently selected aircraft
    QString sel = m_appState->selectionManager()->selectedEntityId();
    if (!sel.isEmpty())
        prevAc = sel;
    int ai = m_aircraftCombo->findData(prevAc);
    if (ai >= 0)
        m_aircraftCombo->setCurrentIndex(ai);
    m_aircraftCombo->blockSignals(false);

    updateButtonStates();
}

void MapToolbar::updateButtonStates()
{
    QString routeId = m_routeCombo->currentData().toString();
    QString acId = m_aircraftCombo->currentData().toString();
    bool hasRoute = !routeId.isEmpty();
    bool hasAc = !acId.isEmpty();

    m_assignBtn->setEnabled(hasRoute && hasAc);
    m_clearBtn->setEnabled(hasRoute);
    m_startBtn->setEnabled(hasAc);
    m_stopBtn->setEnabled(hasAc);

    // Update status
    if (m_waypointMode && hasRoute)
    {
        const RouteState *rt = m_appState->routeManager()->route(routeId);
        int n = rt ? rt->waypoints.size() : 0;
        m_statusLabel->setText(QStringLiteral("📍 WP Mode ON — %1 (%2 waypoints) — Click map to add")
                                   .arg(routeId)
                                   .arg(n));
    }
    else if (m_waypointMode)
    {
        m_statusLabel->setText(QStringLiteral("📍 WP Mode ON — Create or select a route first"));
    }
    else
    {
        m_statusLabel->setText(QStringLiteral("Select aircraft & route, then click [📍 WP Mode] to add waypoints"));
    }
}

void MapToolbar::toggleWaypointMode()
{
    m_waypointMode = !m_waypointMode;
    m_wpModeBtn->setChecked(m_waypointMode);
    updateButtonStates();

    if (m_waypointMode)
    {
        // Ensure there's a route to add to
        ensureActiveRoute();
        emit logMessage(QStringLiteral("[MapToolbar] Waypoint mode ON"));
    }
    else
    {
        emit logMessage(QStringLiteral("[MapToolbar] Waypoint mode OFF"));
    }
}

QString MapToolbar::ensureActiveRoute()
{
    QString routeId = m_routeCombo->currentData().toString();
    if (!routeId.isEmpty())
        return routeId;

    // Try selected aircraft's route
    QString acId = m_aircraftCombo->currentData().toString();
    if (!acId.isEmpty())
    {
        AircraftState *ac = m_appState->aircraftManager()->aircraft(acId);
        if (ac && !ac->currentRouteId.isEmpty())
        {
            routeId = ac->currentRouteId;
            int idx = m_routeCombo->findData(routeId);
            if (idx >= 0)
                m_routeCombo->setCurrentIndex(idx);
            return routeId;
        }
    }

    // Create new route
    onNewRoute();
    return m_routeCombo->currentData().toString();
}

void MapToolbar::onMapClicked(double lat, double lon, double alt)
{
    if (!m_waypointMode)
        return;

    QString routeId = ensureActiveRoute();
    if (routeId.isEmpty())
        return;

    double wpAlt = (alt < 100.0) ? 5000.0 : alt;

    const RouteState *rt = m_appState->routeManager()->route(routeId);
    int wpNum = rt ? rt->waypoints.size() + 1 : 1;

    Waypoint wp;
    wp.lat = lat;
    wp.lon = lon;
    wp.alt = wpAlt;
    wp.name = QStringLiteral("WP%1").arg(wpNum);

    m_appState->routeManager()->addWaypoint(routeId, wp);
    m_bridge->pushFullSync();

    emit logMessage(QStringLiteral("Added %1 to %2: (%3, %4) @ %5m")
                        .arg(wp.name, routeId)
                        .arg(lat, 0, 'f', 4)
                        .arg(lon, 0, 'f', 4)
                        .arg(wpAlt, 0, 'f', 0));
}

void MapToolbar::onNewRoute()
{
    int nextId = m_appState->routeManager()->routeIds().size() + 1;
    RouteState route;
    route.id = QStringLiteral("route_%1").arg(nextId);
    route.loopMode = false;
    m_appState->routeManager()->createRoute(route);

    // Select the new route in combo
    refresh();
    int idx = m_routeCombo->findData(route.id);
    if (idx >= 0)
        m_routeCombo->setCurrentIndex(idx);

    emit logMessage(QStringLiteral("Created new route: %1").arg(route.id));
}

void MapToolbar::onAssignRoute()
{
    QString routeId = m_routeCombo->currentData().toString();
    QString acId = m_aircraftCombo->currentData().toString();
    if (routeId.isEmpty() || acId.isEmpty())
        return;

    AircraftState *ac = m_appState->aircraftManager()->aircraft(acId);
    if (!ac)
        return;

    ac->currentRouteId = routeId;

    RouteState *route = m_appState->routeManager()->route(routeId);
    if (route)
    {
        route->aircraftId = acId;
        route->currentWaypointIndex = 0;
    }

    m_bridge->pushFullSync();
    emit logMessage(QStringLiteral("Assigned %1 to %2").arg(routeId, ac->callSign));
    updateButtonStates();
}

void MapToolbar::onClearRoute()
{
    QString routeId = m_routeCombo->currentData().toString();
    if (routeId.isEmpty())
        return;

    RouteState *route = m_appState->routeManager()->route(routeId);
    if (!route)
        return;

    route->waypoints.clear();
    route->currentWaypointIndex = 0;
    m_appState->routeManager()->emitChanged();
    m_bridge->pushFullSync();

    emit logMessage(QStringLiteral("Cleared all waypoints from %1").arg(routeId));
}

void MapToolbar::onStartAircraft()
{
    QString acId = m_aircraftCombo->currentData().toString();
    if (acId.isEmpty())
        return;

    AircraftState *ac = m_appState->aircraftManager()->aircraft(acId);
    if (!ac)
        return;

    // If no route assigned, assign the currently selected one
    if (ac->currentRouteId.isEmpty())
    {
        QString routeId = m_routeCombo->currentData().toString();
        if (routeId.isEmpty())
        {
            m_statusLabel->setText(QStringLiteral("⚠ No route to fly — create waypoints first"));
            return;
        }
        ac->currentRouteId = routeId;
        RouteState *route = m_appState->routeManager()->route(routeId);
        if (route)
        {
            route->aircraftId = acId;
            route->currentWaypointIndex = 0;
        }
    }

    // Reset waypoint index to start
    RouteState *route = m_appState->routeManager()->route(ac->currentRouteId);
    if (route)
        route->currentWaypointIndex = 0;

    ac->controlMode = ControlMode::AUTOPILOT;
    ac->targetSpeed = 100.0; // cruise speed
    m_bridge->pushFullSync();

    // Ensure simulation engine is running
    emit requestSimStart();

    emit logMessage(QStringLiteral("▶ %1 flying on %2").arg(ac->callSign, ac->currentRouteId));
    updateButtonStates();
}

void MapToolbar::onStopAircraft()
{
    QString acId = m_aircraftCombo->currentData().toString();
    if (acId.isEmpty())
        return;

    AircraftState *ac = m_appState->aircraftManager()->aircraft(acId);
    if (!ac)
        return;

    ac->controlMode = ControlMode::IDLE;
    ac->targetSpeed = 0;
    m_bridge->pushFullSync();

    emit logMessage(QStringLiteral("⏹ %1 stopped").arg(ac->callSign));
    updateButtonStates();
}
