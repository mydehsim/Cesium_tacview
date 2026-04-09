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
    // Single-select: clears multi-selection, sets primary
    m_selectedEntities.clear();
    m_selectedEntityId = id;
    m_selectionType = type;
    m_activeControlTarget = id;

    if (!id.isEmpty())
        m_selectedEntities.insert(id);

    emit selectionChanged(id, type);
    emit controlTargetChanged(id);
    emit multiSelectionChanged(selectedEntities());
}

void SelectionManager::clearSelection()
{
    m_selectedEntityId.clear();
    m_selectionType = SelectionType::NONE;
    m_activeControlTarget.clear();
    m_selectedEntities.clear();

    emit selectionChanged(QString(), SelectionType::NONE);
    emit controlTargetChanged(QString());
    emit multiSelectionChanged(QStringList());
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

void SelectionManager::toggleSelection(const QString &id, SelectionType type)
{
    if (m_selectedEntities.contains(id))
    {
        m_selectedEntities.remove(id);
        // If we removed the primary, pick another or clear
        if (m_selectedEntityId == id)
        {
            if (!m_selectedEntities.isEmpty())
            {
                m_selectedEntityId = *m_selectedEntities.constBegin();
                m_activeControlTarget = m_selectedEntityId;
            }
            else
            {
                m_selectedEntityId.clear();
                m_activeControlTarget.clear();
                m_selectionType = SelectionType::NONE;
            }
            emit selectionChanged(m_selectedEntityId, m_selectionType);
            emit controlTargetChanged(m_activeControlTarget);
        }
    }
    else
    {
        m_selectedEntities.insert(id);
        // First item becomes primary if we had none
        if (m_selectedEntityId.isEmpty())
        {
            m_selectedEntityId = id;
            m_selectionType = type;
            m_activeControlTarget = id;
            emit selectionChanged(id, type);
            emit controlTargetChanged(id);
        }
    }
    emit multiSelectionChanged(selectedEntities());
}

void SelectionManager::addToSelection(const QString &id, SelectionType type)
{
    m_selectedEntities.insert(id);
    if (m_selectedEntityId.isEmpty())
    {
        m_selectedEntityId = id;
        m_selectionType = type;
        m_activeControlTarget = id;
        emit selectionChanged(id, type);
        emit controlTargetChanged(id);
    }
    emit multiSelectionChanged(selectedEntities());
}

void SelectionManager::removeFromSelection(const QString &id)
{
    m_selectedEntities.remove(id);
    if (m_selectedEntityId == id)
    {
        if (!m_selectedEntities.isEmpty())
        {
            m_selectedEntityId = *m_selectedEntities.constBegin();
            m_activeControlTarget = m_selectedEntityId;
        }
        else
        {
            m_selectedEntityId.clear();
            m_activeControlTarget.clear();
            m_selectionType = SelectionType::NONE;
        }
        emit selectionChanged(m_selectedEntityId, m_selectionType);
        emit controlTargetChanged(m_activeControlTarget);
    }
    emit multiSelectionChanged(selectedEntities());
}

QStringList SelectionManager::selectedEntities() const
{
    return QStringList(m_selectedEntities.constBegin(), m_selectedEntities.constEnd());
}

bool SelectionManager::isSelected(const QString &id) const
{
    return m_selectedEntities.contains(id);
}

int SelectionManager::selectionCount() const
{
    return m_selectedEntities.size();
}
