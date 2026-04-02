#include "TacviewAdapter.h"
#include <QXmlStreamWriter>
#include <QJsonArray>

TacviewAdapter::TacviewAdapter(QObject *parent)
    : QObject(parent)
{
    QObject::connect(&m_socket, &QTcpSocket::connected,
                     this, &TacviewAdapter::onSocketConnected);
    QObject::connect(&m_socket, &QTcpSocket::disconnected,
                     this, &TacviewAdapter::onSocketDisconnected);
    QObject::connect(&m_socket, &QTcpSocket::readyRead,
                     this, &TacviewAdapter::onReadyRead);
    QObject::connect(&m_socket, &QAbstractSocket::errorOccurred,
                     this, &TacviewAdapter::onSocketError);
}

TacviewAdapter::~TacviewAdapter()
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
}

bool TacviewAdapter::connect(const QString &host, int port)
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        return false;

    m_socket.connectToHost(host, static_cast<quint16>(port));
    return m_socket.waitForConnected(3000);
}

void TacviewAdapter::disconnect()
{
    m_socket.disconnectFromHost();
}

bool TacviewAdapter::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

// ── incoming XML → QJsonObject list ──

QVector<QJsonObject> TacviewAdapter::parseIncoming(const QByteArray &data)
{
    QVector<QJsonObject> results;

    QXmlStreamReader xml(data);
    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("Track"))
        {
            results.append(parseTrackElement(xml));
        }
    }
    return results;
}

QJsonObject TacviewAdapter::parseTrackElement(QXmlStreamReader &xml)
{
    QJsonObject track;
    QJsonArray waypoints;

    while (!(xml.isEndElement() && xml.name() == QLatin1String("Track")))
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            const auto name = xml.name();
            if (name == QLatin1String("Number"))
                track[QStringLiteral("id")] = xml.readElementText();
            else if (name == QLatin1String("CallSign"))
                track[QStringLiteral("callSign")] = xml.readElementText();
            else if (name == QLatin1String("Type"))
                track[QStringLiteral("type")] = xml.readElementText();
            else if (name == QLatin1String("Color"))
                track[QStringLiteral("color")] = xml.readElementText();
            else if (name == QLatin1String("Waypoints"))
            {
                auto wps = parseWaypoints(xml);
                QJsonArray arr;
                for (const auto &wp : wps)
                    arr.append(wp);
                track[QStringLiteral("waypoints")] = arr;
            }
            else if (name == QLatin1String("Latitude"))
                track[QStringLiteral("lat")] = xml.readElementText().toDouble();
            else if (name == QLatin1String("Longitude"))
                track[QStringLiteral("lon")] = xml.readElementText().toDouble();
            else if (name == QLatin1String("Altitude"))
                track[QStringLiteral("alt")] = xml.readElementText().toDouble();
            else if (name == QLatin1String("Heading"))
                track[QStringLiteral("heading")] = xml.readElementText().toDouble();
            else if (name == QLatin1String("Speed"))
                track[QStringLiteral("speed")] = xml.readElementText().toDouble();
        }
    }
    return track;
}

QVector<QJsonObject> TacviewAdapter::parseWaypoints(QXmlStreamReader &xml)
{
    QVector<QJsonObject> wps;
    while (!(xml.isEndElement() && xml.name() == QLatin1String("Waypoints")))
    {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QLatin1String("Waypoint"))
        {
            QJsonObject wp;
            while (!(xml.isEndElement() && xml.name() == QLatin1String("Waypoint")))
            {
                xml.readNext();
                if (xml.isStartElement())
                {
                    const auto n = xml.name();
                    if (n == QLatin1String("Latitude"))
                        wp[QStringLiteral("lat")] = xml.readElementText().toDouble();
                    else if (n == QLatin1String("Longitude"))
                        wp[QStringLiteral("lon")] = xml.readElementText().toDouble();
                    else if (n == QLatin1String("Altitude"))
                        wp[QStringLiteral("alt")] = xml.readElementText().toDouble();
                    else if (n == QLatin1String("Name"))
                        wp[QStringLiteral("name")] = xml.readElementText();
                }
            }
            wps.append(wp);
        }
    }
    return wps;
}

