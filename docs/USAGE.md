# Kullanım Kılavuzu

Bu kılavuz, CesiumJS Workshop TCP Track Injection sistemini adım adım nasıl kullanacağınızı açıklar.

---

## 📋 İçindekiler

1. [İlk Kurulum](#ilk-kurulum)
2. [TCP Sunucusu Hazırlama](#tcp-sunucusu-hazırlama)
3. [Temel Kullanım](#temel-kullanım)
4. [İleri Seviye Kullanım](#ileri-seviye-kullanım)
5. [Sorun Giderme](#sorun-giderme)
6. [Kullanım Senaryoları](#kullanım-senaryoları)

---

## 🚀 İlk Kurulum

### Adım 1: Projeyi İndirin

```bash
git clone <repository-url>
cd cesiumjs-workshop
```

### Adım 2: Bağımlılıkları Yükleyin

```bash
npm install
```

### Adım 3: Cesium Ion Token Ayarlayın

[src/main.js](../src/main.js) dosyasını açın ve token'ınızı girin:

```javascript
Ion.defaultAccessToken = "YOUR_TOKEN_HERE";
```

**Token nasıl alınır:**
1. [Cesium Ion](https://cesium.com/ion/) hesabı oluşturun
2. Dashboard'dan Access Token oluşturun
3. Token'ı kopyalayıp main.js'e yapıştırın

### Adım 4: Geliştirme Sunucusunu Başlatın

```bash
npm run dev
```

Tarayıcınızda otomatik olarak `http://localhost:5173` açılır.

---

## 🖥️ TCP Sunucusu Hazırlama

### Seçenek 1: Tacview TrackInjector (C#)

Eğer C# TrackInjector kullanıyorsanız:

1. TrackInjector uygulamasını başlatın
2. `Tools > TCP Server Settings` menüsünden:
   - Port: `9001` (varsayılan)
   - Start Server

### Seçenek 2: Node.js Test Sunucusu

Basit bir WebSocket test sunucusu:

```javascript
// test-server.js
const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 9001 });

wss.on('connection', (ws) => {
  console.log('Client connected');
  
  ws.on('message', (message) => {
    console.log('Received:', message.toString());
    
    // Echo back
    ws.send('ACK: Data received');
  });
  
  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

console.log('WebSocket server running on port 9001');
```

Çalıştırın:
```bash
node test-server.js
```

### Seçenek 3: Python Test Sunucusu

```python
# test-server.py
import asyncio
import websockets

async def handle_client(websocket, path):
    print("Client connected")
    try:
        async for message in websocket:
            print(f"Received: {message}")
            await websocket.send("ACK: Data received")
    except websockets.exceptions.ConnectionClosed:
        print("Client disconnected")

async def main():
    async with websockets.serve(handle_client, "127.0.0.1", 9001):
        print("WebSocket server running on port 9001")
        await asyncio.Future()  # run forever

asyncio.run(main())
```

Çalıştırın:
```bash
pip install websockets
python test-server.py
```

---

## 🎯 Temel Kullanım

### 1. Uygulamayı Açın

Tarayıcıda `http://localhost:5173` adresini açın.

**Otomatik İşlemler:**
- ✅ TCP paneli sol tarafta açılır
- ✅ Otomatik bağlantı denemesi başlar
- ✅ Örnek track'ler haritada gösterilir
- ✅ Kamera İstanbul bölgesine odaklanır

### 2. Panel Kullanımı

#### Bağlantı Sekmesi (📡 Bağlantı)

**Durum Göstergesi:**
- 🔴 Kırmızı: Bağlı değil
- 🟡 Sarı: Bağlanıyor...
- 🟢 Yeşil: Bağlı

**Bağlantı Ayarları:**
- **Host**: Sunucu IP adresi (varsayılan: 127.0.0.1)
- **Port**: Port numarası (varsayılan: 9001)

**Butonlar:**
- **🔗 Bağlan**: Sunucuya bağlan
- **🔌 Bağlantıyı Kes**: Mevcut bağlantıyı kapat

**Log Konsolu:**
Tüm bağlantı olayları burada görüntülenir.

#### Track'ler Sekmesi (📍 Track'ler)

**Track Listesi:**
Her track için:
- Track numarası ve callsign
- Tip (Fighter, Transport, vb.)
- Waypoint sayısı

**Butonlar:**
- **📤 Track'leri Gönder**: Seçili track'leri sunucuya gönder
- **🔄 Reset**: Telemetriyi sıfırla

#### Ayarlar Sekmesi (⚙️ Ayarlar)

Gelecekteki konfigürasyon seçenekleri için ayrılmıştır.

### 3. Track Gönderme

**Adımlar:**

1. Sunucunun çalıştığından emin olun
2. "Bağlan" butonuna basın
3. Durum göstergesi yeşil olana kadar bekleyin
4. "Track'ler" sekmesine geçin
5. "Track'leri Gönder" butonuna basın

**Sonuç:**
- ✅ Track'ler XML formatında sunucuya gönderilir
- ✅ Log konsolunda "Tracks sent successfully" mesajını görürsünüz
- ✅ Haritada track'ler görselleşir

### 4. Harita Etkileşimi

**Kamera Kontrolü:**
- **Sol Tıklama + Sürükleme**: Kamerayı döndür
- **Sağ Tıklama + Sürükleme**: Kamerayı kaydır
- **Tekerlek**: Zoom in/out
- **Orta Tıklama + Sürükleme**: Eğim ayarla

**Track İnceleme:**
- Track'e tıklayarak detayları görebilirsiniz
- Polyline track'in rotasını gösterir
- Marker başlangıç noktasını işaretler
- Label track callsign'ını gösterir

---

## 🔥 İleri Seviye Kullanım

### Konsoldan Manuel Kontrol

Tarayıcı konsolunu açın (F12) ve şu komutları kullanın:

#### Panel Kontrolü

```javascript
// Panel'i aç/kapat
window.toggleTcpPanel();

// Panel durumunu kontrol et
console.log(window.tcpControlPanel.isOpen);

// Sekme değiştir
window.tcpControlPanel.switchTab('tracks');
```

#### TCP İşlemleri

```javascript
// Manuel bağlantı
await window.tcpClient.connect('192.168.1.100', 9001);

// Bağlantı durumu
console.log(window.tcpClient.isConnected);

// Manuel track gönderme
await window.sendSampleTracks();

// Özel track gönderme
const customTracks = [{
  number: 99,
  callSign: "CUSTOM-99",
  type: "Fighter",
  color: "00FF00",
  waypoints: [
    { latitude: 40.0, longitude: 29.0, altitude: 3000, time: 0 },
    { latitude: 40.5, longitude: 29.5, altitude: 3500, time: 60 }
  ]
}];
await window.tcpClient.sendTracks(customTracks);

// Reset telemetry
await window.resetTelemetry();

// Bağlantıyı kes
window.tcpClient.disconnect();
```

#### Görselleştirme İşlemleri

```javascript
// Tüm track'leri temizle
window.trackVisualizer.clearAllTracks();

// Yeni track'leri görselleştir
window.trackVisualizer.visualizeAllTracks(window.sampleTracks);

// Tek track görselleştir
window.trackVisualizer.visualizeTrack(window.sampleTracks[0]);

// Track'ı odakla
window.trackVisualizer.focusOnTrack('track_1');
```

#### Event Listening

```javascript
// Bağlantı durumu değişikliklerini dinle
window.tcpClient.on('statusChanged', (data) => {
  console.log('Status:', data.status);
  console.log('Message:', data.message);
});

// Gelen mesajları dinle
window.tcpClient.on('messageReceived', (msg) => {
  console.log('Server said:', msg);
});

// Bağlantı hatalarını dinle
window.tcpClient.on('connectionFailed', (error) => {
  console.error('Connection error:', error);
});
```

### Özel Track Oluşturma

#### Örnek 1: Basit Rota

```javascript
const basicTrack = {
  number: 1,
  callSign: "TEST-01",
  type: "Fighter",
  color: "FF0000", // Kırmızı
  waypoints: [
    { latitude: 41.0, longitude: 28.9, altitude: 5000, time: 0 },
    { latitude: 41.1, longitude: 29.0, altitude: 5500, time: 60 },
    { latitude: 41.2, longitude: 29.1, altitude: 6000, time: 120 }
  ]
};

await window.tcpClient.sendTracks([basicTrack]);
window.trackVisualizer.visualizeTrack(basicTrack);
```

#### Örnek 2: Çoklu Track Operasyonu

```javascript
const multiTracks = [
  {
    number: 1,
    callSign: "BLUE-01",
    type: "Fighter",
    color: "0000FF",
    waypoints: [
      { latitude: 41.0, longitude: 28.8, altitude: 5000, time: 0 },
      { latitude: 41.2, longitude: 29.0, altitude: 5200, time: 120 }
    ]
  },
  {
    number: 2,
    callSign: "RED-02",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 40.8, longitude: 29.0, altitude: 5000, time: 0 },
      { latitude: 41.0, longitude: 29.2, altitude: 5200, time: 120 }
    ]
  },
  {
    number: 3,
    callSign: "GREEN-03",
    type: "Helicopter",
    color: "00FF00",
    waypoints: [
      { latitude: 41.0, longitude: 29.0, altitude: 1000, time: 0 },
      { latitude: 41.1, longitude: 29.1, altitude: 1200, time: 180 }
    ]
  }
];

await window.tcpClient.sendTracks(multiTracks);
window.trackVisualizer.visualizeAllTracks(multiTracks);
```

#### Örnek 3: Kompleks Rota (Çoklu Waypoint)

```javascript
const complexRoute = {
  number: 10,
  callSign: "PATROL-10",
  type: "Transport",
  color: "FFFF00", // Sarı
  waypoints: [
    { latitude: 41.0082, longitude: 28.9784, altitude: 3000, time: 0 },   // Istanbul
    { latitude: 40.8, longitude: 29.4, altitude: 3200, time: 180 },       // East
    { latitude: 40.6, longitude: 29.6, altitude: 3400, time: 360 },       // Southeast
    { latitude: 40.4, longitude: 29.4, altitude: 3300, time: 540 },       // South
    { latitude: 40.6, longitude: 29.0, altitude: 3100, time: 720 },       // West
    { latitude: 40.8, longitude: 28.8, altitude: 3000, time: 900 },       // Northwest
    { latitude: 41.0082, longitude: 28.9784, altitude: 3000, time: 1080 } // Return
  ]
};

await window.tcpClient.sendTracks([complexRoute]);
window.trackVisualizer.visualizeTrack(complexRoute);
```

### Konfigürasyon Değiştirme

```javascript
// Mevcut konfigürasyonu görüntüle
console.log(window.tcpClient.config.getHost());
console.log(window.tcpClient.config.getPort());

// Yeni konfigürasyon ayarla
window.tcpClient.config.setHostPort('192.168.1.100', 8080);

// Yeni ayarlarla bağlan
await window.tcpClient.connect();
```

---

## 🛠️ Sorun Giderme

### Problem 1: Bağlantı Kurulamıyor

**Semptomlar:**
- Durum göstergesi kırmızı kalıyor
- "Connection timeout" hatası

**Çözümler:**

1. **Sunucu Kontrolü:**
   ```bash
   # Windows
   netstat -an | findstr "9001"
   
   # Linux/Mac
   netstat -an | grep 9001
   ```
   Port dinleniyor mu kontrol edin.

2. **Firewall Kontrolü:**
   - Windows Defender Firewall'da port 9001'i açın
   - Antivirus yazılımı engellemiyor mu kontrol edin

3. **Host/Port Kontrolü:**
   - Paneldeki host/port değerlerini kontrol edin
   - Sunucu IP'si doğru mu?

4. **WebSocket Desteği:**
   - Tarayıcı konsolunda kontrol edin:
   ```javascript
   console.log(typeof WebSocket); // "function" olmalı
   ```

### Problem 2: Track'ler Görüntülenmiyor

**Semptomlar:**
- Track'ler gönderiliyor ama haritada görünmüyor

**Çözümler:**

1. **Koordinat Kontrolü:**
   ```javascript
   // Track'leri konsola yazdır
   console.log(window.sampleTracks);
   
   // Waypoint koordinatları geçerli mi?
   // Latitude: -90 ile +90 arası
   // Longitude: -180 ile +180 arası
   ```

2. **Kamera Konumu:**
   ```javascript
   // Kamerayı track'lere odakla
   window.viewer.camera.flyTo({
     destination: Cesium.Cartesian3.fromDegrees(28.9784, 41.0082, 500000)
   });
   ```

3. **Entity Kontrolü:**
   ```javascript
   // Mevcut entity'leri listele
   console.log(window.viewer.entities);
   
   // Track entity'leri
   console.log(window.trackVisualizer.trackEntities);
   ```

4. **Manuel Görselleştirme:**
   ```javascript
   // Track'leri yeniden görselleştir
   window.trackVisualizer.clearAllTracks();
   window.trackVisualizer.visualizeAllTracks(window.sampleTracks);
   ```

### Problem 3: Panel Açılmıyor

**Semptomlar:**
- Sol panel görünmüyor
- Toggle butonu çalışmıyor

**Çözümler:**

1. **DOM Kontrolü:**
   ```javascript
   // Panel element'i var mı?
   const panel = document.getElementById('tcp-control-panel');
   console.log(panel);
   
   // Panel class'ları
   console.log(panel?.className);
   ```

2. **CSS Yükleme:**
   - Tarayıcı konsolunda CSS hatası var mı kontrol edin
   - Stiller inject edildi mi:
   ```javascript
   ControlPanel.injectStyles();
   ```

3. **Manuel Açma:**
   ```javascript
   window.tcpControlPanel.isOpen = false;
   window.tcpControlPanel.togglePanel();
   ```

### Problem 4: XML Format Hatası

**Semptomlar:**
- Sunucu XML'i parse edemiyor
- "Invalid XML" hatası

**Çözümler:**

1. **XML Kontrolü:**
   ```javascript
   const xml = XmlBuilder.buildTracksXml(window.sampleTracks);
   console.log(xml);
   
   // XML'i kopyala ve online validator'da test et
   // https://www.xmlvalidation.com/
   ```

2. **Özel Karakterler:**
   ```javascript
   // Özel karakterler escape ediliyor mu?
   const test = XmlBuilder.escapeXml('<test>');
   console.log(test); // "&lt;test&gt;" olmalı
   ```

3. **Track Validation:**
   ```javascript
   // Track objelerinin yapısı doğru mu?
   const track = window.sampleTracks[0];
   console.log('Number:', track.number);
   console.log('CallSign:', track.callSign);
   console.log('Waypoints:', track.waypoints?.length);
   ```

### Problem 5: Performance Sorunları

**Semptomlar:**
- Çok sayıda track eklendiğinde kasma
- Harita yavaşlıyor

**Çözümler:**

1. **Entity Limiti:**
   ```javascript
   // En fazla 100 track ile çalışın
   const limitedTracks = allTracks.slice(0, 100);
   ```

2. **Cleanup:**
   ```javascript
   // Eski track'leri düzenli temizleyin
   window.trackVisualizer.clearAllTracks();
   ```

3. **Optimize Rendering:**
   ```javascript
   // Polyline width azaltın (performans artışı)
   // TrackVisualizer.js içinde width: 2 yapın
   ```

---

## 📚 Kullanım Senaryoları

### Senaryo 1: Hava Operasyonu Simülasyonu

**Amaç:** 4 uçaklı bir formasyon uçuşunu simüle edin.

```javascript
const formation = [
  {
    number: 1,
    callSign: "LEAD",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 41.0, longitude: 28.9, altitude: 10000, time: 0 },
      { latitude: 41.5, longitude: 29.5, altitude: 10000, time: 300 }
    ]
  },
  {
    number: 2,
    callSign: "WING-2",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 41.0, longitude: 28.95, altitude: 9800, time: 0 },
      { latitude: 41.5, longitude: 29.55, altitude: 9800, time: 300 }
    ]
  },
  {
    number: 3,
    callSign: "WING-3",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 40.95, longitude: 28.9, altitude: 9800, time: 0 },
      { latitude: 41.45, longitude: 29.5, altitude: 9800, time: 300 }
    ]
  },
  {
    number: 4,
    callSign: "WING-4",
    type: "Fighter",
    color: "FF0000",
    waypoints: [
      { latitude: 40.95, longitude: 28.95, altitude: 9600, time: 0 },
      { latitude: 41.45, longitude: 29.55, altitude: 9600, time: 300 }
    ]
  }
];

await window.tcpClient.sendTracks(formation);
window.trackVisualizer.visualizeAllTracks(formation);
```

### Senaryo 2: Arama Kurtarma Operasyonu

**Amaç:** Helikopter ve nakliye uçağı koordineli operasyon.

```javascript
const searchRescue = [
  {
    number: 1,
    callSign: "HELO-1",
    type: "Helicopter",
    color: "00FF00",
    waypoints: [
      { latitude: 41.0, longitude: 28.9, altitude: 500, time: 0 },
      { latitude: 41.05, longitude: 28.95, altitude: 500, time: 120 },
      { latitude: 41.0, longitude: 29.0, altitude: 500, time: 240 },
      { latitude: 40.95, longitude: 28.95, altitude: 500, time: 360 },
      { latitude: 41.0, longitude: 28.9, altitude: 500, time: 480 } // Return
    ]
  },
  {
    number: 2,
    callSign: "TRANS-1",
    type: "Transport",
    color: "0000FF",
    waypoints: [
      { latitude: 40.5, longitude: 28.5, altitude: 3000, time: 0 },
      { latitude: 41.0, longitude: 28.9, altitude: 3000, time: 300 } // Rendezvous
    ]
  }
];

await window.tcpClient.sendTracks(searchRescue);
window.trackVisualizer.visualizeAllTracks(searchRescue);
```

### Senaryo 3: Test & Debug

**Amaç:** Tek bir track ile sistem testini yapın.

```javascript
const debugTrack = {
  number: 999,
  callSign: "DEBUG",
  type: "Fighter",
  color: "FFFF00",
  waypoints: [
    { latitude: 41.0082, longitude: 28.9784, altitude: 5000, time: 0 }
  ]
};

console.log('Sending debug track...');
await window.tcpClient.sendTracks([debugTrack]);

console.log('Visualizing...');
window.trackVisualizer.visualizeTrack(debugTrack);

console.log('Focusing camera...');
window.trackVisualizer.focusOnTrack('track_999');

console.log('Debug complete!');
```

---

## 💡 İpuçları ve Best Practices

### Performance

1. **Batch Gönderme:** Çok sayıda track'i tek seferde gönderin
2. **Cleanup:** Kullanılmayan track'leri düzenli temizleyin
3. **Optimize Waypoints:** Gereksiz waypoint'leri azaltın

### Debugging

1. **Console Kullanımı:** `window.*` objlerini düzenli inceleyin
2. **Log Takibi:** Panel log konsolunu aktif kullanın
3. **Network Tab:** WebSocket trafiğini Chrome DevTools'da izleyin

### Güvenlik

1. **Token Güvenliği:** Cesium Ion token'ı public repo'da paylaşmayın
2. **Input Validation:** Track verilerini validate edin
3. **HTTPS:** Prodüksiyonda WSS (WebSocket Secure) kullanın

---

## 📞 Destek

Sorularınız için:
1. [GitHub Issues](repository-url/issues) açın
2. [Dokümantasyon](./README.md) okuyun
3. [API Referansı](./API.md) kontrol edin

---

**Son Güncelleme**: Mart 2026  
**Versiyon**: 1.0.0
