#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>
#include <QString>

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

    QString selectedEntityId() const;
    SelectionType selectionType() const;
    QString activeControlTarget() const;

    void selectEntity(const QString &id, SelectionType type = SelectionType::AIRCRAFT);
    void clearSelection();
    void setActiveControlTarget(const QString &id);
    void clearControlTarget();

signals:
    void selectionChanged(const QString &entityId, SelectionType type);
    void controlTargetChanged(const QString &entityId);

private:
    QString m_selectedEntityId;
    SelectionType m_selectionType = SelectionType::NONE;
    QString m_activeControlTarget;
};

#endif // SELECTIONMANAGER_H
