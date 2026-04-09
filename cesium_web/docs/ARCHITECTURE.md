# Mimari Dokümantasyon

## 🏛️ Sistem Mimarisi

Bu belge, CesiumJS Workshop TCP Track Injection sisteminin detaylı mimari tasarımını açıklar.

---

## 📊 Genel Mimari Diyagramı

```
┌─────────────────────────────────────────────────────────────┐
│                    Browser Application                       │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │              Main Application (main.js)             │    │
│  │                                                      │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌───────────┐│    │
│  │  │ CesiumViewer │  │  TcpClient   │  │  Control  ││    │
│  │  │   (3D Map)   │  │ (Connection) │  │   Panel   ││    │
│  │  └──────────────┘  └──────────────┘  └───────────┘│    │
│  │         │                  │                │       │    │
│  │         └──────────────────┴────────────────┘       │    │
│  │                          │                          │    │
│  │                 ┌────────▼────────┐                 │    │
│  │                 │ TrackVisualizer │                 │    │
│  │                 │  (Rendering)    │                 │    │
│  │                 └─────────────────┘                 │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            │
                  WebSocket (ws://)
                            │
                            ▼
        ┌────────────────────────────────────┐
        │    TCP Server (Tacview/Custom)     │
        │         Port: 9001 (default)       │
        └────────────────────────────────────┘
```

---

## 🔄 Veri Akış Diyagramı

### 1. Bağlantı Kurulum Akışı

```
User Action          TcpClient           Server               UI
    │                    │                  │                  │
    │  Click "Connect"   │                  │                  │
    ├────────────────────>                  │                  │
    │                    │  WS Handshake    │                  │
    │                    ├──────────────────>                  │
    │                    │                  │                  │
    │                    │  Connection OK   │                  │
    │                    <──────────────────┤                  │
    │                    │                  │                  │
    │                    │  Emit "statusChanged"               │
    │                    ├────────────────────────────────────>│
    │                    │                  │                  │
    │                    │                  │  Update UI       │
    │                    │                  │  (Green status)  │
    │                    │                  │<─────────────────┤
```

### 2. Track Gönderme Akışı

```
User Action          ControlPanel     TcpClient      XmlBuilder    Server
    │                    │               │               │            │
    │  Click "Send"      │               │               │            │
    ├────────────────────>               │               │            │
    │                    │  Get Tracks   │               │            │
    │                    │               │  Build XML    │            │
    │                    ├───────────────────────────────>            │
    │                    │               │               │            │
    │                    │               │  XML String   │            │
    │                    <───────────────────────────────┤            │
    │                    │               │               │            │
    │                    │  Send XML     │               │            │
    │                    ├──────────────>                │            │
    │                    │               │  WS Send      │            │
    │                    │               ├────────────────────────────
    │                    │               │               │            │
    │                    │               │  ACK/Response │            │
    │                    │               <────────────────────────────┤
    │                    │               │               │            │
    │                    │  Success      │               │            │
    │                    <───────────────┤               │            │
```

### 3. Track Görselleştirme Akışı

```
Track Data       TrackVisualizer     Cesium API      Viewer (3D)
    │                   │                 │               │
    │  visualizeTracks  │                 │               │
    ├───────────────────>                 │               │
    │                   │                 │               │
    │                   │  Parse waypoints│               │
    │                   ├─────────────────┘               │
    │                   │                 │               │
    │                   │  entities.add() │               │
    │                   ├─────────────────>               │
    │                   │                 │               │
    │                   │                 │  Render       │
    │                   │                 ├──────────────>│
    │                   │                 │               │
    │                   │                 │  Display      │
    │                   │                 │  Tracks       │
    │                   │                 │<──────────────┤
```

---

## 🧩 Modül Detayları

### 1. TcpClient Modülü

**Sorumluluklar:**
- WebSocket bağlantı yönetimi
- Otomatik yeniden bağlanma mekanizması
- Event tabanlı bildirim sistemi
- Mesaj gönderme ve alma

**Önemli Metodlar:**
```javascript
connect(host, port)      // Bağlantı kur
disconnect()             // Bağlantıyı kes
sendTracks(tracks)       // Track verisi gönder
reset()                  // Telemetri sıfırla
on(event, callback)      // Event listener ekle
emit(event, data)        // Event tetikle
```

