#ifndef EVENTPARSER_H
#define EVENTPARSER_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

struct CesiumEvent
{
    QString type;
    QJsonObject payload;
    qint64 timestamp = 0;

    bool isValid() const { return !type.isEmpty(); }
};

class EventParser
{
public:
    static CesiumEvent parse(const QString &jsonStr);
    static CesiumEvent parse(const QJsonObject &obj);
};

#endif // EVENTPARSER_H
