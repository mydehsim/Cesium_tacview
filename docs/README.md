# CesiumJS Workshop - TCP Track Injection System

## 📋 Proje Genel Bakış

Bu proje, CesiumJS tabanlı bir 3D görselleştirme sistemidir. Tacview benzeri bir uygulama ile TCP üzerinden track (iz) verilerini alıp, CesiumJS haritasında gerçek zamanlı olarak görselleştirmek için geliştirilmiştir.

### Temel Özellikler

- ✅ **TCP/WebSocket Bağlantısı**: Tacview TrackInjector benzeri protokol desteği
- ✅ **Gerçek Zamanlı Track Görselleştirme**: Uçak, helikopter, nakliye araçları vb.
- ✅ **Interaktif Kontrol Paneli**: Bağlantı yönetimi ve track kontrolü
- ✅ **XML Tabanlı Veri Protokolü**: Tacview uyumlu format
- ✅ **Çoklu Waypoint Desteği**: Her track için zaman bazlı rota noktaları
- ✅ **3D Görselleştirme**: Cesium terrain ve OSM yapıları ile gerçekçi görünüm

---

## 🏗️ Mimari

### Proje Yapısı

```
cesiumjs-workshop/
├── src/
│   ├── main.js                    # Ana uygulama giriş noktası
│   ├── style.css                  # Genel stiller
│   └── modules/
│       ├── tcp/                   # TCP bağlantı modülleri
│       │   ├── TcpClient.js       # WebSocket/TCP istemci yönetimi
│       │   ├── TcpConfig.js       # Bağlantı konfigürasyon yönetimi
│       │   └── XmlBuilder.js      # XML format builder
│       └── ui/                    # Kullanıcı arayüzü modülleri
│           ├── ControlPanel.js    # Sol panel kontrol arayüzü
│           └── TrackVisualizer.js # Track görselleştirme motoru
├── docs/                          # Dokümantasyon
│   ├── README.md                  # Bu dosya
│   ├── ARCHITECTURE.md            # Detaylı mimari dokümantasyon
│   ├── API.md                     # API referansı
│   └── USAGE.md                   # Kullanım kılavuzu
└── public/                        # Statik dosyalar
```

### Modül İlişkileri

```
main.js
  ├── TcpClient (TCP bağlantı yönetimi)
  │   ├── TcpConfig (Konfigürasyon)
  │   └── XmlBuilder (Veri formatı)
  ├── ControlPanel (UI yönetimi)
  │   └── TcpClient referansı
  └── TrackVisualizer (Görselleştirme)
      └── Cesium Viewer referansı
```

---

## 🚀 Hızlı Başlangıç

### Gereksinimler

- Node.js (v16+)
- npm veya yarn
- Modern web tarayıcı (Chrome, Firefox, Edge)

### Kurulum

```bash
# Bağımlılıkları yükle
npm install

# Geliştirme sunucusunu başlat
npm run dev

# Tarayıcıda aç: http://localhost:5173
```

### Temel Kullanım

1. **TCP Sunucusu**: Tacview veya benzeri bir TCP sunucusu başlatın (varsayılan: `127.0.0.1:9001`)
2. **Panel Açma**: Sayfa yüklendiğinde sol panel otomatik açılır
3. **Bağlantı**: Panel'den "Bağlan" butonuna basın
4. **Track Gönderme**: "Track'leri Gönder" butonu ile örnek verileri gönderin

---

## 📡 TCP Protokolü

### Bağlantı Akışı

1. Client -> Server: WebSocket bağlantısı (`ws://host:port`)
2. Client -> Server: XML track verisi gönderimi
3. Server -> Client: Onay/hata mesajı
4. Client -> Server: Reset komutu (isteğe bağlı)

### XML Format Örneği

```xml
<?xml version="1.0" encoding="utf-8"?>
<Tracks>
  <Track>
    <Number>1</Number>
    <CallSign>ALPHA-01</CallSign>
    <Type>Fighter</Type>
    <Color>FF0000</Color>
    <EngagementRange>0</EngagementRange>
    <VerticalEngagementRange>0</VerticalEngagementRange>
    <IsSelected>false</IsSelected>
    <Waypoints>
      <Waypoint>
        <Latitude>41.0082</Latitude>
        <Longitude>28.9784</Longitude>
        <Altitude>5000</Altitude>
        <Time>0</Time>
      </Waypoint>
      <!-- Daha fazla waypoint... -->
    </Waypoints>
  </Track>
</Tracks>
```

---

## 🎨 Özellikler Detay

### 1. TCP İstemci Yönetimi (TcpClient)

- WebSocket tabanlı bağlantı
- Otomatik yeniden bağlanma (5 deneme, 3sn gecikme)
- Event-based mimari (statusChanged, messageReceived, vb.)
- LocalStorage ile bağlantı ayarları kaydetme

### 2. Kontrol Paneli (ControlPanel)

- 3 sekmeli arayüz: Bağlantı, Track'ler, Ayarlar
- Gerçek zamanlı bağlantı durumu göstergesi
- Track listesi ve seçim yönetimi
- Log konsolu

### 3. Track Görselleştirme (TrackVisualizer)

- Polyline ile rota çizimi
- Marker ve label ile track işaretleme
- Tip bazlı renklendirme (Fighter: Kırmızı, Transport: Mavi, vb.)
- Otomatik kamera odaklanma

### 4. XML Builder

- Tacview uyumlu XML formatı
- Track ve waypoint serileştirme
- XML özel karakter escape işlemi

---

## 🔧 Konfigürasyon

### TcpConfig Varsayılan Değerler

```javascript
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9001
STORAGE_KEY = "cesium_tcp_config"
```

### Cesium Ion Access Token

`src/main.js` dosyasında token'ı güncelleyin:

```javascript
Ion.defaultAccessToken = "YOUR_TOKEN_HERE";
```

---

## 🧪 Test Etme

### Konsoldan Test Komutları

Tarayıcı konsolunda şu komutları kullanabilirsiniz:

```javascript
// Panel'i aç/kapat
window.toggleTcpPanel();

// Örnek track'leri gönder
await window.sendSampleTracks();

// Telemetriyi sıfırla
await window.resetTelemetry();

// TCP istemciye erişim
window.tcpClient.connect();
window.tcpClient.disconnect();

// Track görselleştirici
window.trackVisualizer.visualizeAllTracks(tracks);
```

---

## 📚 Daha Fazla Bilgi

- [Detaylı Mimari](./ARCHITECTURE.md) - Sistem tasarımı ve veri akışı
- [API Referansı](./API.md) - Modül ve fonksiyon dokümantasyonu
- [Kullanım Kılavuzu](./USAGE.md) - Adım adım kullanım senaryoları

---

## 🤝 Katkıda Bulunma

Bu proje, Tacview SDK ve CesiumJS Workshop temel alınarak geliştirilmiştir.

### İlgili Projeler

- [CesiumJS](https://cesium.com/platform/cesiumjs/)
- [Tacview](https://www.tacview.net/)

---

## 📝 Lisans

Apache License 2.0 - Detaylar için [LICENSE](../LICENSE) dosyasına bakın.

---

## 📞 İletişim & Destek

Sorularınız için issue açabilir veya pull request gönderebilirsiniz.

**Geliştirme Tarihi**: Mart 2026  
**Versiyon**: 1.0.0
