#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>
#include <QString>
#include <QSet>
#include <QStringList>

enum class SelectionType
{
    NONE,
    AIRCRAFT,
    WAYPOINT
};

class SelectionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString selectedEntityId READ selectedEntityId NOTIFY selectionChanged)
    Q_PROPERTY(QString activeControlTarget READ activeControlTarget NOTIFY controlTargetChanged)

public:
    explicit SelectionManager(QObject *parent = nullptr);

    // Primary selection (single entity — camera follows)
    QString selectedEntityId() const;
    SelectionType selectionType() const;
    QString activeControlTarget() const;

    void selectEntity(const QString &id, SelectionType type = SelectionType::AIRCRAFT);
    void clearSelection();
    void setActiveControlTarget(const QString &id);
    void clearControlTarget();

    // Multi-selection (Ctrl+Click)
    void toggleSelection(const QString &id, SelectionType type = SelectionType::AIRCRAFT);
    void addToSelection(const QString &id, SelectionType type = SelectionType::AIRCRAFT);
    void removeFromSelection(const QString &id);
    QStringList selectedEntities() const;
    bool isSelected(const QString &id) const;
    int selectionCount() const;

signals:
    void selectionChanged(const QString &entityId, SelectionType type);
    void controlTargetChanged(const QString &entityId);
    void multiSelectionChanged(const QStringList &ids);

private:
    QString m_selectedEntityId; // Primary (camera follows)
    SelectionType m_selectionType = SelectionType::NONE;
    QString m_activeControlTarget;
    QSet<QString> m_selectedEntities; // All selected (multi)
};

#endif // SELECTIONMANAGER_H
