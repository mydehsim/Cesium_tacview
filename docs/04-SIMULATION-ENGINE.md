# Simülasyon Motoru

## Neden Ayrı Engine

- Cesium clock render frame rate'e bağlı (~60fps)
- Fizik hesaplamaları sabit tick rate'de olmalı (deterministic)
- Pause/resume/time-scale bağımsız kontrol gerekli

## SimulationEngine Yapısı

```
SimulationEngine (QObject)
├── QTimer tickTimer (20Hz = 50ms)
├── tick(dt) slot:
│   ├── for each aircraft:
│   │   ├── if MANUAL → ManualController::update(dt, inputState)
│   │   ├── if AUTOPILOT → AutopilotController::update(dt, route)
│   │   └── if SCRIPTED → ScriptedController::update(dt, timeline)
│   ├── AircraftManager::updatePositions()
│   ├── checkRouteCompletion()
│   └── emit stateChanged()
├── pause() / resume()
├── setTimeScale(double)
└── setTickRate(int hz)
```

## Kinematik Model (İlk Faz)

```
her tick:
  heading += turnRate * dt
  speed = clamp(speed + accel * dt, 0, maxSpeed)
  altitude += verticalSpeed * dt
  lat += (speed * cos(heading) * dt) / R * (180/π)
  lon += (speed * sin(heading) * dt) / (R * cos(lat*π/180)) * (180/π)
```

## Kontrol Modları

| Mod | Input | Açıklama |
|-----|-------|----------|
| IDLE | Yok | Uçak hareketsiz |
| MANUAL | WASD | Operatör kontrolü |
| AUTOPILOT | Route waypoints | Rota takibi |
| SCRIPTED | Scenario timeline | Senaryo komutu |

## AutopilotController

1. Mevcut pozisyon ile hedef waypoint arasındaki bearing hesapla
2. Heading'i bearing'e doğru turnRate * dt ile döndür
3. Hedefe yaklaşma mesafesi < threshold → sonraki waypoint
4. Son waypoint → loopMode ise başa dön, değilse IDLE

## Tick/Render Decoupling

- SimulationEngine: 20Hz (QTimer), sabit, bağımsız
- Cesium render: ~60fps, GPU bağımlı
- Her sim tick'te delta state push edilir
- ~50ms gecikme kabul edilebilir
