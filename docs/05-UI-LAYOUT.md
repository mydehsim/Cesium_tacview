# İki Ekran Düzeni ve Paneller

## Monitör 1 (Ana Ekran)

```
┌──────────┬─────────────────────────────────┐
│ Aircraft │                                 │
│ List     │      CesiumJS Map View          │
│ (Dock)   │      (QWebEngineView)           │
├──────────┤                                 │
│ Route    │                                 │
│ Editor   │                                 │
│ (Dock)   │                                 │
└──────────┴─────────────────────────────────┘
│ StatusBar: selected | sim time | tick# | fps │
```

## Monitör 2 (Debug/Komuta)

```
┌──────────────────────────┬─────────────────┐
│ Aircraft State Inspector │ Command History │
├──────────────────────────┤                 │
│ Simulation Log           │                 │
├──────────────────────────┼─────────────────┤
│ Network/Protocol Log     │ Hotkeys Ref     │
├──────────────────────────┴─────────────────┤
│ Scenario Panel                             │
└────────────────────────────────────────────┘
```

## Pencere Stratejisi

- Tek QApplication, iki QMainWindow
- Ana pencere: Cesium + sol dock'lar
- Debug pencere: Panel odaklı QDockWidget'lar
- QScreen API ile ikinci monitör algılama

## QDockWidget Panelleri

1. AircraftListPanel — QTreeView + model, context menu
2. AircraftInspector — QFormLayout, canlı state, editable
3. RouteEditorPanel — QListWidget waypoints, drag-drop, Apply
4. SimulationLogPanel — QPlainTextEdit, filtreleme
5. CommandHistoryPanel — Son N komut
6. NetworkLogPanel — TCP/WS mesaj logu
7. ScenarioPanel — Yükle/başlat/durdur
8. HotkeysPanel — Kısayol referans

## Keyboard Mapping

| Tuş | Aksiyon |
|-----|---------|
| W/↑ | Speed artır |
| S/↓ | Speed azalt |
| A/← | Sol dön |
| D/→ | Sağ dön |
| Q | Yüksel |
| E | Alçal |
| Space | AUTOPILOT↔MANUAL |
| Tab | Sonraki uçağa geç |
| F | Kamerayı seçili uçağa odakla |
| P | Pause/Resume |
| Escape | Seçimi kaldır |
| Ctrl+N | Yeni uçak oluştur |
| Delete | Seçili uçağı sil |
