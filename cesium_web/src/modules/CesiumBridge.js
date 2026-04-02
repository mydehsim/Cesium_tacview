/**
 * CesiumBridge.js — QWebChannel bridge between Qt (C++) and CesiumJS
 *
 * This module handles:
 * - Connecting to Qt via QWebChannel
 * - Receiving state updates and commands from Qt
 * - Sending user interaction events back to Qt
 *
 * Qt is the authoritative state owner. This bridge is the single
 * communication layer — no other module should talk to Qt directly.
 */

export class CesiumBridge {
  constructor() {
    this.qtBridge = null;
    this.connected = false;
    this.onStateUpdate = null; // callback: (stateMsg) => void
    this.onCommand = null; // callback: (cmdMsg) => void
    this.lastTick = 0;
  }

  /**
   * Connect to Qt via QWebChannel.
   * Returns a promise that resolves when connection is established.
   */
  async connect() {
    return new Promise((resolve, reject) => {
      // Check if QWebChannel is available (injected by Qt)
      if (typeof QWebChannel === "undefined") {
        console.warn(
          "[CesiumBridge] QWebChannel not available — running standalone mode"
        );
        this.connected = false;
        resolve(false);
        return;
      }

      new QWebChannel(qt.webChannelTransport, (channel) => {
        this.qtBridge = channel.objects.qtBridge;
        if (!this.qtBridge) {
          console.error("[CesiumBridge] qtBridge object not found in channel");
          reject(new Error("qtBridge not found"));
          return;
        }

        this.connected = true;

        // Listen for state/command messages from Qt
        this.qtBridge.sendToCesium.connect((jsonStr) => {
          try {
            const msg = JSON.parse(jsonStr);
            this._handleMessage(msg);
          } catch (e) {
            console.error("[CesiumBridge] Failed to parse message:", e);
          }
        });

        console.log("[CesiumBridge] Connected to Qt via QWebChannel");

        // Notify Qt that Cesium is ready
        this.qtBridge.onCesiumReady();

        resolve(true);
      });
    });
  }

  /**
   * Send an event to Qt (user interaction)
   */
  sendEvent(type, payload = {}) {
    if (!this.connected || !this.qtBridge) {
      console.log("[CesiumBridge] Not connected, event dropped:", type);
      return;
    }

    const event = {
      type: type,
      payload: payload,
      timestamp: Date.now(),
    };

    this.qtBridge.onCesiumEvent(JSON.stringify(event));
  }

  /**
   * Request full state sync from Qt
   */
  requestFullSync() {
    this.sendEvent("REQ_FULL_SYNC");
  }

  /**
   * Handle incoming message from Qt
   */
  _handleMessage(msg) {
    const type = msg.type;

    if (type === "STATE_FULL_SYNC" || type === "STATE_DELTA") {
      const tick = msg.tick || 0;

      // Detect missed ticks — request full sync if gap > 10
      if (type === "STATE_DELTA" && tick > this.lastTick + 10) {
        console.warn(
          `[CesiumBridge] Tick gap detected: ${this.lastTick} → ${tick}, requesting full sync`
        );
        this.requestFullSync();
      }

      this.lastTick = tick;

      if (this.onStateUpdate) {
        this.onStateUpdate(msg);
      }
    } else if (type.startsWith("CMD_")) {
      if (this.onCommand) {
        this.onCommand(msg);
      }
    }
  }

  /**
   * Check if running in Qt embedded mode
   */
  isEmbedded() {
    return this.connected;
  }
}

export default CesiumBridge;
