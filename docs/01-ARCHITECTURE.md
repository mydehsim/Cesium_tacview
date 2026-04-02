# Katmanlı Mimari

> Son güncelleme: 2026-04-02 — Gerçek implementasyonu yansıtır

## Sistem Katmanları

```
┌──────────────────────────────────────────────────────┐
│              Qt Application Shell                     │
│  QMainWindow, QDockWidget, Menüler, Toolbar           │
├───────────────┬──────────────────────────────────────┤
│ Command &     │       Simulation Engine               │
│ Control UI    │  (tick loop, kinematik, autopilot)     │
│ (C++ / Qt)    │                                       │
├───────────────┴──────────────────────────────────────┤
│            Authoritative State Store                  │
│  AircraftManager, RouteManager, ScenarioManager       │
├──────────────────────────────────────────────────────┤
│              Bridge Layer (QWebChannel)                │
│         JSON Command / Event / State Protocol         │
├──────────────────────────────────────────────────────┤
│           Cesium Render Layer (JS)                    │
│  QWebEngineView içinde gömülü CesiumJS app            │
└──────────────────────────────────────────────────────┘
```

## Qt Sorumlulukları
- Pencere yönetimi, iki ekran dağıtımı
- Tüm state modelleri (aircraft, route, scenario, selection)
- SimulationEngine: tick döngüsü, kinematik, kontrol modları
- Klavye/Mouse input yakalama ve yönlendirme
- Senaryo yükleme/kaydetme
- Loglama, debug paneli, komut doğrulama
- Protocol bridge (TCP/UDP/WebSocket/Tacview)

## Cesium Sorumlulukları
- Harita render (terrain, imagery, OSM buildings)
- Entity/model çizimi (Qt state'e göre)
- Kamera kontrolü
- Polyline/trail/waypoint/label render
- Seçili obje vurgulama
- Kullanıcı tıklama/sürükleme → event olarak Qt'ye bildirim
- **Kendi başına state TUTMAZ**

## Event Flow (Cesium → Qt)

```
Kullanıcı Cesium'da entity'ye tıklar
  → JS: InteractionHandler.sendEvent({type:"ENTITY_CLICKED", entityId:"ac_1"})
  → QWebChannel → CesiumBridge::onCesiumEvent(json)
  → SelectionManager::selectAircraft("ac_1")
  → state değişir → signal emit
  → CesiumBridge::pushState() → JSON → Cesium
  → EntityRenderer.updateFromState(json) → vurgulama güncellenir
```

## Command Flow (Qt → Cesium)

```
Operatör "Create Aircraft" butonuna basar
  → AircraftManager::createAircraft(params)
  → state değişir → signal emit
  → CesiumBridge → JSON → Cesium → entity oluşturulur
```
