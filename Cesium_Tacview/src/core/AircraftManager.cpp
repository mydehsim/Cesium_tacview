#include "AircraftManager.h"

AircraftManager::AircraftManager(QObject *parent)
    : QObject(parent)
{
}

bool AircraftManager::createAircraft(const AircraftState &state)
{
    if (m_aircraft.contains(state.id))
        return false;

    m_aircraft.insert(state.id, state);
    emit aircraftCreated(state.id);
    emit stateChanged();
    return true;
}

bool AircraftManager::removeAircraft(const QString &id)
{
    if (!m_aircraft.contains(id))
        return false;

    m_aircraft.remove(id);
    emit aircraftRemoved(id);
    emit stateChanged();
    return true;
}

bool AircraftManager::updateAircraft(const AircraftState &state)
{
    if (!m_aircraft.contains(state.id))
        return false;

    m_aircraft[state.id] = state;
    emit aircraftUpdated(state.id);
    return true;
}

AircraftState *AircraftManager::aircraft(const QString &id)
{
    auto it = m_aircraft.find(id);
    return it != m_aircraft.end() ? &it.value() : nullptr;
}

const AircraftState *AircraftManager::aircraft(const QString &id) const
{
    auto it = m_aircraft.find(id);
    return it != m_aircraft.end() ? &it.value() : nullptr;
}

QStringList AircraftManager::aircraftIds() const
{
    return m_aircraft.keys();
}

int AircraftManager::count() const
{
    return m_aircraft.size();
}

const QMap<QString, AircraftState> &AircraftManager::allAircraft() const
{
    return m_aircraft;
}

void AircraftManager::applyPositions(const QMap<QString, AircraftState> &updated)
{
    for (auto it = updated.cbegin(); it != updated.cend(); ++it)
    {
        if (m_aircraft.contains(it.key()))
        {
            auto &ac = m_aircraft[it.key()];
            ac.lat = it.value().lat;
            ac.lon = it.value().lon;
            ac.alt = it.value().alt;
            ac.heading = it.value().heading;
            ac.speed = it.value().speed;
            ac.verticalSpeed = it.value().verticalSpeed;
            ac.addTrailPoint();
        }
    }
    emit stateChanged();
}