**State Yönetimi:**
```javascript
{
  socket: WebSocket | null,
  isConnected: boolean,
  reconnectAttempts: number,
  listeners: {
    statusChanged: Function[],
    messageReceived: Function[],
    connectionFailed: Function[],
    disconnected: Function[]
  }
}
```

---

### 2. TcpConfig Modülü

**Sorumluluklar:**
- Bağlantı ayarlarını LocalStorage'da sakla
- Host ve port yönetimi
- Varsayılan değerler sağlama

**LocalStorage Şeması:**
```javascript
{
  key: "cesium_tcp_config",
  value: {
    host: string,    // "127.0.0.1"
    port: number     // 9001
  }
}
```

**Singleton Pattern:**
```javascript
// TcpConfig tek bir instance olarak export edilir
export default new TcpConfig();
```

---

### 3. XmlBuilder Modülü

**Sorumluluklar:**
- JavaScript track objelerini XML'e dönüştürme
- XML özel karakterleri escape etme
- Tacview uyumlu format sağlama

**XML Şablon Yapısı:**
```xml
<Tracks>
  <Track>
    <Number/>
    <CallSign/>
    <Type/>
    <Color/>
    <EngagementRange/>
    <VerticalEngagementRange/>
    <IsSelected/>
    <Waypoints>
      <Waypoint>
        <Latitude/>
        <Longitude/>
        <Altitude/>
        <Time/>
      </Waypoint>
    </Waypoints>
  </Track>
</Tracks>
```

**Static Methods:**
```javascript
buildTracksXml(tracks[])       // Ana XML oluştur
buildTrackElement(track)       // Tek track element
buildWaypointElement(wp)       // Waypoint element
buildElement(name, value)      // Generic element
escapeXml(string)              // Escape özel karakterler
```

---

### 4. ControlPanel Modülü

**Sorumluluklar:**
- Kullanıcı arayüzü oluşturma ve yönetimi
- TCP bağlantı kontrolü
- Track listesi gösterimi
- Log mesajları gösterimi

**UI Bileşenleri:**
```
ControlPanel
├── Header (Başlık + Toggle Button)
├── Tab Navigation
│   ├── Connection Tab
│   ├── Tracks Tab
│   └── Settings Tab
└── Content Area
    ├── Connection Content
    │   ├── Status Display
    │   ├── Host/Port Inputs
    │   ├── Connect/Disconnect Buttons
    │   └── Log Console
    ├── Tracks Content
    │   ├── Track List
    │   ├── Send Tracks Button
    │   └── Reset Button
    └── Settings Content
        └── Configuration Options
```

**Event Handlers:**
```javascript
onTogglePanel()           // Panel aç/kapat
onTabSwitch(tabName)      // Sekme değiştir
onConnect()               // Bağlan
onDisconnect()            // Bağlantıyı kes
onSendTracks()            // Track gönder
onReset()                 // Reset komutu
```

---

### 5. TrackVisualizer Modülü

**Sorumluluklar:**
- Track'leri Cesium haritasında görselleştirme
- Polyline, marker ve label oluşturma
- Renk ve stil yönetimi
- Entity yaşam döngüsü yönetimi

**Görselleştirme Bileşenleri:**

Her track için:
1. **Polyline**: Waypoint'ler arasında çizgi
2. **Point**: Başlangıç noktası marker
3. **Label**: Track callsign etiketi

**Cesium Entity Yapısı:**
```javascript
viewer.entities.add({
  name: "Track: ALPHA-01",
  polyline: {
    positions: Cartesian3[],
    width: 3,
    material: Color.RED
  },
  point: {
    pixelSize: 8,
    color: Color.RED
  },
  label: {
    text: "ALPHA-01",
    font: "12px Arial"
  }
})
```

**Renk Haritası:**
```javascript
{
  Fighter: Color.RED,
  Transport: Color.BLUE,
  Helicopter: Color.GREEN,
  Unknown: Color.YELLOW
}
```

---

## 🔐 Güvenlik Considerations

### 1. WebSocket Güvenliği

