/**
 * CesiumApp.js — Viewer init, terrain, imagery, clock, lighting
 *
 * Extracted from original main.js. This is the Cesium viewer bootstrap.
 * No business logic here — only map setup.
 *
 * PERFORMANCE NOTES (Qt-embedded mode):
 * - OSM Buildings REMOVED (heaviest single layer, 3-5x slowdown)
 * - Fog, atmosphere, ground atmosphere DISABLED
 * - FXAA disabled, shadows disabled
 * - requestRenderMode ON (only renders when needed)
 * - maximumScreenSpaceError = 8 (fewer terrain tile requests)
 * - tileCacheSize = 1000 (keep tiles in memory)
 */

import {
  Cartesian3,
  Math as CesiumMath,
  Terrain,
  Viewer,
  ImageryLayer,
  IonWorldImageryStyle,
  JulianDate,
  Matrix4,
  Ion,
} from "cesium";
import "cesium/Build/Cesium/Widgets/widgets.css";
import "../style.css";

export class CesiumApp {
  constructor() {
    this.viewer = null;
  }

  async init(containerId = "cesiumContainer") {
    // Cesium ion access token
    Ion.defaultAccessToken =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI4MzA4Mjc0MS1jY2M0LTRlYmQtYjc5My01OGQ4Yzk0OTMzMDAiLCJpZCI6MzU2NTM4LCJpYXQiOjE3NjIzMjU3NzB9.GnAL6LKzbzx6QcW8vprAwdkMHsWraP46l30QiQYduOU";

    // Create viewer with terrain — minimal config for performance
    this.viewer = new Viewer(containerId, {
      terrain: Terrain.fromWorldTerrain(),
      infoBox: false,
      shadows: false,
      shouldAnimate: true,
      requestRenderMode: true,
      maximumRenderTimeChange: 0.0, // render every frame when animating
    });

    // Aerial imagery with labels
    const mapLayer = ImageryLayer.fromWorldImagery({
      style: IonWorldImageryStyle.AERIAL_WITH_LABELS,
    });
    this.viewer.imageryLayers.add(mapLayer);

    // ===== PERFORMANCE TUNING =====
    const scene = this.viewer.scene;

    // Terrain: higher error = less tiles = faster loading & rendering
    scene.globe.maximumScreenSpaceError = 8;

    // Tile cache: keep loaded tiles in memory
    scene.globe.tileCacheSize = 1000;

    // Disable expensive post-processing
    scene.fxaa = false;
    scene.fog.enabled = false;
    scene.globe.showGroundAtmosphere = false;
    if (scene.skyAtmosphere) scene.skyAtmosphere.show = false;

    // Disable lighting computation (flat shading, much faster)
    scene.globe.enableLighting = false;

    // Logarithmic depth buffer: keep ON (prevents z-fighting)
    scene.logarithmicDepthBuffer = true;

    // ===== NO OSM BUILDINGS =====
    // Intentionally omitted — heaviest single layer
    // Can be re-enabled via: createOsmBuildingsAsync()

    // Clock
    const customTime = JulianDate.fromDate(
      new Date(Date.UTC(2025, 5, 10, 3, 0, 0))
    );
    this.viewer.clock.currentTime = customTime;
    this.viewer.clock.multiplier = 60;

    // Initial camera: Istanbul overview
    this.setCamera();

    // Show FPS counter in debug builds
    scene.debugShowFramesPerSecond = true;

    console.log("[CesiumApp] Viewer initialized (Qt optimized — no OSM buildings, reduced terrain LOD)");
    return this.viewer;
  }

  setCamera() {
    this.viewer.camera.lookAtTransform(Matrix4.IDENTITY);
    this.viewer.camera.flyTo({
      destination: Cartesian3.fromDegrees(28.9784, 41.0082, 50000),
      orientation: {
        heading: CesiumMath.toRadians(0.0),
        pitch: CesiumMath.toRadians(-45.0),
        range: 50000.0,
      },
      duration: 2,
    });
  }
}

export default CesiumApp;
