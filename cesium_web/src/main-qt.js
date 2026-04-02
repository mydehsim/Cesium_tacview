/**
 * main-qt.js — Bootstrap entry point for Qt-embedded mode
 *
 * This is the slim entry point that Qt's QWebEngineView loads.
 * Imports the 5 refactored modules and wires them together.
 * The original main.js is kept for standalone browser testing.
 */

import { CesiumApp } from "./modules/CesiumApp.js";
import { CesiumBridge } from "./modules/CesiumBridge.js";
import { EntityRenderer } from "./modules/EntityRenderer.js";
import { InteractionHandler } from "./modules/InteractionHandler.js";
import { CameraController } from "./modules/CameraController.js";

// ===== Module instances =====
let cesiumApp;
let bridge;
let renderer;
let interaction;
let camera;

async function main() {
  console.log("[main-qt] Starting Qt-embedded Cesium...");

  // 1. Initialize Cesium viewer
  cesiumApp = new CesiumApp();
  const viewer = await cesiumApp.init("cesiumContainer");

  // 2. Create bridge (connects to Qt via QWebChannel)
  bridge = new CesiumBridge();

  // 3. Create render modules (need viewer)
  renderer = new EntityRenderer(viewer);
  camera = new CameraController(viewer);

  // 4. Create interaction handler (needs viewer + bridge)
  interaction = new InteractionHandler(viewer, bridge);

  // 5. Wire bridge callbacks
  bridge.onStateUpdate = (msg) => {
    if (msg.type === "STATE_FULL_SYNC") {
      renderer.applyFullState(msg);
    } else if (msg.type === "STATE_DELTA") {
      renderer.applyDelta(msg);
    }
  };

  bridge.onCommand = (msg) => {
    // Route to appropriate module
    const type = msg.type;

    if (type.startsWith("CMD_CAMERA_")) {
      camera.handleCommand(msg);
    } else {
      // Entity/route commands go to renderer
      renderer.handleCommand(msg);
    }
  };

  // 6. Connect to Qt
  const connected = await bridge.connect();

  if (connected) {
    console.log("[main-qt] Connected to Qt — running in embedded mode");
  } else {
    console.log("[main-qt] Qt not available — running standalone preview");
    // In standalone mode, just show the empty Cesium globe
    // Useful for testing the web side without Qt
  }

  console.log("[main-qt] Initialization complete");
}

// Start
main().catch((err) => {
  console.error("[main-qt] Fatal error:", err);
});
