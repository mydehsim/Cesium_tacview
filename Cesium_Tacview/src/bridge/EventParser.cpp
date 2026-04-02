#include "EventParser.h"

CesiumEvent EventParser::parse(const QString &jsonStr)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError)
        return {};
    return parse(doc.object());
}

CesiumEvent EventParser::parse(const QJsonObject &obj)
{
    CesiumEvent ev;
    ev.type = obj.value(QStringLiteral("type")).toString();
    ev.payload = obj.value(QStringLiteral("payload")).toObject();
    ev.timestamp = obj.value(QStringLiteral("timestamp")).toInteger(0);
    return ev;
}