- **Şifrelenmemiş Bağlantı**: Mevcut implementasyon `ws://` kullanır
- **Önerilen**: Prodüksiyon için `wss://` (WebSocket Secure) kullanın
- **CORS**: Same-origin policy dikkate alınmalı

### 2. Input Validation

- **XML Injection**: XmlBuilder.escapeXml() ile korunuyor
- **Host/Port Girişi**: Kullanıcı girdileri validate edilmeli
- **Track Data**: Server'dan gelen veri sanitize edilmeli

### 3. LocalStorage

- **Hassas Veri**: Token veya şifre LocalStorage'da saklanmamalı
- **Veri Temizleme**: Kullanıcı logout olduğunda config silinmeli

---

## 📈 Performans Optimizasyonları

### 1. Entity Yönetimi

```javascript
// Eski entity'leri cache'le ve yeniden kullan
if (this.trackEntities.has(trackId)) {
  const oldEntity = this.trackEntities.get(trackId);
  this.viewer.entities.remove(oldEntity);
}
```

### 2. Batch İşlemler

- Çok sayıda track tek seferde gönderildiğinde batching yapılmalı
- Entity oluşturma işlemleri requestAnimationFrame içinde yapılabilir

### 3. Memory Leaks Önleme

- Kullanılmayan entity'ler temizlenmeli
- Event listener'lar destroy'da kaldırılmalı
- WebSocket bağlantıları kapatılmalı

---

## 🔄 Extension Points (Genişletme Noktaları)

### 1. Yeni Track Tipleri Ekleme

```javascript
// TrackVisualizer.js içinde
this.trackColors = {
  Fighter: Color.RED,
  Transport: Color.BLUE,
  Helicopter: Color.GREEN,
  Drone: Color.ORANGE,      // EKLE
  Ship: Color.CYAN          // EKLE
};
```

### 2. Farklı Protokol Desteği

```javascript
// TcpClient.js içinde alternatif protokol ekle
async sendTracksJson(tracks) {
  const json = JSON.stringify(tracks);
  this.socket.send(json);
}
```

### 3. Custom Rendering

```javascript
// TrackVisualizer'ı extend et
class AdvancedTrackVisualizer extends TrackVisualizer {
  visualizeTrackWith3DModel(track, modelUrl) {
    // 3D model rendering logic
  }
}
```

---

## 🧪 Testing Stratejisi

### Unit Tests (Önerilen)

```javascript
// TcpClient.test.js
describe('TcpClient', () => {
  it('should connect to server', async () => {
    const client = new TcpClient();
    await client.connect('localhost', 9001);
    expect(client.isConnected).toBe(true);
  });
});

// XmlBuilder.test.js
describe('XmlBuilder', () => {
  it('should escape XML special chars', () => {
    const result = XmlBuilder.escapeXml('<tag>');
    expect(result).toBe('&lt;tag&gt;');
  });
});
```

### Integration Tests

- WebSocket mock server ile tam akış testi
- Cesium viewer mock ile görselleştirme testi

---

## 📖 Referans Implementasyonlar

### C# TrackInjector ile Karşılaştırma

| Özellik | C# TrackInjector | JS CesiumJS Implementation |
|---------|------------------|----------------------------|
| Bağlantı | TCP Socket | WebSocket |
| UI Framework | WPF (XAML) | Vanilla JS (DOM) |
| 3D Görselleştirme | - | CesiumJS |
| Veri Format | XML | XML (aynı şema) |
| Config Storage | App Settings | LocalStorage |
| Threading | Multi-threaded | Single-threaded (async) |

---

## 🔮 Gelecek Geliştirmeler

### Phase 2 - Özellik Geliştirmeleri

- [ ] Track editing (waypoint düzenleme)
- [ ] Track playback (animasyon)
- [ ] Multi-track selection
- [ ] Export/import track files
- [ ] Real-time telemetry display

### Phase 3 - Enterprise Features

- [ ] Authentication & authorization
- [ ] Multi-user collaboration
- [ ] Cloud storage integration
- [ ] Advanced analytics dashboard
- [ ] Mobile responsive design

---

**Son Güncelleme**: Mart 2026  
**Mimari Versiyon**: 1.0.0
