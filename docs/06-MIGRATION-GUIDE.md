# Mevcut Koddan Evrilme Rehberi

> Son güncelleme: 2026-04-02

## Mevcut Durum

Qt penceresi `http://localhost:5173` üzerinden orijinal `index.html` + `main.js`'i
yüklüyor. Tüm mevcut HTML panelleri (Route Configuration, Object Management, Entity
Selector) web view içinde görünür durumda. Ayrıca Qt tarafında eşdeğer paneller
(AircraftList, Inspector, RouteEditor) oluşturuldu. **Sonraki adım:** Bridge aktif
hale getirilerek web panellerini kademeli olarak kaldırıp Qt panellerine taşımak.

## Evrilme Adımları (Güncellenmiş)

### Adım 1 — ✅ Tamamlandı: Gömme
main.js olduğu gibi QWebEngineView'e yüklendi, çalışıyor.

### Adım 2 — ✅ Tamamlandı: Bridge İskelet
QWebChannel + CesiumBridge.cpp + qwebchannel.js injection hazır.

### Adım 3 — 🔄 Sonraki: Bridge Aktivasyonu
main.js'e QWebChannel init kodu eklenmeli (yoksa standalone devam):
```js
// main.js'in sonuna eklenecek:
if (window.qt && window.qt.webChannelTransport) {
  new QWebChannel(qt.webChannelTransport, (channel) => {
    window.qtBridge = channel.objects.qtBridge;
    qtBridge.onCesiumReady(); // Qt'ye hazır sinyali
  });
}
```

### Adım 4 — Entity selector'ı Qt paneline taşı (HTML DOM silinir)
### Adım 5 — Control panel'ı Qt'ye taşı  
### Adım 6 — Route editor'ı Qt'ye taşı
### Adım 7 — main.js'yi 5 modüle böl (CesiumApp/Bridge/Renderer/Interaction/Camera)
### Adım 8 — ControlPanel.js + TcpClient.js + TcpConfig.js import zincirinden çıkar
