# Riskler ve Kararlar

## Riskler

| Risk | Etki | Önlem |
|------|------|-------|
| State drift (Cesium ↔ Qt) | Tutarsız görüntü | Cesium'da state tutma, periyodik full-sync, tick sayacı |
| Çift otorite geliştiricisi | Dev Cesium'a state eklerse | Mimari kural: Cesium = render slave |
| QWebEngineView GPU crash | Uygulama çöker | Crash handler, state recovery |
| Qt/JS boundary karmaşası | Debug zorluğu | Tüm mesajları logla, JSON Schema |
| Render-sim coupling | Frame rate bağımlı fizik | Ayrı QTimer, Cesium clock'a bağlanma |
| İlk fazda over-engineering | Gecikme, karmaşıklık | YAGNI prensibi |
| CORS/WebGL QWebEngine | Tile yüklenemez | COOP/COEP header, GPU flag test |

## Alınan Kararlar

1. Qt = tek otorite → Cesium'da state tutulmaz
2. Hibrit sync (full + delta) → performans + güvenilirlik
3. 20Hz tick → insan algısı için yeterli, CPU friendly
4. Kinematik model ilk → fizik modeli ileride
5. Tek app iki pencere → process yönetimi basit
6. İkinci Cesium view yok (ilk fazda) → GPU tasarrufu
7. Undo/redo yok (ilk fazda) → karmaşıklık azaltma
8. Keyboard Qt'de → güvenilirlik
