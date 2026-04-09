#ifndef STATESERIALIZER_H
#define STATESERIALIZER_H

#include <QJsonObject>

class AppState;

class StateSerializer
{
public:
    static QJsonObject serializeFullState(const AppState *state, quint64 tick);
    static QJsonObject serializeDelta(AppState *state, quint64 tick);
    static QJsonObject createCommand(const QString &type, const QJsonObject &payload);
};

#endif // STATESERIALIZER_H
