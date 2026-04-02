# API Referansı

Bu belge, projedeki tüm modüllerin ve fonksiyonların detaylı API dokümantasyonunu içerir.

---

## 📡 TcpClient API

### Constructor

```javascript
const client = new TcpClient()
```

WebSocket tabanlı TCP istemcisi oluşturur.

**Returns**: `TcpClient` instance

---

### Methods

#### `connect(host, port)`

TCP sunucusuna bağlantı kurar.

**Parameters:**
- `host` (string, optional): Sunucu adresi. Belirtilmezse TcpConfig'den alınır.
- `port` (number, optional): Port numarası. Belirtilmezse TcpConfig'den alınır.

**Returns**: `Promise<void>`

**Example:**
```javascript
await tcpClient.connect('192.168.1.100', 9001);
```

**Events Emitted:**
- `statusChanged`: Bağlantı durumu değiştiğinde
- `connectionFailed`: Bağlantı başarısız olduğunda

---

#### `disconnect()`

Mevcut bağlantıyı kapatır.

**Returns**: `void`

**Example:**
```javascript
tcpClient.disconnect();
```

**Events Emitted:**
- `disconnected`: Bağlantı kapatıldığında
- `statusChanged`: Durum değişikliğinde

---

#### `sendTracks(tracks)`

Track listesini XML formatında sunucuya gönderir.

**Parameters:**
- `tracks` (Array<Track>): Gönderilecek track listesi

**Returns**: `Promise<void>`

**Throws**: Bağlantı yoksa hata fırlatır

**Example:**
```javascript
const tracks = [
  {
    number: 1,
    callSign: "ALPHA-01",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 41.0, longitude: 28.9, altitude: 5000, time: 0 }
    ]
  }
];

await tcpClient.sendTracks(tracks);
```

---

#### `reset()`

Telemetri sıfırlama komutu gönderir.

**Returns**: `Promise<void>`

**Example:**
```javascript
await tcpClient.reset();
```

---

#### `on(event, callback)`

Event listener ekler.

**Parameters:**
- `event` (string): Event adı (`statusChanged`, `messageReceived`, `connectionFailed`, `disconnected`)
- `callback` (Function): Çağrılacak fonksiyon

**Returns**: `void`

**Example:**
```javascript
tcpClient.on('statusChanged', (data) => {
  console.log('Status:', data.status);
  console.log('Message:', data.message);
});

tcpClient.on('messageReceived', (message) => {
  console.log('Server message:', message);
});
```

---

#### `emit(event, data)`

Event tetikler (internal kullanım).

**Parameters:**
- `event` (string): Event adı
- `data` (any): Event verisi

**Returns**: `void`

---

### Properties

```javascript
{
  socket: WebSocket | null,           // WebSocket bağlantısı
  isConnected: boolean,                // Bağlantı durumu
  config: TcpConfig,                   // Konfigürasyon referansı
  reconnectAttempts: number,           // Yeniden bağlanma denemesi
  maxReconnectAttempts: number,        // Max deneme sayısı (5)
  reconnectDelay: number,              // Bekleme süresi (3000ms)
  listeners: Object                    // Event listener map
}
```

---

## ⚙️ TcpConfig API

### Static Properties

```javascript
TcpConfig.DEFAULT_HOST = "127.0.0.1"
TcpConfig.DEFAULT_PORT = 9001
TcpConfig.STORAGE_KEY = "cesium_tcp_config"
```

---

### Methods

#### `load()`

LocalStorage'dan konfigürasyon yükler.

**Returns**: `{host: string, port: number}`

**Example:**
```javascript
const config = TcpConfig.load();
console.log(config.host, config.port);
```

---

#### `save(config)`

Konfigürasyonu LocalStorage'a kaydeder.

**Parameters:**
- `config` (Object): `{host: string, port: number}`

**Returns**: `void`

**Example:**
```javascript
TcpConfig.save({ host: '192.168.1.100', port: 9001 });
```

---

#### `getHost()`

Kayıtlı host adresini döndürür.

**Returns**: `string`

---

#### `getPort()`

Kayıtlı port numarasını döndürür.

**Returns**: `number`

---

#### `setHostPort(host, port)`

Host ve port'u ayarlar ve kaydeder.

**Parameters:**
- `host` (string): Sunucu adresi
- `port` (number): Port numarası

**Returns**: `void`

**Example:**
```javascript
TcpConfig.setHostPort('192.168.1.100', 9001);
```

---

#### `getEndpointUrl()`

WebSocket URL'ini oluşturur.

**Returns**: `string`

**Example:**
```javascript
const url = TcpConfig.getEndpointUrl();
// "ws://127.0.0.1:9001"
```

---

## 🔨 XmlBuilder API

