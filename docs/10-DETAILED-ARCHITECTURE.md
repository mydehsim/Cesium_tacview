# 10 — Detaylı Mimari & Performans Analizi

> **Son güncelleme:** 9 Nisan 2026  
> **Toplam kod:** ~8 000 satır C++ · ~2 000 satır JavaScript  
> **Derleme süresi:** ~30-60 s (bağımlılıklar cache'lendikten sonra)  
> **Çalışma zamanı belleği:** ~400–600 MB (Chromium tabanı + veriler)

---

## 1. Genel Bakış

```
┌───────────────────────────────────────────────────────────────────────────────┐
│                          Cesium Tacview — Uygulama                           │
│                                                                              │
│  ┌──────────── C++ (Qt 6.8.3) ────────────┐  ┌─── JavaScript (CesiumJS) ──┐ │
│  │                                         │  │                            │ │
│  │   MainWindow ← UI Panelleri (Dock)      │  │  CesiumApp (Viewer init)   │ │
│  │   AppState   ← Merkezi durum            │  │  EntityRenderer (3D obje)  │ │
│  │   SimEngine  ← 20 Hz fizik döngüsü      │  │  CameraController          │ │
│  │   CesiumBridge ←→ JSON IPC              │  │  InteractionHandler        │ │
│  │                                         │  │  CesiumBridge.js           │ │
│  │   MapWindow  ← QWebEngineView           │──│  (QWebChannel bağlantısı)  │ │
│  │                                         │  │                            │ │
│  └─────────────────────────────────────────┘  └────────────────────────────┘ │
│                                                                              │
│  ┌── Ayrı İşlem ──┐                                                         │
│  │ ViteProcess     │ ← npm run dev (node.js, localhost:5173)                 │
│  └─────────────────┘                                                         │
└───────────────────────────────────────────────────────────────────────────────┘
```

**Temel ilke:** Qt C++ **otoriter durum sahibidir** (authoritative state owner). JavaScript tarafı saf bir **görüntüleme kölesidir** (render slave). Tüm iş mantığı, fizik, seçim, rota yönetimi C++ tarafında yapılır; JS yalnızca 3D sahneyi günceller.

---

## 2. Dizin Yapısı & Dosya Haritası

```
Qt_Cesium_Tacview/
├── Cesium_Tacview/                  ← C++ kaynak kodu
│   ├── CMakeLists.txt               ← Build tanımı (Qt6 Widgets+WebEngine+WebChannel+Network)
│   ├── CMakePresets.json            ← dev-debug / build-debug / build-release preset'leri
│   └── src/
│       ├── main.cpp                 ← Giriş noktası, Chromium GPU bayrakları
│       ├── app/                     ← Uygulama katmanı (pencereler, durum, Vite yönetimi)
│       ├── bridge/                  ← Qt ↔ JS iletişim katmanı (QWebChannel + JSON)
│       ├── core/                    ← Veri modelleri & yöneticiler
│       ├── input/                   ← Klavye girişi yönetimi
│       ├── protocols/               ← Protokol adaptörleri (gelecek için)
│       ├── simulation/              ← Fizik motoru, otopilot, kayıt/oynatma
│       └── ui/                      ← Qt dock panelleri
│
├── cesium_web/                      ← JavaScript (Vite + CesiumJS)
│   ├── index-qt.html                ← Qt'nin yüklediği HTML (qwebchannel.js dahil)
│   ├── index.html                   ← Tarayıcı test modu
│   ├── vite.config.js               ← Vite dev sunucu yapılandırması (:5173)
│   ├── package.json                 ← npm bağımlılıkları
│   ├── public/
│   │   ├── qwebchannel.js           ← Qt6 WebChannel JS kütüphanesi
│   │   ├── models/                  ← 3D glTF modelleri (F-16, helikopter vs.)
│   │   └── tiles/                   ← Çevrimdışı harita karoları ({z}/{x}/{y}.jpg)
│   └── src/
│       ├── main-qt.js               ← Qt gömülü mod giriş noktası
│       ├── main.js                  ← Bağımsız tarayıcı modu
│       └── modules/
│           ├── CesiumApp.js          ← Viewer başlatma & performans ayarları
│           ├── CesiumBridge.js       ← QWebChannel bağlantısı
│           ├── EntityRenderer.js     ← 3D uçak/rota/trail çizimi + interpolasyon
│           ├── CameraController.js   ← Kamera komutları
│           ├── InteractionHandler.js ← Fare tıklama/sürükleme olayları
│           └── tcp/                  ← TCP istemcisi (yalnız standalone modda)
│
├── docs/                            ← Proje belgeleri
├── scripts/                         ← Yardımcı araçlar (tile indirme vb.)
├── build-qt/                        ← CMake build çıktısı (Ninja)
└── build-Cesium_Tacview-*/          ← Qt Creator build çıktısı
```

---

## 3. C++ Dosya Detayları

### 3.1 Uygulama Katmanı (`src/app/`)

| Dosya | Sınıf | Sorumluluk |
|-------|-------|------------|
| `main.cpp` | — | Chromium GPU bayrakları, `Qt::AA_ShareOpenGLContexts`, QApplication |
| `MainWindow.h/cpp` | `MainWindow` | Ana pencere: tüm yöneticileri oluşturur, menüler, paneller, sinyal bağlantıları |
| `MapWindow.h/cpp` | `MapWindow` | QWebEngineView'ü barındırır, CesiumBridge'i sayfaya bağlar, toolbar |
| `AppState.h/cpp` | `AppState` | Merkezi durum: AircraftManager, RouteManager, SelectionManager, TrackRecorder, PlaybackEngine |
| `ViteProcess.h/cpp` | `ViteProcess` | `npm run dev` sürecini başlatır, 500ms aralıklarla HTTP yoklamayla hazır olana kadar bekler (maks. 90s) |

**Başlatma sırası:**
```
main() → MainWindow()
  ├─ AppState oluştur (tüm yöneticileri içerir)
  ├─ CesiumBridge oluştur (QWebChannel kaydı)
  ├─ SimulationEngine oluştur
  ├─ MapWindow oluştur (QWebEngineView + bridge.attachToPage)
  ├─ UI panellerini oluştur (dock widget'lar)
  ├─ Sinyal/slot bağlantıları kur
  ├─ ViteProcess.start() → npm run dev
  │    └─ pollServer() her 500ms → HTTP GET localhost:5173
  │         └─ 200 OK → emit ready()
  ├─ onViteReady() → MapWindow::loadCesium()
  │    └─ setUrl("http://localhost:5173/index-qt.html")
  └─ setupDualScreen() → pencereleri konumlandır
```

### 3.2 Köprü Katmanı (`src/bridge/`)

| Dosya | Sınıf | Sorumluluk |
|-------|-------|------------|
| `CesiumBridge.h/cpp` | `CesiumBridge` | QWebChannel üzerinden JSON gönder/al, `pushFullSync()`, `pushDelta()`, `pushCommand()` |
| `StateSerializer.h/cpp` | `StateSerializer` | AppState → JSON serileştirme (`STATE_FULL_SYNC`, `STATE_DELTA`) |
| `EventParser.h/cpp` | `EventParser` | JS'den gelen JSON olaylarını `CesiumEvent` struct'ına ayrıştır |

**Veri akışı (her 50ms tick):**
```
SimulationEngine::tick()  →  emit tickCompleted(tick)
                                   ↓
MainWindow slot  →  CesiumBridge::pushDelta()
                          ↓
StateSerializer::serializeDelta()       → QJsonObject oluştur
  ├─ Her uçak için: AircraftState::toRenderDelta()  (8 alan: lat,lon,alt,heading,speed,vs,roll,pitch)
  └─ Selection bilgisi ekle
                          ↓
QJsonDocument::toJson() → QString (UTF-8 JSON metni)
                          ↓
emit sendToCesium(jsonStr)  →  QWebChannel IPC  →  JavaScript
```

### 3.3 Çekirdek Katman (`src/core/`)

| Dosya | Sınıf/Struct | Sorumluluk |
|-------|-------------|------------|
| `AircraftState.h` | `AircraftState` | 6-DOF uçak durumu: konum, hız, tutum, iz (trail), kontrol modu |
| `RouteState.h` | `RouteState`, `Waypoint` | Rota + yol noktaları, renk, döngü modu |
| `AircraftManager.h/cpp` | `AircraftManager` | `QMap<QString, AircraftState>` CRUD yönetimi |
| `RouteManager.h/cpp` | `RouteManager` | `QMap<QString, RouteState>` CRUD yönetimi |
| `SelectionManager.h/cpp` | `SelectionManager` | Tekli/çoklu seçim, aktif kontrol hedefi |
| `ScenarioManager.h/cpp` | `ScenarioManager` | `.tacscen` dosya kaydet/yükle (JSON) |
| `CommandTypes.h` | Çeşitli | Komut yapıları (gelecek kullanım) |

**AircraftState temel alanları:**
```cpp
struct AircraftState {
    QString id, callSign, type;
    double lat, lon, alt;           // WGS-84 konum
    double heading, speed;          // Gerçek başlık (°), m/s
    double verticalSpeed;           // m/s
    double roll, pitch, yaw;        // Tutum (°)
    double magneticHeading;         // Manyetik deklinasyon dahil
    double groundTrack, groundSpeed;
    double turnRate, gLoad;         // Dönüş hızı (°/s), G yükü
    ControlMode controlMode;        // IDLE / MANUAL / AUTOPILOT / SCRIPTED
    QString currentRouteId;
    QVector<TrailPoint> trail;      // Son 500 nokta (circular buffer)
};
```

### 3.4 Simülasyon Katmanı (`src/simulation/`)

| Dosya | Sınıf | Sorumluluk |
|-------|-------|------------|
| `SimulationEngine.h/cpp` | `SimulationEngine` | Ana tick döngüsü (QTimer, 20 Hz), her uçağı günceller |
| `SimulationClock.h/cpp` | `SimulationClock` | Saat yönetimi: tickRate, timeScale, dt hesaplama |
| `KinematicModel.h/cpp` | `KinematicModel` | 6-DOF kinematik: büyük daire konum integrasyonu, bank açısı, manyetik deklinasyon |
| `ManualController.h/cpp` | `ManualController` | WASD girişinden heading/speed/altitude güncelleme |
| `AutopilotController.h/cpp` | `AutopilotController` | Rota takibi: yol noktasına yönelme, varış kontrolü (200m eşik) |
| `PlaybackEngine.h/cpp` | `PlaybackEngine` | Kayıt oynatma: play/pause/seek, 0.1x–16x hız çarpanı |
| `TrackRecorder.h/cpp` | `TrackRecorder` | Simülasyon anlık görüntülerini kaydet (maks 36000 ≈ 30 dk @ 20Hz) |

**Simülasyon döngüsü (her 50ms):**
```
SimulationEngine::tick()
  ├─ SimulationClock::advance()       ← tick sayacı artır, dt hesapla
  ├─ Her uçak için:
  │   ├─ MANUAL mod:  ManualController::update(ac, dt, inputState)
  │   ├─ AUTOPILOT:   AutopilotController::update(ac, route, dt)
  │   ├─ SCRIPTED:    (yalnızca kinematik)
  │   └─ KinematicModel::update(ac, dt)  ← konum integrasyon
  │       ├─ Heading rate limiting (45°/s)
  │       ├─ Bank açısı = atan(v² / (r·g))
  │       ├─ Büyük daire konum güncellemesi
  │       └─ Manyetik deklinasyon (WMM 2025)
  ├─ addTrailPoint() → circular buffer (500 nokta)
  └─ emit tickCompleted(tick)
```

**Fizik sabitleri:**
| Sabit | Değer | Açıklama |
|-------|-------|----------|
| `TICK_RATE` | 20 Hz | Simülasyon güncelleme hızı |
| `MAX_SPEED` | 1000 m/s | Mach ~3 sınırı |
| `BANK_RATE` | 40°/s | Maksimum yatış değişim hızı |
| `HEADING_CHANGE_RATE` | 45°/s | Manuel mod başlık değişim hızı |
| `PITCH_RATE` | 15°/s | Pitch değişim hızı |
| `MAX_CLIMB_RATE` | 20 m/s | Maksimum tırmanma hızı |
| `WAYPOINT_REACH_THRESHOLD` | 200 m | Yol noktasına varma eşiği |

### 3.5 Giriş Katmanı (`src/input/`)

| Dosya | Sınıf | Sorumluluk |
|-------|-------|------------|
| `InputManager.h/cpp` | `InputManager` | Klavye olaylarını `InputState` struct'ına çevir, kontrol modunu değiştir |
| `KeyBindings.h` | `namespace` | Tuş atamaları: W/A/S/D hareket, Q/E tırmanma, Space mod değiştir |

**Tuş haritası:**
| Tuş | İşlev |
|-----|-------|
| `W` / `S` | Hızlan / Yavaşla |
| `A` / `D` | Sola / Sağa dön |
| `Q` / `E` | Tırman / Alçal |
| `Space` | MANUAL ↔ AUTOPILOT geçiş |
| `Tab` | Sonraki uçağı seç |
| `F` | Seçili uçağa odaklan |
| `P` | Simülasyonu duraklat |
| `Ctrl+N` | Yeni uçak oluştur |
| `Delete` | Seçili uçağı sil |

### 3.6 UI Katmanı (`src/ui/`)

| Dosya | Sınıf | Tür | Sorumluluk |
|-------|-------|-----|------------|
| `AircraftListPanel.h/cpp` | `AircraftListPanel` | QDockWidget | Uçak listesi ağacı + oluştur/sil butonları |
| `AircraftInspector.h/cpp` | `AircraftInspector` | QDockWidget | Seçili uçağın detaylı telemetrisi |
| `RouteEditorPanel.h/cpp` | `RouteEditorPanel` | QDockWidget | Rota seçimi, yol noktası listesi, düzenleme |
| `PlaybackControlPanel.h/cpp` | `PlaybackControlPanel` | QWidget | Play/Pause/Stop, seek slider, hız çarpanı, kayıt butonu |
| `TelemetryPanel.h/cpp` | `TelemetryPanel` | QWidget | QProgressBar tabanlı telemetri göstergeleri (irtifa, hız, heading, G) |
| `SimulationLogPanel.h/cpp` | `SimulationLogPanel` | QDockWidget | QPlainTextEdit günlük (bridge + JS konsol mesajları) |
| `CommandHistoryPanel.h/cpp` | `CommandHistoryPanel` | QDockWidget | Komut geçmişi (taslak) |
| `MapToolbar.h/cpp` | `MapToolbar` | QToolBar | Harita üstü araç çubuğu (kamera kontrolleri) |

**Panel yerleşimi:**
```
┌──── MainWindow (Sol Monitör) ────────────────────┐
│                                                    │
│  ┌── Sol Sütun ──┐  ┌───── Sağ Sütun ──────────┐  │
│  │ AircraftList   │  │ PlaybackControlPanel      │  │
│  │ (3x stretch)   │  │ TelemetryPanel (2x)       │  │
│  │ AircraftInsp.  │  │ Status Label              │  │
│  │ (2x stretch)   │  │ Tabs: SimLog / CmdHistory │  │
│  │ RouteEditor    │  │ (2x)                      │  │
│  │ (2x stretch)   │  │                           │  │
│  └────────────────┘  └───────────────────────────┘  │
│                                                    │
└────────────────────────────────────────────────────┘

┌──── MapWindow (Sağ Monitör / Yan) ────────────────┐
│                                                    │
│  ┌─────────── QWebEngineView ──────────────────┐  │
│  │                                              │  │
│  │           CesiumJS 3D Harita                 │  │
│  │     (uçaklar, rotalar, trail'ler)            │  │
│  │                                              │  │
│  │  ┌─── MapToolbar (yarı şeffaf) ──┐          │  │
│  │  │ Start │ Stop │ Speed │ Camera │          │  │
│  │  └───────────────────────────────┘          │  │
│  │                                              │  │
│  └──────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────┘
```

### 3.7 Protokol Katmanı (`src/protocols/`)

| Dosya | Sınıf | Durum |
|-------|-------|-------|
| `IProtocolAdapter.h` | `IProtocolAdapter` | Soyut arayüz (virtual) |
| `TacviewAdapter.h/cpp` | `TacviewAdapter` | XML over TCP ayrıştırıcı — **tanımlı ama kullanılmıyor** |

---

## 4. JavaScript Dosya Detayları

### 4.1 Giriş Noktaları

| Dosya | Mod | Açıklama |
|-------|-----|----------|
| `src/main-qt.js` | Qt gömülü | QWebChannel ile bağlantı, 5 modülü başlatma |
| `src/main.js` | Bağımsız tarayıcı | TCP istemcisi ile test, demo track'leri |

### 4.2 Modüller (`src/modules/`)

| Dosya | Sınıf | Sorumluluk |
|-------|-------|------------|
| `CesiumApp.js` | `CesiumApp` | Viewer başlatma: düz zemin, çevrimdışı karolar, performans ayarları |
| `CesiumBridge.js` | `CesiumBridge` | QWebChannel IPC: `connect()`, `sendEvent()`, mesaj yönlendirme |
| `EntityRenderer.js` | `EntityRenderer` | 3D uçak/rota/trail render: `applyFullState()`, `applyDelta()`, dead-reckoning interpolasyon |
| `CameraController.js` | `CameraController` | Kamera komutları: `flyTo()`, `trackEntity()`, `orbitEntity()` |
| `InteractionHandler.js` | `InteractionHandler` | Fare tıklama/sürükleme → `EVT_ENTITY_CLICKED`, `EVT_MAP_CLICKED`, `EVT_WAYPOINT_MOVED` |

### 4.3 Kullanılmayan Modüller (yalnızca standalone)

| Dosya | Sınıf | Durum |
|-------|-------|-------|
| `tcp/TcpClient.js` | `TcpClient` | WebSocket bağlantısı — Qt modunda kullanılmaz |
| `tcp/TcpConfig.js` | `TcpConfig` | localStorage ayarları — Qt modunda kullanılmaz |
| `tcp/XmlBuilder.js` | `XmlBuilder` | Track → XML — Qt modunda kullanılmaz |
| `ui/ControlPanel.js` | `ControlPanel` | HTML tabanlı panel — Qt native paneller kullanır |
| `ui/TrackVisualizer.js` | `TrackVisualizer` | Eski render — EntityRenderer tarafından aşıldı |

---

## 5. İş Parçacığı (Threading) Modeli

### 5.1 Mevcut Durum: TEK İŞ PARÇACIKLI

```
┌─── Ana Qt İş Parçacığı (Event Loop) ─────────────────────┐
│                                                            │
│  ├─ UI olayları (fare, klavye)                             │
│  ├─ QTimer: SimulationEngine::tick()  [her 50ms]          │
│  ├─ QTimer: ViteProcess::pollServer() [her 500ms]         │
│  ├─ Sinyal/slot bağlantıları (tümü aynı iş parçacığı)     │
│  ├─ QWebChannel mesaj gönder/al                           │
│  ├─ JSON serileştirme (StateSerializer)                    │
│  ├─ Fizik hesaplamaları (KinematicModel)                   │
│  ├─ UI panel güncellemeleri (refresh)                      │
│  └─ TrackRecorder::capture() (deep copy)                  │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌─── Ayrı İşletim Sistemi Süreci ──────────────────────────┐
│  ViteProcess → npm run dev (node.js)                      │
│  localhost:5173'te HTTP sunucusu                           │
│  Qt ile bağlantısı yok (sadece sayfa sunuyor)              │
└────────────────────────────────────────────────────────────┘

┌─── Chromium Renderer Süreci (QWebEngine tarafından yönetilir) ──┐
│  JavaScript çalıştırma                                           │
│  WebGL render (CesiumJS)                                        │
│  QWebChannel MOJO IPC                                           │
│  Bu süreç Qt tarafından otomatik oluşturulur                     │
└──────────────────────────────────────────────────────────────────┘
```

### 5.2 Kullanılan Eşzamanlılık Primitifleri

| Primitif | Kullanım |
|----------|----------|
| `QThread` | ❌ Kullanılmıyor |
| `QtConcurrent` | ❌ Kullanılmıyor |
| `std::thread` | ❌ Kullanılmıyor |
| `std::async` | ❌ Kullanılmıyor |
| `QMutex` / `QReadWriteLock` | ❌ Kullanılmıyor |
| `std::atomic` | ❌ Kullanılmıyor |
| `QThreadPool` / `QRunnable` | ❌ Kullanılmıyor |
| `QProcess` | ✅ ViteProcess (ayrı işlem, iş parçacığı değil) |
| `QTimer` | ✅ SimulationEngine (50ms), ViteProcess poll (500ms) |

### 5.3 Sonuç: Paralellik KULLANILMIYOR

**Uygulamada hiçbir C++ iş parçacığı oluşturulmamaktadır.** Tüm iş — fizik, JSON serileştirme, UI güncellemesi, köprü iletişimi — ana Qt olay döngüsü üzerinde sıralı (sequential) olarak çalışmaktadır.

Bu, iş parçacığı güvenliği sorunlarını ortadan kaldırır (race condition, deadlock yok) ancak performans açısından önemli kısıtlamalar getirir.

---

## 6. Performans Analizi & Yavaşlık Nedenleri

### 6.1 Tick Başına Hesaplama Maliyeti

Her 50ms'de (20 Hz) ana iş parçacığında sıralı olarak yapılan işlemler:

```
┌─ tick() çağrılır ──────────────────────────────────────────┐
│                                                             │
│  1. SimulationClock::advance()          ~1 µs               │
│                                                             │
│  2. Her uçak için (N adet):                                │
│     ├─ ManualController::update()       ~5 µs               │
│     ├─ AutopilotController::update()    ~10 µs (trig hesap) │
│     ├─ KinematicModel::update()         ~20 µs (büyük daire)│
│     └─ addTrailPoint()                  ~1 µs               │
│     Toplam per uçak:                    ~36 µs              │
│                                                             │
│  3. emit tickCompleted()                                    │
│     → CesiumBridge::pushDelta()                             │
│       ├─ StateSerializer::serializeDelta()                  │
│       │   ├─ N × toRenderDelta()        ~5 µs × N          │
│       │   └─ QJsonObject oluşturma      ~10 µs              │
│       ├─ QJsonDocument::toJson()        ~50–200 µs ⚠️       │
│       └─ emit sendToCesium(jsonStr)                         │
│           └─ QWebChannel IPC            ~100–500 µs ⚠️       │
│                                                             │
│  4. TrackRecorder::capture()                                │
│     └─ QMap<QString, AircraftState> deep copy  ~50 µs ⚠️    │
│                                                             │
│  5. UI panel refresh sinyal/slot'ları   ~100–500 µs ⚠️       │
│     ├─ AircraftInspector::refresh()                         │
│     ├─ TelemetryPanel::refreshTelemetry()                   │
│     ├─ AircraftListPanel durum güncellemesi                  │
│     └─ PlaybackControlPanel slider güncellemesi              │
│                                                             │
│  TOPLAM (10 uçak):  ~1–3 ms / tick                         │
│  Bütçe (50ms/tick):  Kabul edilebilir ✅                     │
│                                                             │
│  TOPLAM (50+ uçak): ~5–15 ms / tick                        │
│  Bütçe aşımı riski:  Dikkat gerekir ⚠️                      │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 Tespit Edilen Darboğazlar

#### 🔴 Kritik: QWebChannel JSON IPC Gecikmesi

```
C++ (serializeDelta)           → JSON string → QWebChannel IPC → JS (JSON.parse) → render
      ~200 µs                    ~100 µs          ~300-1000 µs       ~50 µs
                                                       ↑
                                            EN BÜYÜK DARBOĞAZ
```

QWebChannel, mesajları Chromium IPC (MOJO) üzerinden gönderir. Bu:
- Proses sınırı geçişi (C++ → Chromium renderer process)
- İki defa serileştirme (Qt JSON → string → Chromium → JS JSON.parse)
- Sıralı gönderim (asenkron ama kuyruklanır)

**Etki:** 10 uçakla ~1ms, ama 50 uçakla JSON boyutu büyür ve ~3-5ms'a çıkar.

#### 🟠 Önemli: Trail Data Her Tick Kopyalanıyor

```cpp
// AircraftState::addTrailPoint() — her tick, her uçak
trail.append({lat, lon, alt});
if (trail.size() > MAX_TRAIL)  // 500
    trail.removeFirst();       // ← O(N) shift! QVector baştan silme pahalı
```

`QVector::removeFirst()` tüm elemanları bir ileri kaydırır — 500 × 24 byte = 12 KB bellek hareketi, **her uçak, her tick**.

#### 🟠 Önemli: TrackRecorder Deep Copy

```cpp
void TrackRecorder::capture(quint64 tick, const QMap<QString, AircraftState> &aircraft) {
    TickSnapshot snap;
    snap.aircraftStates = aircraft;  // ← QMap DEEP COPY (trail dahil!)
    m_snapshots.append(snap);
}
```

Her tick'te tüm AircraftState'ler **trail vektörleriyle birlikte** deep copy ediliyor. 10 uçak × 500 trail point = 120 KB/tick kopyalama.

#### 🟡 Dikkat: UI Panel Güncellemeleri Ana Thread'de

Tüm panel refresh'leri (AircraftInspector, TelemetryPanel, AircraftListPanel) her tick sinyal/slot zinciri üzerinden çalışır. `isVisible()` kontrolü yapılsa bile, sinyal emit'leri ve slot çağrıları yığılır.

#### 🟡 Dikkat: EntityRenderer Trail Polyline Yeniden Oluşturma

```javascript
// EntityRenderer.js — _updateAircraft()
entry.trailEntity.polyline.positions = entry.trailPositions.slice();
// ← Her güncellemede 500 noktalık dizi kopyalanır ve
//   CesiumJS polyline geometrisi yeniden derlenir
```

Her full sync'te trail polyline'ı tamamen yeniden oluşturulur. CesiumJS bu geometriyi GPU'da yeniden derler — pahalı.

#### 🟢 İyi: Dead-Reckoning Sadece Seçili Uçak İçin

```javascript
// EntityRenderer.js — _interpolatePositions()
if (this.selectedEntityId && id !== this.selectedEntityId) continue;
// Yalnızca seçili uçak için 60fps interpolasyon, diğerleri 20Hz
```

Bu, interpolasyon maliyetini N'den 1'e düşürür. Doğru bir optimizasyon.

### 6.3 Gecikme Yolu Analizi (End-to-End Latency)

```
Kullanıcı 'W' tuşuna basar
  ↓ ~0 ms
MainWindow::keyPressEvent → InputManager
  ↓ ... bekle tick timer'ını ...
SimulationEngine::tick()              ← en kötü durum: +50ms bekleme
  ├─ Fizik hesaplama                  ← ~1 ms
  └─ emit tickCompleted()
  ↓
CesiumBridge::pushDelta()
  ├─ JSON serialize                   ← ~0.3 ms
  └─ QWebChannel IPC                 ← ~0.5–1 ms
  ↓
JavaScript: JSON.parse + applyDelta   ← ~0.1 ms
  ↓
EntityRenderer: konum güncelle        ← ~0.1 ms
  ↓
requestRender() → CesiumJS render     ← 1 frame = ~16 ms (60fps)
  ↓
Ekranda görünür

TOPLAM GECİKME: 50 + 1 + 1 + 16 ≈ 68 ms (en kötü durum)
                 0 + 1 + 1 + 16 ≈ 18 ms (en iyi durum)
```

Bu gecikme simülasyon için kabul edilebilir ama "anlık tepki" hissi vermez. Dead-reckoning interpolasyon görsel olarak yumuşatır ama gerçek gecikmeyi azaltmaz.

---

## 7. Performans İyileştirme Önerileri

### 7.1 Hemen Yapılabilecekler (Kolay)

#### A. Trail `QVector::removeFirst()` → Circular Buffer
```
Mevcut:   QVector + removeFirst()   → O(N) her tick
Önerilen: Sabit boyutlu ring buffer → O(1) her tick
```
500 × 10 uçak = 5000 gereksiz bellek kaydırma/tick ortadan kalkar.

#### B. TrackRecorder: Trail'siz Kopyalama
```
Mevcut:   QMap deep copy (trail DAHİL)   → 120 KB/tick
Önerilen: Trail'i kopyadan hariç tut      → ~5 KB/tick
```
Kayıtta trail verisi gereksiz — pozisyon zaten snapshot'ta var.

#### C. UI Panel Throttle (200ms)
```
Mevcut:   Her tick (50ms) panel refresh → 20 Hz UI güncellemesi
Önerilen: QTimer ile 200ms throttle     → 5 Hz UI (insan gözü için yeterli)
```

### 7.2 Orta Vadede Yapılabilecekler (Orta)

#### D. Dirty Flag ile Delta Optimizasyonu
```
Mevcut:   Her tick TÜM uçakları serialize et
Önerilen: Yalnızca değişen uçakları serialize et (dirty flag)
```
IDLE modundaki uçaklar gereksiz yere delta'ya dahil ediliyor.

#### E. JSON → Binary Protocol (QDataStream veya MessagePack)
```
Mevcut:   JSON serialize + parse → ~2× overhead (string encoding)
Önerilen: Binary encoding         → ~0.3× overhead
```
Ancak QWebChannel JSON ile çalışacak şekilde tasarlandığından, bu değişiklik karmaşıktır.

#### F. Simülasyon Motoru için Ayrı İş Parçacığı
```
Mevcut:   Tüm fizik + UI + serialize → ana thread
Önerilen: SimulationEngine → QThread'de çalıştır
           Mutex ile AppState koruması
           Sonuçları ana thread'e sinyal ile bildir
```
Bu en etkili iyileştirme olabilir — fizik hesaplamaları UI'yı bloklamaz.

### 7.3 Uzun Vadede Yapılabilecekler (Zor)

#### G. QWebChannel IPC Bypass — SharedMemory
```
Mevcut:   JSON string → QWebChannel → Chromium IPC → JS
Önerilen: SharedArrayBuffer + Atomics (WebAssembly düzeyinde)
           Veya OffscreenCanvas + transferable objects
```
Bu radikal bir mimari değişikliktir. Çok yüksek uçak sayıları (100+) için gerekli olabilir.

#### H. CesiumJS Entity Yerine Primitive API
```
Mevcut:   viewer.entities.add() → Entity API (yavaş, her frame property check)
Önerilen: PrimitiveCollection + ModelInstanceCollection → batch render
```
10+ uçakta belirgin fark yaratır. CesiumJS Entity API her frame'de property değişikliği kontrol eder.

---

## 8. Önerilen Öncelikli Eylem Planı

| Öncelik | İyileştirme | Beklenen Etki | Zorluk |
|---------|-------------|---------------|--------|
| 1 | Trail circular buffer (A) | %15-20 CPU azalma (tick başına) | Kolay |
| 2 | TrackRecorder trail'siz copy (B) | %30-40 bellek azalma (kayıt sırasında) | Kolay |
| 3 | UI panel throttle 200ms (C) | %10-15 ana thread yük azalması | Kolay |
| 4 | Dirty flag delta (D) | %20-50 IPC trafik azalması (çok uçakta) | Orta |
| 5 | SimEngine ayrı thread (F) | %40-60 UI cevap süresi iyileşmesi | Orta |
| 6 | Entity → Primitive API (H) | %30-50 CesiumJS render iyileşmesi | Zor |

---

## 9. JSON Protokol Özeti

### Qt → JavaScript Mesajları

| Mesaj Tipi | Ne Zaman | İçerik | Boyut (10 uçak) |
|-----------|---------|--------|-----------------|
| `STATE_FULL_SYNC` | Başlangıç + talep üzerine | Tüm uçaklar + rotalar + seçim + trail | ~15-30 KB |
| `STATE_DELTA` | Her tick (50ms) | Değişen konum/tutum alanları | ~1-3 KB |
| `CMD_CAMERA_*` | Kullanıcı etkileşimi | Kamera komutu | ~0.2 KB |
| `CMD_CREATE_ENTITY` | Uçak oluşturma | Yeni uçak detayları | ~0.5 KB |
| `CMD_REMOVE_ENTITY` | Uçak silme | Entity ID | ~0.1 KB |

### JavaScript → Qt Mesajları

| Mesaj Tipi | Tetikleyici | İçerik |
|-----------|------------|--------|
| `EVT_ENTITY_CLICKED` | Sol tık | entityId, entityType |
| `EVT_ENTITY_CTRL_CLICKED` | Ctrl+tık | entityId, entityType |
| `EVT_MAP_CLICKED` | Boş alana tık | lat, lon, alt |
| `EVT_WAYPOINT_MOVED` | Waypoint sürükleme | routeId, waypointIndex, newPosition |
| `REQ_FULL_SYNC` | Yeniden bağlanma | — |

---

## 10. Build Yapılandırması

### CMakeLists.txt
```cmake
# Qt modülleri
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebChannel Network)

