/**
 * TCP Client Module
 * WebSocket (veya TCP Socket emülasyonu) üzerinden bağlantı yönetimi
 * TrackInjector TcpConnection'a benzer mimari
 */

import TcpConfig from "./TcpConfig.js";
import XmlBuilder from "./XmlBuilder.js";

export class TcpClient {
  constructor() {
    this.socket = null;
    this.isConnected = false;
    this.config = TcpConfig;
    this.reconnectAttempts = 0;
    this.maxReconnectAttempts = 5;
    this.reconnectDelay = 3000; // 3 saniye

    // Event listeners
    this.listeners = {
      statusChanged: [],
      messageReceived: [],
      connectionFailed: [],
      disconnected: [],
    };

    this.log(`[TcpClient] Initialized with ${this.config.getEndpointUrl()}`);
  }

  /**
   * Olaya listener ekle
   */
  on(event, callback) {
    if (this.listeners[event]) {
      this.listeners[event].push(callback);
    }
  }

  /**
   * Olay tetikle
   */
  emit(event, data) {
    if (this.listeners[event]) {
      this.listeners[event].forEach((callback) => callback(data));
    }
  }

  /**
   * TCP/WebSocket'e bağlan (Auto-connect)
   */
  async connect(host, port) {
    // Parametreler varsa config'i güncelle
    if (host && port) {
      this.config.setHostPort(host, port);
    }

    if (this.socket && this.isConnected) {
      this.log("[TcpClient] Already connected");
      return;
    }

    this.log(
      `[TcpClient] Connecting to ${this.config.getHost()}:${this.config.getPort()}...`
    );
    this.emit("statusChanged", {
      status: "CONNECTING",
      message: `Bağlanıyor: ${this.config.getHost()}:${this.config.getPort()}`,
    });

    try {
      // WebSocket denemesi
      const url = `ws://${this.config.getHost()}:${this.config.getPort()}`;

      // Browser ortamında WebSocket kullan
      if (typeof WebSocket !== "undefined") {
        this.socket = new WebSocket(url);

        this.socket.onopen = () => this.onConnected();
        this.socket.onmessage = (event) => this.onMessageReceived(event);
        this.socket.onerror = (error) => this.onConnectionError(error);
        this.socket.onclose = () => this.onDisconnected();
      } else {
        // Node.js ortamında veya WebSocket yoksa fallback
        this.log(
          "[TcpClient] WebSocket not available, using fallback TCP emulation"
        );
        this.setupFallbackConnection();
      }

      // Timeout ayarla
      this.connectionTimeout = setTimeout(() => {
        if (!this.isConnected) {
          this.log("[TcpClient] Connection timeout");
          this.disconnect();
          this.emit("connectionFailed", {
            error: "Connection timeout",
            timestamp: new Date(),
          });
          this.scheduleReconnect();
        }
      }, 5000);
    } catch (error) {
      this.log(`[TcpClient] Connect error: ${error.message}`);
      this.emit("connectionFailed", { error: error.message });
      this.scheduleReconnect();
    }
  }

  /**
   * Fallback TCP emülasyonu
   */
  setupFallbackConnection() {
    // Local storage veya IndexedDB kullanarak mesaj kuyruğu
    this.messageQueue = [];
    this.isConnected = true;
    this.reconnectAttempts = 0;
    this.log("[TcpClient] Fallback connection established");
    this.emit("statusChanged", {
      status: "CONNECTED_FALLBACK",
      message: "Bağlandı (Local Mode)",
    });
  }

  /**
   * Bağlantı başarılı
   */
  onConnected() {
    clearTimeout(this.connectionTimeout);
    this.isConnected = true;
    this.reconnectAttempts = 0;

    this.log("[TcpClient] Connected successfully");
    this.emit("statusChanged", {
      status: "CONNECTED",
      message: `Bağlandı: ${this.config.getHost()}:${this.config.getPort()}`,
    });
  }

  /**
   * Mesaj alındı
   */
  onMessageReceived(event) {
    try {
      const message = event.data;
      this.log(`[TcpClient] Message received: ${message.substring(0, 100)}...`);
      this.emit("messageReceived", { data: message, timestamp: new Date() });
    } catch (error) {
      this.log(`[TcpClient] Message parse error: ${error.message}`);
    }
  }

  /**
   * Bağlantı hatası
   */
  onConnectionError(error) {
    this.log(`[TcpClient] Connection error: ${error}`);
    this.emit("connectionFailed", {
      error: error.message || "Unknown error",
      timestamp: new Date(),
    });
  }

  /**
   * Bağlantı koptu
   */
  onDisconnected() {
    this.isConnected = false;
    this.socket = null;

    this.log("[TcpClient] Disconnected");
    this.emit("statusChanged", { status: "DISCONNECTED", message: "Bağlantı koptu" });
    this.emit("disconnected", { timestamp: new Date() });

    // Otomatik yeniden bağlanma
    this.scheduleReconnect();
  }

  /**
   * Yeniden bağlanmayı planla
   */
  scheduleReconnect() {
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      const delay = this.reconnectDelay * this.reconnectAttempts;

      this.log(
        `[TcpClient] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`
      );

      setTimeout(() => {
        if (!this.isConnected) {
          this.connect();
        }
      }, delay);
    } else {
      this.log("[TcpClient] Max reconnection attempts reached");
      this.emit("statusChanged", {
        status: "FAILED",
        message: "Maksimum bağlantı denemesi aşıldı",
      });
    }
  }

  /**
   * Veri gönder (Track XML)
   */
  async send(data) {
    if (!this.isConnected) {
      this.log("[TcpClient] Not connected, cannot send");
      throw new Error("TCP bağlantısı kopuk");
    }

    try {
      if (this.socket && this.socket.readyState === WebSocket.OPEN) {
        this.socket.send(data);
        this.log(`[TcpClient] Sent ${data.length} bytes`);
      } else if (this.messageQueue) {
        // Fallback mode
        this.messageQueue.push(data);
        this.log(`[TcpClient] Queued message (fallback mode)`);
      }

      return true;
    } catch (error) {
      this.log(`[TcpClient] Send error: ${error.message}`);
      throw error;
    }
  }

  /**
   * Track'i XML olarak hazırla ve gönder
   */
  async sendTracks(tracks) {
    const xml = XmlBuilder.buildTracksXml(tracks);
    return this.send(xml);
  }

  /**
   * Reset komutu gönder (ekranı temizle)
   */
  async reset() {
    const resetXml = XmlBuilder.buildResetXml();
    this.log("[TcpClient] Sending RESET command");
    return this.send(resetXml);
  }

  /**
   * Bağlantıyı kapat
   */
  disconnect() {
    if (this.socket) {
      this.socket.close();
    }

    if (this.connectionTimeout) {
      clearTimeout(this.connectionTimeout);
    }

    this.isConnected = false;
    this.socket = null;

    this.log("[TcpClient] Disconnected");
  }

  /**
   * Basit log
   */
  log(message) {
    console.log(message);
  }
}

export default TcpClient;