Tüm metodlar static'tir.

### `buildTracksXml(tracks)`

Track listesinden XML oluşturur.

**Parameters:**
- `tracks` (Array<Track>): Track listesi

**Returns**: `string` - XML string

**Example:**
```javascript
const xml = XmlBuilder.buildTracksXml([track1, track2]);
console.log(xml);
```

---

### `buildTrackElement(track)`

Tek bir track için XML element oluşturur.

**Parameters:**
- `track` (Track): Track objesi

**Returns**: `string` - XML fragment

---

### `buildWaypointElement(waypoint)`

Waypoint için XML element oluşturur.

**Parameters:**
- `waypoint` (Waypoint): Waypoint objesi

**Returns**: `string` - XML fragment

---

### `buildElement(name, value, indent)`

Generic XML element oluşturur.

**Parameters:**
- `name` (string): Element adı
- `value` (any): Element değeri
- `indent` (number, optional): Girinti boşluğu (default: 4)

**Returns**: `string` - XML element

**Example:**
```javascript
const xml = XmlBuilder.buildElement('CallSign', 'ALPHA-01', 4);
// "    <CallSign>ALPHA-01</CallSign>\n"
```

---

### `escapeXml(str)`

XML özel karakterlerini escape eder.

**Parameters:**
- `str` (any): Escape edilecek değer

**Returns**: `string`

**Escape Map:**
```
& -> &amp;
< -> &lt;
> -> &gt;
" -> &quot;
' -> &apos;
```

**Example:**
```javascript
const escaped = XmlBuilder.escapeXml('<tag>');
// "&lt;tag&gt;"
```

---

## 🎨 ControlPanel API

### Constructor

```javascript
const panel = new ControlPanel(tcpClient)
```

**Parameters:**
- `tcpClient` (TcpClient): TCP istemci referansı

**Returns**: `ControlPanel` instance

---

### Methods

#### `togglePanel()`

Panel'i açar veya kapatır.

**Returns**: `void`

**Example:**
```javascript
panel.togglePanel();
```

---

#### `switchTab(tabName)`

Sekme değiştirir.

**Parameters:**
- `tabName` (string): `'connection'`, `'tracks'`, veya `'settings'`

**Returns**: `void`

**Example:**
```javascript
panel.switchTab('tracks');
```

---

#### `updateTrackList(tracks)`

Track listesini günceller ve UI'da gösterir.

**Parameters:**
- `tracks` (Array<Track>): Track listesi

**Returns**: `void`

**Example:**
```javascript
panel.updateTrackList([track1, track2, track3]);
```

---

#### `addLogMessage(message, type)`

Log konsolu ekleme yapar.

**Parameters:**
- `message` (string): Log mesajı
- `type` (string): `'info'`, `'success'`, `'warning'`, veya `'error'`

**Returns**: `void`

**Example:**
```javascript
panel.addLogMessage('Connected successfully', 'success');
panel.addLogMessage('Connection failed', 'error');
```

---

#### `updateConnectionStatus(status, message)`

Bağlantı durumunu günceller.

**Parameters:**
- `status` (string): Durum kodu
- `message` (string): Durum mesajı

**Returns**: `void`

---

#### `destroy()`

Panel'i DOM'dan kaldırır ve temizler.

**Returns**: `void`

---

### Static Methods

#### `ControlPanel.injectStyles()`

CSS stillerini document'e ekler.

**Returns**: `void`

**Example:**
```javascript
ControlPanel.injectStyles();
```

---

### Properties

```javascript
{
  tcpClient: TcpClient,         // TCP istemci referansı
  config: TcpConfig,            // Config referansı
  isOpen: boolean,              // Panel açık mı?
  tracks: Array<Track>,         // Mevcut track listesi
  currentTab: string,           // Aktif sekme
  panel: HTMLElement,           // Panel DOM element
  logContainer: HTMLElement     // Log konsol element
}
```

---

## 🗺️ TrackVisualizer API

### Constructor

```javascript
const visualizer = new TrackVisualizer(viewer)
```

**Parameters:**
- `viewer` (Cesium.Viewer): Cesium viewer instance

**Returns**: `TrackVisualizer` instance

---

### Methods

#### `visualizeTrack(track)`

Tek bir track'i haritada görselleştirir.

**Parameters:**
- `track` (Track): Track objesi

**Returns**: `void`

**Example:**
```javascript
visualizer.visualizeTrack({
  number: 1,
  callSign: "ALPHA-01",
  type: "Fighter",
  waypoints: [...]
});
```

---

#### `visualizeAllTracks(tracks)`

Tüm track'leri görselleştirir.

**Parameters:**
- `tracks` (Array<Track>): Track listesi

**Returns**: `void`

