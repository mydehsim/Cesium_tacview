# Veri Modeli ve State Yönetimi

## State Hiyerarşisi

```
AppState (QObject singleton-benzeri)
├── AircraftManager → QMap<QString, AircraftState>
│   └── AircraftState:
│       - id: QString
│       - callSign: QString
│       - type: QString (Fighter/Transport/Helicopter/Drone)
│       - lat, lon, alt: double
│       - heading: double (derece)
│       - speed: double (m/s)
│       - verticalSpeed: double (m/s)
│       - controlMode: enum (MANUAL, AUTOPILOT, SCRIPTED, IDLE)
│       - currentRouteId: QString
│       - modelUri: QString
│       - visible: bool
│       - trail: QVector<QGeoCoordinate>
│
├── RouteManager → QMap<QString, RouteState>
│   └── RouteState:
│       - id: QString
│       - aircraftId: QString
│       - waypoints: QVector<Waypoint>
│       - color: QColor
│       - visible: bool
│       - loopMode: bool
│       └── Waypoint:
│           - lat, lon, alt: double
│           - name: QString
│           - speedOverride: double (optional)
│
├── SelectionManager
│   - selectedEntityId: QString
│   - activeControlTarget: QString
│   - selectionType: enum (AIRCRAFT, WAYPOINT, NONE)
│
├── ScenarioManager
│   - scenarioName: QString
│   - scenarioState: enum (IDLE, RUNNING, PAUSED, STOPPED)
│
├── CameraState
│   - position: lat, lon, alt
│   - heading, pitch: double
│   - trackedEntityId: QString
│   - mode: enum (FREE, TRACKING, ORBIT)
│
└── SimulationClock
    - currentTick: quint64
    - tickRate: int (Hz, default 20)
    - timeScale: double (1.0 = real-time)
    - isPaused: bool
```

## Seçim Mantığı

- `selectedEntityId` = bilgi gösterilen, UI'da highlight
- `activeControlTarget` = WASD ile kontrol edilen
- İlk fazda ikisi aynı, ileride ayrılabilir

## Multi-Window Sync

- Tek AppState instance, tüm pencereler Qt signal/slot ile beslenirler
- İki Cesium view olursa ikisi de aynı bridge'den aynı state alır