# Derleyici: MSVC 14.39.33519 (VS 2022)
# C++ standardı: C++17
# Çıktı: WIN32_EXECUTABLE (konsol penceresi yok)
# WEB_CONTENT_PATH: cesium_web klasörünün mutlak yolu (C++ define olarak geçirilir)
```

### CMakePresets.json
| Preset | Tür | Açıklama |
|--------|-----|----------|
| `dev-debug` | Configure | Ninja, Debug, Visual Studio 17 2022 |
| `build-debug` | Build | Debug derleme |
| `build-release` | Build | Release derleme |

### Chromium GPU Bayrakları (`main.cpp`)
| Bayrak | Amaç |
|--------|------|
| `--use-angle=d3d11` | D3D11 backend (SwiftShader yerine) |
| `--ignore-gpu-blocklist` | Engelli GPU'ları da kullan |
| `--enable-gpu-rasterization` | GPU hızlandırmalı 2D |
| `--enable-zero-copy` | CesiumJS karoları için sıfır kopyalama |
| `--disable-software-rasterizer` | CPU render'ı devre dışı |
| `--disable-background-timer-throttling` | Arka planda timer kısıtlama |
| `--disable-renderer-backgrounding` | Odak dışında kısıtlama |
| `--remote-debugging-port=9222` | DevTools ile JS hata ayıklama |

### Vite Yapılandırması (`vite.config.js`)
- **Port:** 5173
- **Build hedefleri:** `index-qt.html` (Qt), `index.html` (standalone)
- **CesiumJS:** Assets, Workers, Widgets, ThirdParty statik kopyalama
- **COOP/COEP başlıkları yok** (QWebChannel uyumluluğu için)

---

## 11. Sinyal/Slot Haritası (Kritik Bağlantılar)

```
SimulationEngine::tickCompleted(tick)
  └─→ MainWindow lambda:
       ├─ CesiumBridge::pushDelta()
       ├─ TrackRecorder::capture()
       ├─ AircraftInspector::refresh()
       └─ TelemetryPanel::refreshTelemetry()