**Example:**
```javascript
visualizer.visualizeAllTracks([track1, track2, track3]);
```

---

#### `clearTrack(trackId)`

Belirli bir track'i haritadan siler.

**Parameters:**
- `trackId` (string): Track ID'si

**Returns**: `void`

---

#### `clearAllTracks()`

Tüm track'leri haritadan siler.

**Returns**: `void`

**Example:**
```javascript
visualizer.clearAllTracks();
```

---

#### `getColorForType(type)`

Track tipine göre renk döndürür.

**Parameters:**
- `type` (string): Track tipi (`'Fighter'`, `'Transport'`, vb.)

**Returns**: `Cesium.Color`

**Example:**
```javascript
const color = visualizer.getColorForType('Fighter');
// Returns Cesium.Color.RED
```

---

#### `focusOnTrack(trackId)`

Kamerayı belirli bir track'e odaklar.

**Parameters:**
- `trackId` (string): Track ID'si

**Returns**: `void`

---

### Properties

```javascript
{
  viewer: Cesium.Viewer,                // Cesium viewer referansı
  trackEntities: Map<string, Entity>,   // Track ID -> Entity map
  trackColors: Object                   // Type -> Color map
}
```

---

## 📦 Data Types

### Track Type

```typescript
interface Track {
  number: number;                  // Track numarası
  callSign: string;                // Çağrı işareti
  type: string;                    // Tip (Fighter, Transport, vb.)
  color: string;                   // Hex color (FF0000)
  engagementRange?: number;        // Muharebe menzili
  verticalEngagementRange?: number; // Dikey muharebe menzili
  isSelected?: boolean;            // Seçili mi?
  waypoints: Waypoint[];           // Rota noktaları
}
```

### Waypoint Type

```typescript
interface Waypoint {
  latitude: number;     // Enlem (degrees)
  longitude: number;    // Boylam (degrees)
  altitude: number;     // Yükseklik (meters)
  time: number;         // Zaman (seconds)
}
```

### Config Type

```typescript
interface Config {
  host: string;         // Sunucu adresi
  port: number;         // Port numarası
}
```

### Status Event Data

```typescript
interface StatusChangeData {
  status: 'CONNECTING' | 'CONNECTED' | 'DISCONNECTED' | 'ERROR';
  message: string;
  timestamp?: Date;
}
```

---

## 🔌 Global API (Window)

### Debug/Test Fonksiyonları

```javascript
// Panel toggle
window.toggleTcpPanel()

// Track gönderme
await window.sendSampleTracks()

// Telemetri reset
await window.resetTelemetry()
```

### Global Referanslar

```javascript
window.tcpClient       // TcpClient instance
window.trackVisualizer // TrackVisualizer instance
window.tcpControlPanel // ControlPanel instance
window.sampleTracks    // Örnek track listesi
```

---

## 🎯 Usage Examples

### Tam Akış Örneği

```javascript
// 1. Modülleri import et
import TcpClient from './modules/tcp/TcpClient.js';
import TcpConfig from './modules/tcp/TcpConfig.js';
import TrackVisualizer from './modules/ui/TrackVisualizer.js';
import ControlPanel from './modules/ui/ControlPanel.js';

// 2. Cesium viewer'ı başlat
const viewer = new Cesium.Viewer('cesiumContainer');

// 3. TCP client oluştur
const tcpClient = new TcpClient();

// 4. Event listener'lar ekle
tcpClient.on('statusChanged', (data) => {
  console.log('Status:', data.status);
});

tcpClient.on('messageReceived', (msg) => {
  console.log('Message:', msg);
});

// 5. Bağlan
await tcpClient.connect('127.0.0.1', 9001);

// 6. Track görselleştirici oluştur
const visualizer = new TrackVisualizer(viewer);

// 7. Track'leri gönder ve görselleştir
const tracks = [...];
await tcpClient.sendTracks(tracks);
visualizer.visualizeAllTracks(tracks);

// 8. Kontrol paneli oluştur
const panel = new ControlPanel(tcpClient);
panel.updateTrackList(tracks);
```

---

## ⚠️ Error Handling

### Connection Errors

```javascript
tcpClient.on('connectionFailed', (error) => {
  console.error('Connection failed:', error);
  // Yeniden bağlanma otomatik
});
```

### Send Errors

```javascript
try {
  await tcpClient.sendTracks(tracks);
} catch (error) {
  console.error('Send failed:', error.message);
  // Kullanıcıya bildir
}
```

### Visualization Errors

```javascript
try {
  visualizer.visualizeTrack(track);
} catch (error) {
  console.error('Visualization error:', error);
  // Geçersiz waypoint veya koordinat
}
```

---

**API Versiyon**: 1.0.0  
**Son Güncelleme**: Mart 2026
