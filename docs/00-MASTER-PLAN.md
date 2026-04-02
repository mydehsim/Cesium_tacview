# Master Plan — Qt 6 + CesiumJS Gömülü Masaüstü Simülasyon

> Son güncelleme: 2026-04-02 — Faz 1 tamamlandı

## Executive Summary

Qt 6 `QMainWindow` ana kabuk, CesiumJS `QWebEngineView` içinde gömülü render katmanı,
`QWebChannel` üzerinden JSON mesajlaşma ile çift yönlü haberleşme.
Qt tüm state'in tek otoritesi, Cesium render slave.

## Mevcut Durum (Faz 1 ✅)

- **46 C++ kaynak dosyası** derlenip linkleniyor (MSVC 2022 + Ninja)
- **10 JS modülü** (5 mevcut + 5 yeni bridge modülü) hazır
- Qt penceresinde CesiumJS gömülü yükleniyor (Vite dev server)
- QWebChannel bağlantısı aktif + `qwebchannel.js` injection çalışıyor
- JS console çıktıları Qt Simulation Log paneline yönlendiriliyor
- GPU/WebGL ayarları page load öncesinde yapılandırılıyor
- Remote debugging port 9222 aktif (JS breakpoint desteği)
- CMakePresets.json ile VS Code CMake Tools entegrasyonu tamam
- launch.json + tasks.json ile F5 debug + Ctrl+Shift+B build

## 5 Fazlı Yol Haritası

| Faz | Durum | Amaç | Çıktı | Başarı Kriteri |
|-----|-------|------|-------|----------------|
| 1 | ✅ Done | Çalışan minimal sistem | Qt + gömülü Cesium + QWebChannel + UI panelleri | Cesium Qt'de render, JS console görünür |
| 2 | 🔄 Next | Çift yönlü sync + seçim | Bridge aktif, entity CRUD, çift yönlü seçim | Qt buton→Cesium entity, tık→Qt seçim |
| 3 | ⬜ | Route editing + aircraft control | SimulationEngine 20Hz, WASD kontrol, autopilot | WASD sürme + rota takibi + mod geçişi |
| 4 | ⬜ | İki ekran + debug paneli | DebugWindow, Inspector, Log, CommandHistory | İki monitörde ayrı pencereler |
| 5 | ⬜ | Protokol bridge + gelişmiş | IProtocolAdapter, TacviewAdapter, playback | Tacview replay + harici veri |

## Faz 2 Sprint (Sonraki 2 Hafta)

| Gün | Görev |
|-----|-------|
| 1-2 | main.js'e QWebChannel bağlantı kodu ekle (bridge varsa aktif, yoksa standalone) |
| 3-4 | EntityRenderer.js'i main.js'deki balloon/track kodundan besle |
| 5-6 | InteractionHandler.js → tıklama event'lerini Qt'ye gönder |
| 7-8 | Qt "Create Aircraft" butonu → Cesium'da marker görünmesi |
| 9-10 | Cesium tıklama → Qt Inspector + AircraftListPanel güncellenmesi |

## İlk PoC Kapsamı (Faz 1 — Tamamlandı)

**Dahil:** Qt pencere + gömülü Cesium + QWebChannel + JS console capture + 
AircraftState/RouteState modelleri + AircraftList/Inspector/RouteEditor/Log panelleri +
SimulationEngine iskelet + InputManager + CesiumBridge + StateSerializer + Demo senaryo

**Hariç (Faz 2+):** Aktif bridge üzerinden entity sync, klavye kontrol, route editing,
iki ekran, TCP bridge, senaryo dosyası, Tacview
