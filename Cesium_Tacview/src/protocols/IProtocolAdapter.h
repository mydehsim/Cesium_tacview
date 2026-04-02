#ifndef IPROTOCOLADAPTER_H
#define IPROTOCOLADAPTER_H

#include <QByteArray>
#include <QVector>
#include <QJsonObject>

class IProtocolAdapter
{
public:
    virtual ~IProtocolAdapter() = default;

    virtual QString protocolName() const = 0;
    virtual bool connect(const QString &host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Parse incoming data into state update messages
    virtual QVector<QJsonObject> parseIncoming(const QByteArray &data) = 0;

    // Serialize outgoing state for this protocol
    virtual QByteArray serializeOutgoing(const QJsonObject &stateUpdate) = 0;
};

#endif // IPROTOCOLADAPTER_H
