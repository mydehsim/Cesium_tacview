#include "SelectionManager.h"

SelectionManager::SelectionManager(QObject *parent)
    : QObject(parent)
{
}

QString SelectionManager::selectedEntityId() const
{
    return m_selectedEntityId;
}

SelectionType SelectionManager::selectionType() const
{
    return m_selectionType;
}

QString SelectionManager::activeControlTarget() const
{
    return m_activeControlTarget;
}

void SelectionManager::selectEntity(const QString &id, SelectionType type)
{
    if (m_selectedEntityId == id && m_selectionType == type)
        return;

    m_selectedEntityId = id;
    m_selectionType = type;

    // In initial phase, control target follows selection
    m_activeControlTarget = id;

    emit selectionChanged(id, type);
    emit controlTargetChanged(id);
}

void SelectionManager::clearSelection()
{
    m_selectedEntityId.clear();
    m_selectionType = SelectionType::NONE;
    m_activeControlTarget.clear();

    emit selectionChanged(QString(), SelectionType::NONE);
    emit controlTargetChanged(QString());
}

void SelectionManager::setActiveControlTarget(const QString &id)
{
    if (m_activeControlTarget == id)
        return;
    m_activeControlTarget = id;
    emit controlTargetChanged(id);
}

void SelectionManager::clearControlTarget()
{
    m_activeControlTarget.clear();
    emit controlTargetChanged(QString());
}
