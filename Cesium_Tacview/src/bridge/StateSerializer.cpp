#include "StateSerializer.h"
#include "app/AppState.h"
#include <QJsonArray>
#include <QDateTime>

QJsonObject StateSerializer::serializeFullState(const AppState *state, quint64 tick)
{
    QJsonObject msg;
    msg[QStringLiteral("type")] = QStringLiteral("STATE_FULL_SYNC");
    msg[QStringLiteral("tick")] = static_cast<qint64>(tick);
    msg[QStringLiteral("timestamp")] = QDateTime::currentMSecsSinceEpoch();

    // Aircraft
    QJsonObject acObj;
    const auto &allAc = state->aircraftManager()->allAircraft();
    for (auto it = allAc.cbegin(); it != allAc.cend(); ++it)
    {
        acObj[it.key()] = it.value().toJson();
    }
    msg[QStringLiteral("aircraft")] = acObj;

    // Routes
    QJsonObject rtObj;
    const auto &allRt = state->routeManager()->allRoutes();
    for (auto it = allRt.cbegin(); it != allRt.cend(); ++it)
    {
        rtObj[it.key()] = it.value().toJson();
    }
    msg[QStringLiteral("routes")] = rtObj;

    // Selection (with multi-selection support)
    QJsonObject sel;
    sel[QStringLiteral("entityId")] = state->selectionManager()->selectedEntityId();
    sel[QStringLiteral("type")] = static_cast<int>(state->selectionManager()->selectionType());
    QJsonArray selIds;
    for (const QString &id : state->selectionManager()->selectedEntities())
        selIds.append(id);
    sel[QStringLiteral("selectedIds")] = selIds;
    msg[QStringLiteral("selection")] = sel;

    return msg;
}

QJsonObject StateSerializer::serializeDelta(AppState *state, quint64 tick)
{
    QJsonObject msg;
    msg[QStringLiteral("type")] = QStringLiteral("STATE_DELTA");
    msg[QStringLiteral("tick")] = static_cast<qint64>(tick);
    msg[QStringLiteral("timestamp")] = QDateTime::currentMSecsSinceEpoch();

    QJsonObject acObj;
    for (const QString &id : state->aircraftManager()->aircraftIds())
    {
        AircraftState *ac = state->aircraftManager()->aircraft(id);
        if (!ac || !ac->dirty)
            continue;
        acObj[id] = ac->toRenderDelta();
        ac->dirty = false;
    }
    msg[QStringLiteral("aircraft")] = acObj;

    // Selection always included in delta (with multi-selection)
    QJsonObject sel;
    sel[QStringLiteral("entityId")] = state->selectionManager()->selectedEntityId();
    QJsonArray selIds;
    for (const QString &id : state->selectionManager()->selectedEntities())
        selIds.append(id);
    sel[QStringLiteral("selectedIds")] = selIds;
    msg[QStringLiteral("selection")] = sel;

    return msg;
}

QJsonObject StateSerializer::createCommand(const QString &type, const QJsonObject &payload)
{
    QJsonObject msg;
    msg[QStringLiteral("type")] = type;
    msg[QStringLiteral("payload")] = payload;
    msg[QStringLiteral("timestamp")] = QDateTime::currentMSecsSinceEpoch();
    return msg;
}
