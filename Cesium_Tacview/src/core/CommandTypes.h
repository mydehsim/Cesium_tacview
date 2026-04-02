#ifndef COMMANDTYPES_H
#define COMMANDTYPES_H

#include <QString>
#include <QJsonObject>

struct CreateAircraftCmd {
    QString id;
    QString callSign;
    QString type;
    double  lat, lon, alt;
    double  heading;
    QString modelUri;
};

struct RemoveAircraftCmd {
    QString id;
};

struct SetControlModeCmd {
    QString id;
    int     mode; // ControlMode enum cast
};

struct AssignRouteCmd {
    QString aircraftId;
    QString routeId;
};

struct MoveWaypointCmd {
    QString routeId;
    int     waypointIndex;
    double  lat, lon, alt;
};

struct AddWaypointCmd {
    QString routeId;
    double  lat, lon, alt;
    QString name;
};

struct RemoveWaypointCmd {
    QString routeId;
    int     waypointIndex;
};

#endif // COMMANDTYPES_H
