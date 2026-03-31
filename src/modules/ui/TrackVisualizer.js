/**
 * Track Visualizer Module
 * Cesium haritasında Track'leri polyline, marker ve label olarak göster
 */

import {
  Cartesian3,
  Cartesian2,
  Color,
  PolylineGraphics,
  Entity,
  LabelGraphics,
  PointGraphics,
  CallbackProperty,
  Cartographic,
  Ellipsoid,
} from "cesium";

export class TrackVisualizer {
  constructor(viewer) {
    this.viewer = viewer;
    this.trackEntities = new Map(); // trackId -> entity
    this.trackColors = {
      Fighter: Color.RED,
      Transport: Color.BLUE,
      Helicopter: Color.GREEN,
      Unknown: Color.YELLOW,
    };
  }

  /**
   * Track'i haritada görselleştir
   */
  visualizeTrack(track) {
    if (!track || !track.waypoints || track.waypoints.length === 0) {
      console.warn("[TrackVisualizer] Empty track");
      return;
    }

    const trackId = `track_${track.number || track.callSign}`;

    // Eski entity varsa sil
    if (this.trackEntities.has(trackId)) {
      const oldEntity = this.trackEntities.get(trackId);
      this.viewer.entities.remove(oldEntity);
    }

    // Waypoint'leri Cartesian3'e çevir
    const positions = track.waypoints
      .map((wp) => {
        try {
          return Cartesian3.fromDegrees(
            wp.longitude || 0,
            wp.latitude || 0,
            wp.altitude || 0
          );
        } catch (e) {
          console.error("[TrackVisualizer] Error converting waypoint:", wp, e);
          return null;
        }
      })
      .filter((p) => p); // null'ları filtrele

    if (positions.length === 0) {
      console.warn("[TrackVisualizer] No valid positions");
      return;
    }

    const color = this.getColorForType(track.type);

    // Polyline entity oluştur (Track yolunu çiz)
    const polylineEntity = this.viewer.entities.add({
      name: `Track: ${track.callSign}`,
      polyline: new PolylineGraphics({
        positions: positions,
        width: 3,
        material: color,
        clampToGround: true,
      }),
    });

    // Başlangıç noktasına marker ekle
    if (positions.length > 0) {
      this.viewer.entities.add({
        position: positions[0],
        point: new PointGraphics({
          pixelSize: 8,
          color: color,
          outlineColor: Color.WHITE,
          outlineWidth: 2,
        }),
        label: new LabelGraphics({
          text: track.callSign || "Unknown",
          font: "12px Arial",
          fillColor: Color.WHITE,
          outlineColor: Color.BLACK,
          outlineWidth: 2,
          pixelOffset: new Cartesian2(0, 20),
          showBackground: true,
          backgroundColor: new Color(0, 0, 0, 0.5),
          backgroundPadding: new Cartesian2(4, 4),
        }),
      });
    }

    // Son noktasına destination marker ekle
    if (positions.length > 1) {
      this.viewer.entities.add({
        position: positions[positions.length - 1],
        point: new PointGraphics({
          pixelSize: 6,
          color: Color.WHITE,
          outlineColor: color,
          outlineWidth: 2,
        }),
      });
    }

    this.trackEntities.set(trackId, polylineEntity);

    console.log(
      `[TrackVisualizer] Track '${track.callSign}' visualized with ${positions.length} waypoints`
    );

    return polylineEntity;
  }

  /**
   * Tüm track'leri görselleştir
   */
  visualizeAllTracks(tracks) {
    if (!Array.isArray(tracks)) {
      console.warn("[TrackVisualizer] Invalid tracks array");
      return;
    }

    this.clearAll();

    tracks.forEach((track) => {
      try {
        this.visualizeTrack(track);
      } catch (error) {
        console.error(
          `[TrackVisualizer] Error visualizing track: ${error.message}`
        );
      }
    });

    // Kamera'yı tüm track'leri görecek şekilde ayarla
    if (this.trackEntities.size > 0) {
      this.zoomToAll();
    }
  }

  /**
   * Track türüne göre renk al
   */
  getColorForType(type) {
    if (!type) return this.trackColors.Unknown;

    const typeUpper = type.toUpperCase();

    for (const [key, color] of Object.entries(this.trackColors)) {
      if (typeUpper.includes(key.toUpperCase())) {
        return color;
      }
    }

    return this.trackColors.Unknown;
  }

  /**
   * Tüm track'leri haritadan sil
   */
  clearAll() {
    this.trackEntities.forEach((entity) => {
      this.viewer.entities.remove(entity);
    });
    this.trackEntities.clear();

    console.log("[TrackVisualizer] All tracks cleared");
  }

  /**
   * Tüm track'leri görecek şekilde kamera ayarla
   */
  zoomToAll() {
    try {
      if (this.trackEntities.size === 0) return;

      const allPositions = [];

      this.trackEntities.forEach((entity) => {
        if (
          entity.polyline &&
          entity.polyline.positions &&
          entity.polyline.positions.getValue
        ) {
          const positions = entity.polyline.positions.getValue();
          if (Array.isArray(positions)) {
            allPositions.push(...positions);
          }
        }
      });

      if (allPositions.length > 0) {
        this.viewer.camera.flyToBoundingSphere(
          this.calculateBoundingSphere(allPositions),
          { duration: 1.5 }
        );
      }
    } catch (error) {
      console.error(`[TrackVisualizer] Zoom error: ${error.message}`);
    }
  }

  /**
   * Bounding sphere hesapla
   */
  calculateBoundingSphere(positions) {
    if (positions.length === 0) return null;

    let minLat = Infinity,
      maxLat = -Infinity;
    let minLon = Infinity,
      maxLon = -Infinity;
    let maxAlt = 0;

    positions.forEach((position) => {
      const cartographic = Cartographic.fromCartesian(position);
      const lat = cartographic.latitude * (180 / Math.PI);
      const lon = cartographic.longitude * (180 / Math.PI);
      const alt = cartographic.height;

      minLat = Math.min(minLat, lat);
      maxLat = Math.max(maxLat, lat);
      minLon = Math.min(minLon, lon);
      maxLon = Math.max(maxLon, lon);
      maxAlt = Math.max(maxAlt, alt);
    });

    const centerLat = (minLat + maxLat) / 2;
    const centerLon = (minLon + maxLon) / 2;

    const center = Cartesian3.fromDegrees(
      centerLon,
      centerLat,
      maxAlt + 50000
    );
    const radius = Cartesian3.distance(
      center,
      Cartesian3.fromDegrees(centerLon, minLat, maxAlt)
    );

    return { center, radius };
  }

  /**
   * Belirli track'i sil
   */
  removeTrack(trackId) {
    if (this.trackEntities.has(trackId)) {
      const entity = this.trackEntities.get(trackId);
      this.viewer.entities.remove(entity);
      this.trackEntities.delete(trackId);

      console.log(`[TrackVisualizer] Track '${trackId}' removed`);
    }
  }

  /**
   * Belirli track'i güncelle
   */
  updateTrack(track) {
    this.removeTrack(`track_${track.number || track.callSign}`);
    this.visualizeTrack(track);
  }

  /**
   * Track'leri renklendir (interaktif seçim için)
   */
  highlightTrack(trackId, isHighlight = true) {
    if (this.trackEntities.has(trackId)) {
      const entity = this.trackEntities.get(trackId);
      if (entity.polyline) {
        entity.polyline.width = isHighlight ? 5 : 3;
      }
    }
  }
}

export default TrackVisualizer;
