# Klasör ve Proje Yapısı

> Son güncelleme: 2026-04-02 — Gerçek dosya envanteriyle eşleştirildi

## Durum Açıklaması

- ✅ = Dosya mevcut ve derleniyor / çalışıyor
- 🔲 = Planlandı ama henüz oluşturulmadı
- ⚡ = Mevcut ama henüz bridge'e bağlanmadı (standalone çalışıyor)

```
Qt_Cesium_Tacview/
├── docs/                             # Mimari dokümantasyon (10 dosya)
│
├── Cesium_Tacview/                   # Qt/C++ projesi
│   ├── CMakeLists.txt                # ✅ Qt6 Widgets + WebEngine + WebChannel
│   ├── CMakePresets.json             # ✅ dev-debug, dev-release presetleri
│   │
│   └── src/
│       ├── main.cpp                  # ✅ QApplication + GPU flags + remote debug
│       │
│       ├── app/
│       │   ├── MainWindow.h/cpp      # ✅ Ana pencere, CesiumPage subclass, demo senaryo
│       │   ├── DebugWindow.h/cpp     # ✅ İkinci ekran penceresi (iskelet)
│       │   └── AppState.h/cpp        # ✅ Merkezi state container
│       │
│       ├── core/
│       │   ├── AircraftState.h       # ✅ Struct + ControlMode enum + helpers
│       │   ├── AircraftManager.h/cpp # ✅ CRUD + signals
│       │   ├── RouteState.h          # ✅ Route + Waypoint structs
│       │   ├── RouteManager.h/cpp    # ✅ Route CRUD + signals
│       │   ├── SelectionManager.h/cpp# ✅ selectedEntity + activeControlTarget
│       │   └── CommandTypes.h        # ✅ Command structs (CreateAircraft, MoveWaypoint, etc.)
│       │
│       ├── simulation/
│       │   ├── SimulationEngine.h/cpp# ✅ 20Hz QTimer tick loop
│       │   ├── SimulationClock.h/cpp # ✅ Tick counter, timeScale, pause
│       │   ├── KinematicModel.h/cpp  # ✅ Basit pozisyon hesaplama
│       │   ├── ManualController.h/cpp# ✅ InputState → heading/speed/alt target
│       │   └── AutopilotController.h/cpp # ✅ Waypoint takip mantığı
│       │
│       ├── bridge/
│       │   ├── CesiumBridge.h/cpp    # ✅ QWebChannel + qwebchannel.js inject
│       │   ├── StateSerializer.h/cpp # ✅ AppState → JSON (full + delta)
│       │   └── EventParser.h/cpp     # ✅ JSON → typed C++ event struct
│       │
│       ├── ui/
│       │   ├── AircraftListPanel.h/cpp   # ✅ QDockWidget + QTreeWidget + Create/Delete
│       │   ├── AircraftInspector.h/cpp   # ✅ QFormLayout, seçili uçak state
│       │   ├── RouteEditorPanel.h/cpp    # ✅ Route combo + waypoint list + Apply
│       │   ├── SimulationLogPanel.h/cpp  # ✅ QPlainTextEdit canlı log
│       │   ├── CommandHistoryPanel.h/cpp # ✅ Son komutlar listesi
│       │   ├── NetworkLogPanel.h/cpp     # 🔲 Faz 4
│       │   ├── ScenarioPanel.h/cpp       # 🔲 Faz 4
│       │   └── HotkeysPanel.h/cpp        # 🔲 Faz 4
│       │
│       ├── input/
│       │   ├── InputManager.h/cpp    # ✅ Key dispatch + currentInputState()
│       │   └── KeyBindings.h         # ✅ Tuş → aksiyon mapping (header-only)
│       │
│       └── protocols/
│           ├── IProtocolAdapter.h    # ✅ Interface tanımı
│           ├── XmlTrackAdapter.h/cpp # 🔲 Faz 5
│           └── TacviewAdapter.h/cpp  # 🔲 Faz 5
│
├── cesium_web/                       # CesiumJS uygulaması
│   ├── index.html                    # ✅ Orijinal (tüm HTML panelleri dahil)
│   ├── index-qt.html                 # ✅ Sadeleştirilmiş (yalnızca cesiumContainer)
│   ├── package.json                  # ✅ CesiumJS 1.130.0 + Vite 5
│   ├── vite.config.js                # ✅ viteStaticCopy + Cesium assets
│   │
│   └── src/
│       ├── main.js                   # ⚡ Orijinal monolitik (~2100 satır, standalone)
│       ├── main-qt.js                # ✅ Qt modu bootstrap (bridge + modüller)
│       ├── style.css                 # ✅ Cesium UI stilleri
│       │
│       └── modules/
│           ├── CesiumApp.js          # ✅ Viewer init + terrain + imagery
│           ├── CesiumBridge.js       # ✅ QWebChannel bağlantısı
│           ├── EntityRenderer.js     # ✅ State → entity CRUD
│           ├── InteractionHandler.js # ✅ Click/drag → event
│           ├── CameraController.js   # ✅ Kamera komutları
│           │
│           ├── tcp/
│           │   ├── TcpClient.js      # ⚡ Mevcut (standalone modda kullanılır)
│           │   ├── TcpConfig.js      # ⚡ Mevcut (standalone modda kullanılır)
│           │   └── XmlBuilder.js     # ⚡ Mevcut (Tacview referansı)
│           │
│           └── ui/
│               ├── ControlPanel.js   # ⚡ Mevcut (standalone modda kullanılır)
│               └── TrackVisualizer.js# ⚡ Mevcut (EntityRenderer'ın temeli)
│
├── build-qt/                         # ✅ CMake build output (Ninja)
│
├── .vscode/
│   ├── settings.json                 # ✅ CMake Tools entegrasyonu
│   ├── launch.json                   # ✅ Debug C++ + JS + Combined
│   ├── tasks.json                    # ✅ Build + Vite + full-dev
│   └── extensions.json               # ✅ Önerilen eklentiler
│
├── scenarios/                        # 🔲 Faz 4
└── tests/                            # 🔲 Faz 3+
```

## CMake Yapısı

- `find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebChannel)`
- `target_link_libraries(... Qt6::Widgets Qt6::WebEngineWidgets Qt6::WebChannel)`
- `file(GLOB_RECURSE ...)` ile otomatik kaynak dosya toplama
- `WEB_CONTENT_PATH` define ile cesium_web dizini C++'dan erişilebilir
- `CMakePresets.json` ile `CMAKE_PREFIX_PATH`, generator, build dizini tanımlı

## Derleme/Çalıştırma

```bash
# CMake configure (otomatik vcvars + Ninja)
cmake --preset dev-debug           # Cesium_Tacview/ dizininde

# Build
cmake --build ../build-qt --parallel

# Vite dev server (ayrı terminalde)
cd cesium_web && npm run dev

# Çalıştır
../build-qt/Cesium_Tacview.exe
```
