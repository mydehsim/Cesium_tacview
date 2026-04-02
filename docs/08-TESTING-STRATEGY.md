# Test Stratejisi

## Unit Test (QTest)

- AircraftManager: create/update/remove/getById → doğru state
- KinematicModel: sabit dt=0.05s, bilinen heading/speed → beklenen lat/lon
- StateSerializer: AircraftState → JSON → AircraftState roundtrip
- EventParser: JSON string → typed event struct doğrulama
- AutopilotController: 2 waypoint, N tick → hedefe varış
- SelectionManager: select/deselect/toggle

## Integration Test

- SimulationEngine + AircraftManager → N tick sonra pozisyon doğrulama
- AutopilotController + RouteManager → rota takip akışı
- CesiumBridge + StateSerializer → JSON çıktı kontrolü

## Qt-JS Bridge Test

- Test HTML sayfası (mock Cesium) yükleyerek QWebChannel mesaj gönder/al
- CI'da ayrı stage

## Deterministic Tick

- GEREKLI: Mock clock ile dt=0.05, bilinen input → assert beklenen pozisyon
- SimulationClock inject edilebilir
