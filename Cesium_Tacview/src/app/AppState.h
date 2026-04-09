#ifndef APPSTATE_H
#define APPSTATE_H

#include <QObject>
#include "core/AircraftManager.h"
#include "core/RouteManager.h"
#include "core/SelectionManager.h"

class TrackRecorder;
class PlaybackEngine;

enum class AppMode
{
    SIMULATION,
    PLAYBACK
};

class AppState : public QObject
{
    Q_OBJECT
public:
    explicit AppState(QObject *parent = nullptr);

    AircraftManager *aircraftManager() { return m_aircraftMgr; }
    RouteManager *routeManager() { return m_routeMgr; }
    SelectionManager *selectionManager() { return m_selectionMgr; }
    TrackRecorder *trackRecorder() { return m_trackRecorder; }
    PlaybackEngine *playbackEngine() { return m_playbackEngine; }

    const AircraftManager *aircraftManager() const { return m_aircraftMgr; }
    const RouteManager *routeManager() const { return m_routeMgr; }
    const SelectionManager *selectionManager() const { return m_selectionMgr; }
    const TrackRecorder *trackRecorder() const { return m_trackRecorder; }

    AppMode mode() const { return m_mode; }
    void setMode(AppMode mode);

    void clearAll();

signals:
    void fullSyncRequired();
    void modeChanged(AppMode mode);

private:
    AircraftManager *m_aircraftMgr;
    RouteManager *m_routeMgr;
    SelectionManager *m_selectionMgr;
    TrackRecorder *m_trackRecorder;
    PlaybackEngine *m_playbackEngine;
    AppMode m_mode = AppMode::SIMULATION;
};

#endif // APPSTATE_H
