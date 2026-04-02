# QWebChannel Bridge Protokolü

## Bağlantı Modeli

- C++ tarafı: `CesiumBridge` (QObject, QWebChannel'a register)
- JS tarafı: `CesiumBridge.js` (qwebchannel.js ile Qt nesnesine erişim)
- Dev modda: Vite `http://localhost:5173` → `QWebEngineView::setUrl()`
- Production: `vite build` çıktısı uygulama yanında dağıtılır

## Sync Stratejisi — Hibrit

- `STATE_FULL_SYNC`: İlk bağlantı + büyük değişiklikler
- `STATE_DELTA`: Her sim tick'te yalnızca değişen aircraft pozisyonları
- Her mesaja `tick` numarası, Cesium tick kaçırırsa `REQ_FULL_SYNC`

## Mesaj Tipleri (Qt → Cesium)

| Tip | Açıklama |
|-----|----------|
| STATE_FULL_SYNC | Tüm aircraft + route + selection state |
| STATE_DELTA | Yalnızca değişen pozisyon/heading/speed |
| CMD_CREATE_ENTITY | Yeni entity oluştur |
| CMD_REMOVE_ENTITY | Entity sil |
| CMD_SELECT_ENTITY | Seçimi değiştir |
| CMD_CAMERA_FLY_TO | Kamerayı hedefe uçur |
| CMD_CAMERA_TRACK | Kamerayı entity'ye kilitle |
| CMD_UPDATE_ROUTE | Rota waypoint'lerini güncelle |
| CMD_HIGHLIGHT_ENTITY | Entity vurgula/kaldır |

## Mesaj Tipleri (Cesium → Qt)

| Tip | Açıklama |
|-----|----------|
| EVT_ENTITY_CLICKED | Entity'ye tıklandı |
| EVT_ENTITY_DOUBLE_CLICKED | Entity'ye çift tıklandı |
| EVT_MAP_CLICKED | Harita boş alana tıklandı |
| EVT_WAYPOINT_MOVED | Waypoint sürüklendi |
| EVT_WAYPOINT_ADDED | Haritadan waypoint eklendi |
| EVT_CAMERA_CHANGED | Kamera pozisyonu değişti |
| EVT_SELECTION_CHANGED | Cesium içi seçim değişti |
| EVT_CONTEXT_MENU | Sağ tık menüsü tetiklendi |
| EVT_READY | Cesium viewer hazır |
| REQ_FULL_SYNC | Full state talep et |

## JSON Şema Örnekleri

### Qt → Cesium (STATE_DELTA)
```json
{
  "type": "STATE_DELTA",
  "tick": 4523,
  "timestamp": 1711900800500,
  "aircraft": {
    "ac_1": {
      "lat": 41.0082, "lon": 28.9784, "alt": 5000,
      "heading": 45.0, "speed": 250.0
    }
  },
  "selection": {"entityId": "ac_1", "type": "aircraft"}
}
```

### Cesium → Qt (Event)
```json
{
  "type": "EVT_WAYPOINT_MOVED",
  "payload": {
    "waypointId": "wp_2",
    "routeId": "route_1",
    "newPosition": {"lat": 41.01, "lon": 28.98, "alt": 300}
  },
  "timestamp": 1711900800500
}
```

### Qt → Cesium (Command)
```json
{
  "type": "CMD_CREATE_ENTITY",
  "payload": {
    "id": "ac_5",
    "callSign": "ECHO-05",
    "entityType": "aircraft",
    "modelUri": "/models/f16-c_falcon.glb",
    "position": {"lat": 41.0, "lon": 29.0, "alt": 5000},
    "heading": 90.0
  }
}
```
