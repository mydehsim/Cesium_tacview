#include "AircraftListPanel.h"
#include "app/AppState.h"
#include "bridge/CesiumBridge.h"

AircraftListPanel::AircraftListPanel(AppState *appState, CesiumBridge *bridge,
                                     QWidget *parent)
    : QDockWidget(tr("Aircraft List"), parent), m_appState(appState), m_bridge(bridge)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget;
    auto *layout = new QVBoxLayout(container);

    m_tree = new QTreeWidget;
    m_tree->setHeaderLabels({tr("ID"), tr("CallSign"), tr("Mode"), tr("Speed"), tr("Alt")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_tree);

    auto *btnLayout = new QHBoxLayout;
    m_createBtn = new QPushButton(tr("+ Create"));
    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    btnLayout->addWidget(m_createBtn);
    btnLayout->addWidget(m_deleteBtn);
    layout->addLayout(btnLayout);

    setWidget(container);

    connect(m_tree, &QTreeWidget::itemClicked, this, &AircraftListPanel::onItemClicked);
    connect(m_createBtn, &QPushButton::clicked, this, &AircraftListPanel::onCreateClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &AircraftListPanel::onDeleteClicked);

    connect(appState->aircraftManager(), &AircraftManager::stateChanged, this, &AircraftListPanel::refresh);
    connect(appState->selectionManager(), &SelectionManager::selectionChanged, this, [this](const QString &, SelectionType)
            { refresh(); });
}

void AircraftListPanel::refresh()
{
    m_tree->clear();
    const auto &all = m_appState->aircraftManager()->allAircraft();
    const QString selected = m_appState->selectionManager()->selectedEntityId();

    for (auto it = all.cbegin(); it != all.cend(); ++it)
    {
        const auto &ac = it.value();
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(0, ac.id);
        item->setText(1, ac.callSign);
        item->setText(2, controlModeToString(ac.controlMode));
        item->setText(3, QString::number(ac.speed, 'f', 1));
        item->setText(4, QString::number(ac.alt, 'f', 0));
        item->setData(0, Qt::UserRole, ac.id);

        if (ac.id == selected)
        {
            item->setSelected(true);
            m_tree->setCurrentItem(item);
        }
    }

    m_deleteBtn->setEnabled(!selected.isEmpty());
}

void AircraftListPanel::onItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    QString id = item->data(0, Qt::UserRole).toString();
    m_appState->selectionManager()->selectEntity(id, SelectionType::AIRCRAFT);
    m_bridge->pushFullSync();
}

void AircraftListPanel::onCreateClicked()
{
    emit createAircraftRequested();
}

void AircraftListPanel::onDeleteClicked()
{
    const QString sel = m_appState->selectionManager()->selectedEntityId();
    if (!sel.isEmpty())
    {
        emit deleteAircraftRequested(sel);
    }
}