CesiumBridge::cesiumReady()
  └─→ MainWindow::onCesiumReady()
       └─ createDemonstrationScenario()

CesiumBridge::entityClicked(id, type)
  └─→ SelectionManager::selectEntity()
       └─→ selectionChanged()
            └─ UI panelleri güncelle

AircraftManager::aircraftCreated(id)
  └─→ AircraftListPanel::refresh()
  └─→ RouteEditorPanel::refresh()

InputManager::createAircraftRequested()
  └─→ MainWindow lambda → AircraftManager::createAircraft()
```

---

## 12. Bellek Kullanım Tahmini

| Bileşen | Tahmin | Not |
|---------|--------|-----|
| QWebEngineView + Chromium | 200–400 MB | Tarayıcı temel yükü |
| CesiumJS viewer + karolar | 50–150 MB | Zoom seviyesine bağlı |
| CesiumJS entity'ler (10 uçak) | 10–20 MB | Model + trail + label |
| Qt C++ durum (10 uçak) | <5 MB | QMap + AircraftState |
| TrackRecorder (tam kapasite) | ~73 MB | 36000 × (10 uçak × ~200B) |
| **TOPLAM** | **~400–650 MB** | |

---

> **Sonuç:** Uygulama temiz bir tek-iş parçacıklı mimariye sahip. 10-20 uçak için performans yeterli ancak ölçekleme sınırlı. Bölüm 7'deki iyileştirmeler (özellikle A, B, C) hemen uygulanabilir ve gözle görülür iyileşme sağlar. Daha büyük ölçekler (50+ uçak) için SimulationEngine'in ayrı bir iş parçacığına taşınması (F) en etkili çözüm olacaktır.
