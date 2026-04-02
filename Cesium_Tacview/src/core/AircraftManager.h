#ifndef AIRCRAFTMANAGER_H
#define AIRCRAFTMANAGER_H

#include <QObject>
#include <QMap>
#include "core/AircraftState.h"

class AircraftManager : public QObject
{
    Q_OBJECT
public:
    explicit AircraftManager(QObject *parent = nullptr);

    // CRUD
    bool createAircraft(const AircraftState &state);
    bool removeAircraft(const QString &id);
    bool updateAircraft(const AircraftState &state);

    // Queries
    AircraftState *aircraft(const QString &id);
    const AircraftState *aircraft(const QString &id) const;
    QStringList aircraftIds() const;
    int count() const;
    const QMap<QString, AircraftState> &allAircraft() const;

    // Bulk position update (called per tick)
    void applyPositions(const QMap<QString, AircraftState> &updated);

signals:
    void aircraftCreated(const QString &id);
    void aircraftRemoved(const QString &id);
    void aircraftUpdated(const QString &id);
    void stateChanged();

private:
    QMap<QString, AircraftState> m_aircraft;
    int m_nextId = 1;
};

#endif // AIRCRAFTMANAGER_H