// ── outgoing QJsonObject → XML ──

QByteArray TacviewAdapter::serializeOutgoing(const QJsonObject &stateUpdate)
{
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QStringLiteral("Tracks"));

    xml.writeStartElement(QStringLiteral("Track"));

    auto writeEl = [&](const QString &tag, const QString &val) {
        if (!val.isEmpty())
            xml.writeTextElement(tag, val);
    };
    auto writeDbl = [&](const QString &tag, double val) {
        xml.writeTextElement(tag, QString::number(val, 'f', 6));
    };

    writeEl(QStringLiteral("Number"), stateUpdate.value(QStringLiteral("id")).toString());
    writeEl(QStringLiteral("CallSign"), stateUpdate.value(QStringLiteral("callSign")).toString());
    writeEl(QStringLiteral("Type"), stateUpdate.value(QStringLiteral("type")).toString());
    writeDbl(QStringLiteral("Latitude"), stateUpdate.value(QStringLiteral("lat")).toDouble());
    writeDbl(QStringLiteral("Longitude"), stateUpdate.value(QStringLiteral("lon")).toDouble());
    writeDbl(QStringLiteral("Altitude"), stateUpdate.value(QStringLiteral("alt")).toDouble());
    writeDbl(QStringLiteral("Heading"), stateUpdate.value(QStringLiteral("heading")).toDouble());
    writeDbl(QStringLiteral("Speed"), stateUpdate.value(QStringLiteral("speed")).toDouble());

    // Waypoints
    QJsonArray wps = stateUpdate.value(QStringLiteral("waypoints")).toArray();
    if (!wps.isEmpty())
    {
        xml.writeStartElement(QStringLiteral("Waypoints"));
        for (const auto &wpVal : wps)
        {
            QJsonObject wp = wpVal.toObject();
            xml.writeStartElement(QStringLiteral("Waypoint"));
            writeDbl(QStringLiteral("Latitude"), wp.value(QStringLiteral("lat")).toDouble());
            writeDbl(QStringLiteral("Longitude"), wp.value(QStringLiteral("lon")).toDouble());
            writeDbl(QStringLiteral("Altitude"), wp.value(QStringLiteral("alt")).toDouble());
            writeEl(QStringLiteral("Name"), wp.value(QStringLiteral("name")).toString());
            xml.writeEndElement(); // Waypoint
        }
        xml.writeEndElement(); // Waypoints
    }

    xml.writeEndElement(); // Track
    xml.writeEndElement(); // Tracks
    xml.writeEndDocument();

    return out;
}

// ── socket slots ──

void TacviewAdapter::onSocketConnected()
{
    m_buffer.clear();
    emit connected();
}

void TacviewAdapter::onSocketDisconnected()
{
    m_buffer.clear();
    emit disconnected();
}

void TacviewAdapter::onSocketError(QAbstractSocket::SocketError /*err*/)
{
    emit errorOccurred(m_socket.errorString());
}

void TacviewAdapter::onReadyRead()
{
    m_buffer.append(m_socket.readAll());

    // Simple framing: complete XML document ends with </Tracks>\n
    while (m_buffer.contains("</Tracks>"))
    {
        int end = m_buffer.indexOf("</Tracks>") + 9;
        QByteArray frame = m_buffer.left(end);
        m_buffer.remove(0, end);

        // Trim leading whitespace/newlines before <?xml
        int xmlStart = frame.indexOf("<?xml");
        if (xmlStart > 0)
            frame = frame.mid(xmlStart);

        auto tracks = parseIncoming(frame);
        if (!tracks.isEmpty())
            emit tracksReceived(tracks);
    }
}
