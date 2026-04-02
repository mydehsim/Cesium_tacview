#ifndef TACVIEWADAPTER_H
#define TACVIEWADAPTER_H

#include "IProtocolAdapter.h"
#include <QObject>
#include <QTcpSocket>
#include <QXmlStreamReader>

class TacviewAdapter : public QObject, public IProtocolAdapter
{
    Q_OBJECT
public:
    explicit TacviewAdapter(QObject *parent = nullptr);
    ~TacviewAdapter() override;

    QString protocolName() const override { return QStringLiteral("Tacview"); }
    bool connect(const QString &host, int port) override;
    void disconnect() override;
    bool isConnected() const override;

    QVector<QJsonObject> parseIncoming(const QByteArray &data) override;
    QByteArray serializeOutgoing(const QJsonObject &stateUpdate) override;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void tracksReceived(const QVector<QJsonObject> &tracks);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onReadyRead();

private:
    QJsonObject parseTrackElement(QXmlStreamReader &xml);
    QVector<QJsonObject> parseWaypoints(QXmlStreamReader &xml);

    QTcpSocket m_socket;
    QByteArray m_buffer;
};

#endif // TACVIEWADAPTER_H
