/**
 * TCP Configuration Management
 * TrackInjector'daki TcpConfigService'e benzer - LocalStorage kullanır
 */

export class TcpConfig {
  static DEFAULT_HOST = "127.0.0.1";
  static DEFAULT_PORT = 9001;
  static STORAGE_KEY = "cesium_tcp_config";

  constructor() {
    this.config = this.load();
  }

  /**
   * LocalStorage'dan config yükle
   */
  load() {
    try {
      const stored = localStorage.getItem(TcpConfig.STORAGE_KEY);
      if (stored) {
        const parsed = JSON.parse(stored);
        return {
          host: parsed.host || TcpConfig.DEFAULT_HOST,
          port: parsed.port || TcpConfig.DEFAULT_PORT,
        };
      }
    } catch (error) {
      console.error("[TcpConfig] Load error:", error);
    }

    // Varsayılan config döndür
    return {
      host: TcpConfig.DEFAULT_HOST,
      port: TcpConfig.DEFAULT_PORT,
    };
  }

  /**
   * Config'i LocalStorage'a kaydet
   */
  save(config = this.config) {
    try {
      localStorage.setItem(TcpConfig.STORAGE_KEY, JSON.stringify(config));
      this.config = config;
      console.info("[TcpConfig] Saved:", config);
    } catch (error) {
      console.error("[TcpConfig] Save error:", error);
    }
  }

  /**
   * Host ve Port al
   */
  getHost() {
    return this.config.host;
  }

  getPort() {
    return this.config.port;
  }

  /**
   * Host ve Port ayarla
   */
  setHostPort(host, port) {
    this.config.host = host;
    this.config.port = port;
    this.save();
  }

  /**
   * Endpoint URL'sini oluştur
   */
  getEndpointUrl() {
    return `ws://${this.config.host}:${this.config.port}`;
  }
}

export default new TcpConfig();
