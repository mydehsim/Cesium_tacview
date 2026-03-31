/**
 * Control Panel Module
 * Açılır/kapanır panel - TCP ayarları, Track yönetimi, Send/Reset butonları
 */

import TcpConfig from "../tcp/TcpConfig.js";
import TcpClient from "../tcp/TcpClient.js";

export class ControlPanel {
  constructor(tcpClient) {
    this.tcpClient = tcpClient || new TcpClient();
    this.config = TcpConfig;
    this.isOpen = false;
    this.tracks = [];
    this.currentTab = "connection"; // connection, tracks, settings

    this.createPanel();
    this.setupEventListeners();
    this.setupTcpListeners();
  }

  /**
   * Ana panel DOM'unu oluştur
   */
  createPanel() {
    console.log("[ControlPanel] Creating panel DOM...");
    
    // Panel container - SOL TARAFA KONUMLANDIRıL
    const panel = document.createElement("div");
    panel.id = "tcp-control-panel";
    panel.className = "tcp-panel-left";

    console.log("[ControlPanel] Panel created:", panel);

    // Panel başlık barı (kapatma butonu ile)
    const headerDiv = document.createElement("div");
    headerDiv.className = "tcp-panel-header";

    // Toggle/Kapatma butonu
    const toggleBtn = document.createElement("button");
    toggleBtn.id = "tcp-toggle-btn";
    toggleBtn.className = "tcp-toggle-btn";
    toggleBtn.innerHTML = "✕";
    toggleBtn.title = "Panel'ı kapat";

    // Title
    const title = document.createElement("h3");
    title.textContent = "TCP Panel";
    title.className = "tcp-panel-title";

    headerDiv.appendChild(title);
    headerDiv.appendChild(toggleBtn);

    // ============= TAB NAVIGATION =============
    const tabNavDiv = document.createElement("div");
    tabNavDiv.className = "tcp-tab-nav";

    // Tab 1: Connection
    const connTabBtn = document.createElement("button");
    connTabBtn.className = "tcp-tab-btn active";
    connTabBtn.id = "tcp-tab-connection";
    connTabBtn.innerHTML = "📡 Bağlantı";
    connTabBtn.dataset.tab = "connection";

    // Tab 2: Tracks
    const tracksTabBtn = document.createElement("button");
    tracksTabBtn.className = "tcp-tab-btn";
    tracksTabBtn.id = "tcp-tab-tracks";
    tracksTabBtn.innerHTML = "📍 Track'ler";
    tracksTabBtn.dataset.tab = "tracks";

    // Tab 3: Settings
    const settingsTabBtn = document.createElement("button");
    settingsTabBtn.className = "tcp-tab-btn";
    settingsTabBtn.id = "tcp-tab-settings";
    settingsTabBtn.innerHTML = "⚙️ Ayarlar";
    settingsTabBtn.dataset.tab = "settings";

    tabNavDiv.appendChild(connTabBtn);
    tabNavDiv.appendChild(tracksTabBtn);
    tabNavDiv.appendChild(settingsTabBtn);

    // ============= CONTENT AREA =============
    const contentArea = document.createElement("div");
    contentArea.className = "tcp-content-area";

    // ============= TAB 1: CONNECTION =============
    const connectionTabContent = document.createElement("div");
    connectionTabContent.id = "tcp-tab-connection-content";
    connectionTabContent.className = "tcp-tab-content active";

    // Connection status
    const statusDiv = document.createElement("div");
    statusDiv.className = "tcp-status-section";

    const statusLabel = document.createElement("label");
    statusLabel.textContent = "Durum:";

    const statusValue = document.createElement("span");
    statusValue.id = "tcp-status-value";
    statusValue.className = "status-disconnected";
    statusValue.textContent = "Bağlı değil";

    statusDiv.appendChild(statusLabel);
    statusDiv.appendChild(statusValue);

    // Connection settings
    const settingsDiv = document.createElement("div");
    settingsDiv.className = "tcp-settings-section";

    const hostLabel = document.createElement("label");
    hostLabel.textContent = "Host:";
    const hostInput = document.createElement("input");
    hostInput.id = "tcp-host-input";
    hostInput.type = "text";
    hostInput.placeholder = "127.0.0.1";
    hostInput.value = this.config.getHost();

    const portLabel = document.createElement("label");
    portLabel.textContent = "Port:";
    const portInput = document.createElement("input");
    portInput.id = "tcp-port-input";
    portInput.type = "number";
    portInput.placeholder = "9001";
    portInput.value = this.config.getPort();

    settingsDiv.appendChild(hostLabel);
    settingsDiv.appendChild(hostInput);
    settingsDiv.appendChild(portLabel);
    settingsDiv.appendChild(portInput);

    // Buttons
    const buttonsDiv = document.createElement("div");
    buttonsDiv.className = "tcp-buttons-section";

    const connectBtn = document.createElement("button");
    connectBtn.id = "tcp-connect-btn";
    connectBtn.className = "tcp-action-btn";
    connectBtn.textContent = "🔗 Bağlan";

    const disconnectBtn = document.createElement("button");
    disconnectBtn.id = "tcp-disconnect-btn";
    disconnectBtn.className = "tcp-action-btn";
    disconnectBtn.textContent = "🔌 Bağlantıyı Kes";

    buttonsDiv.appendChild(connectBtn);
    buttonsDiv.appendChild(disconnectBtn);

    // Log area
    const logDiv = document.createElement("div");
    logDiv.className = "tcp-log-section";

    const logLabel = document.createElement("label");
    logLabel.textContent = "Günlük:";

    const logArea = document.createElement("textarea");
    logArea.id = "tcp-log-area";
    logArea.className = "tcp-log-area";
    logArea.readOnly = true;
    logArea.rows = 6;

    logDiv.appendChild(logLabel);
    logDiv.appendChild(logArea);

    connectionTabContent.appendChild(statusDiv);
    connectionTabContent.appendChild(settingsDiv);
    connectionTabContent.appendChild(buttonsDiv);
    connectionTabContent.appendChild(logDiv);

    // ============= TAB 2: TRACKS =============
    const tracksTabContent = document.createElement("div");
    tracksTabContent.id = "tcp-tab-tracks-content";
    tracksTabContent.className = "tcp-tab-content";

    // Track list
    const trackListDiv = document.createElement("div");
    trackListDiv.className = "tcp-track-list-section";

    const trackLabel = document.createElement("label");
    trackLabel.textContent = "Yüklü Track'ler:";

    const trackList = document.createElement("ul");
    trackList.id = "tcp-track-list";
    trackList.className = "tcp-track-list";

    trackListDiv.appendChild(trackLabel);
    trackListDiv.appendChild(trackList);

    // Action buttons
    const trackButtonsDiv = document.createElement("div");
    trackButtonsDiv.className = "tcp-track-buttons-section";

    const sendBtn = document.createElement("button");
    sendBtn.id = "tcp-send-btn";
    sendBtn.className = "tcp-action-btn tcp-send-btn";
    sendBtn.textContent = "📤 Track'leri Gönder";
    sendBtn.disabled = true;

    const resetBtn = document.createElement("button");
    resetBtn.id = "tcp-reset-btn";
    resetBtn.className = "tcp-action-btn tcp-reset-btn";
    resetBtn.textContent = "🗑️ Ekranı Temizle";
    resetBtn.disabled = true;

    trackButtonsDiv.appendChild(sendBtn);
    trackButtonsDiv.appendChild(resetBtn);

    tracksTabContent.appendChild(trackListDiv);
    tracksTabContent.appendChild(trackButtonsDiv);

    // ============= TAB 3: SETTINGS =============
    const settingsTabContent = document.createElement("div");
    settingsTabContent.id = "tcp-tab-settings-content";
    settingsTabContent.className = "tcp-tab-content";

    const settingsContent = document.createElement("div");
    settingsContent.innerHTML = `
      <div class="tcp-settings-group">
        <label>📝 Ayarlar</label>
        <p>Bağlantı ve Track ayarlarını buradan yapılandırabilirsiniz.</p>
        
        <label style="margin-top: 12px;">🔄 Otomatik Bağlanma:</label>
        <div class="tcp-checkbox-group">
          <input type="checkbox" id="tcp-auto-connect" checked>
          <span>Sayfa yükleme sırasında otomatik bağlan</span>
        </div>
        
        <label style="margin-top: 12px;">🔄 Yeniden Bağlanma:</label>
        <div class="tcp-checkbox-group">
          <input type="checkbox" id="tcp-auto-reconnect" checked>
          <span>Bağlantı kesilirse otomatik yeniden bağlan</span>
        </div>
        
        <label style="margin-top: 12px;">📊 Bilgi Boyutu:</label>
        <input type="number" id="tcp-max-log-lines" value="100" min="10" max="500" style="width: 100%; padding: 8px; margin-top: 6px;">
        
        <button class="tcp-action-btn" style="margin-top: 16px; width: 100%;">💾 Kaydet</button>
      </div>
    `;

    settingsTabContent.appendChild(settingsContent);

    // ============= CONTENT AREA ASSEMBLY =============
    contentArea.appendChild(connectionTabContent);
    contentArea.appendChild(tracksTabContent);
    contentArea.appendChild(settingsTabContent);

    // Panel'e hepsini ekle
    panel.appendChild(headerDiv);
    panel.appendChild(tabNavDiv);
    panel.appendChild(contentArea);

    document.body.appendChild(panel);

    console.log("[ControlPanel] Panel appended to body. Panel ID:", panel.id);
    console.log("[ControlPanel] DOM check - panel in body:", document.getElementById("tcp-control-panel"));

    // Referansları sakla
    this.panel = panel;
    this.toggleBtn = toggleBtn;
    this.hostInput = hostInput;
    this.portInput = portInput;
    this.connectBtn = connectBtn;
    this.disconnectBtn = disconnectBtn;
    this.sendBtn = sendBtn;
    this.resetBtn = resetBtn;
    this.statusValue = statusValue;
    this.trackList = trackList;
    this.logArea = logArea;
    
    // Tab referansları
    this.connTabBtn = connTabBtn;
    this.tracksTabBtn = tracksTabBtn;
    this.settingsTabBtn = settingsTabBtn;
    this.connectionTabContent = connectionTabContent;
    this.tracksTabContent = tracksTabContent;
    this.settingsTabContent = settingsTabContent;
  }

  /**
   * Tab değiştir
   */
  switchTab(tabName) {
    this.currentTab = tabName;

    // Tüm tabs'ı deaktif et
    document.querySelectorAll(".tcp-tab-btn").forEach(btn => {
      btn.classList.remove("active");
    });

    // Tüm content'leri gizle
    document.querySelectorAll(".tcp-tab-content").forEach(content => {
      content.classList.remove("active");
    });

    // Seçili tab'ı aktif et
    const activeBtn = document.querySelector(`[data-tab="${tabName}"]`);
    const activeContent = document.getElementById(`tcp-tab-${tabName}-content`);

    if (activeBtn) activeBtn.classList.add("active");
    if (activeContent) activeContent.classList.add("active");
  }

  /**
   * Event listener'ları kur
   */
  setupEventListeners() {
    console.log("[ControlPanel] Setting up event listeners...");
    console.log("[ControlPanel] toggleBtn:", this.toggleBtn);
    
    // Panel aç/kapat
    this.toggleBtn.addEventListener("click", () => {
      console.log("[ControlPanel] Toggle button clicked!");
      this.togglePanel();
    });

    // Tab buttons
    this.connTabBtn.addEventListener("click", () => this.switchTab("connection"));
    this.tracksTabBtn.addEventListener("click", () => this.switchTab("tracks"));
    this.settingsTabBtn.addEventListener("click", () => this.switchTab("settings"));

    // Bağlantı butonları
    this.connectBtn.addEventListener("click", () => this.handleConnect());
    this.disconnectBtn.addEventListener("click", () => this.handleDisconnect());

    // Action butonları
    this.sendBtn.addEventListener("click", () => this.handleSend());
    this.resetBtn.addEventListener("click", () => this.handleReset());
  }

  /**
   * TCP Client event'lerini dinle
   */
  setupTcpListeners() {
    this.tcpClient.on("statusChanged", (data) => {
      this.updateStatus(data.status, data.message);
    });

    this.tcpClient.on("messageReceived", (data) => {
      this.addLog(`Mesaj alındı: ${data.data.substring(0, 50)}...`);
    });

    this.tcpClient.on("connectionFailed", (data) => {
      this.addLog(`❌ Bağlantı hatası: ${data.error}`);
    });

    this.tcpClient.on("disconnected", () => {
      this.addLog("🔌 Bağlantı kesildi");
    });
  }

  /**
   * Panel aç/kapat
   */
  togglePanel() {
    console.log("[ControlPanel] togglePanel called. Current state:", this.isOpen);
    this.isOpen = !this.isOpen;
    console.log("[ControlPanel] New state:", this.isOpen);
    
    this.panel.classList.toggle("tcp-panel-open");
    console.log("[ControlPanel] Panel classes after toggle:", this.panel.className);
    
    document.body.classList.toggle("tcp-panel-visible");
    console.log("[ControlPanel] Body classes:", document.body.className);
    
    this.toggleBtn.textContent = this.isOpen ? "✕" : "☰";
    console.log("[ControlPanel] Button text updated to:", this.toggleBtn.textContent);
  }

  /**
   * Bağlan butonu
   */
  handleConnect() {
    const host = this.hostInput.value || TcpConfig.DEFAULT_HOST;
    const port = parseInt(this.portInput.value) || TcpConfig.DEFAULT_PORT;

    this.addLog(`🔗 Bağlanıyor: ${host}:${port}`);
    this.tcpClient.connect(host, port);
  }

  /**
   * Bağlantıyı kes butonu
   */
  handleDisconnect() {
    this.addLog("🔌 Bağlantı kesiliyor...");
    this.tcpClient.disconnect();
    this.updateStatus("DISCONNECTED", "Bağlantı kesildi");
  }

  /**
   * Track'leri gönder butonu
   */
  async handleSend() {
    try {
      this.addLog("📤 Track'ler gönderiliyor...");
      await this.tcpClient.sendTracks(this.tracks);
      this.addLog("✅ Track'ler başarıyla gönderildi");
    } catch (error) {
      this.addLog(`❌ Gönderme hatası: ${error.message}`);
    }
  }

  /**
   * Ekranı temizle butonu
   */
  async handleReset() {
    try {
      this.addLog("🗑️ Reset komutu gönderiliyor...");
      await this.tcpClient.reset();
      this.addLog("✅ Reset başarılı");
    } catch (error) {
      this.addLog(`❌ Reset hatası: ${error.message}`);
    }
  }

  /**
   * Durumu güncelle
   */
  updateStatus(status, message = "") {
    const statusClasses = {
      CONNECTED: "status-connected",
      CONNECTING: "status-connecting",
      DISCONNECTED: "status-disconnected",
      FAILED: "status-failed",
      CONNECTED_FALLBACK: "status-fallback",
    };

    this.statusValue.className = `status-${status.toLowerCase()}`;
    this.statusValue.textContent = message || status;

    // Butonları etkinleştir/devre dışı bırak
    const isConnected =
      status === "CONNECTED" || status === "CONNECTED_FALLBACK";
    this.sendBtn.disabled = !isConnected;
    this.resetBtn.disabled = !isConnected;
    this.connectBtn.disabled = isConnected;
    this.disconnectBtn.disabled = !isConnected;

    this.addLog(
      `[${status}] ${message}`
    );
  }

  /**
   * Track listesini güncelle
   */
  updateTrackList(tracks) {
    this.tracks = tracks;
    this.trackList.innerHTML = "";

    tracks.forEach((track, index) => {
      const li = document.createElement("li");
      li.className = "track-item";
      li.innerHTML = `
        <strong>#${track.number || index}</strong> 
        ${track.callSign || "Unknown"} 
        <span class="track-type">${track.type || ""}</span>
        <span class="track-waypoints">${track.waypoints?.length || 0} WP</span>
      `;
      this.trackList.appendChild(li);
    });

    this.addLog(
      `📍 ${tracks.length} track yüklendi`
    );
  }

  /**
   * Günlüğe yazı ekle
   */
  addLog(message) {
    const timestamp = new Date().toLocaleTimeString("tr-TR");
    const logLine = `[${timestamp}] ${message}\n`;

    this.logArea.value += logLine;
    this.logArea.scrollTop = this.logArea.scrollHeight;

    console.log(logLine);
  }

  /**
   * Panel CSS stillerini ekle
   */
  static injectStyles() {
    if (document.getElementById("tcp-panel-styles")) return;

    const style = document.createElement("style");
    style.id = "tcp-panel-styles";
    style.textContent = `
      /* ============= TCP CONTROL PANEL - LEFT SIDE ============= */
      #tcp-control-panel {
        position: fixed;
        left: 0;
        top: 0;
        width: 380px;
        height: 100vh;
        z-index: 10000;
        font-family: 'Segoe UI', Arial, sans-serif;
        background: linear-gradient(135deg, rgba(20, 30, 48, 0.98) 0%, rgba(30, 40, 65, 0.98) 100%);
        backdrop-filter: blur(10px);
        color: #f5f5f5;
        transform: translateX(-100%);
        transition: transform 0.4s cubic-bezier(0.4, 0, 0.2, 1);
        border-right: 2px solid rgba(102, 126, 234, 0.3);
        box-shadow: 4px 0 20px rgba(0, 0, 0, 0.5);
        overflow-y: auto;
        overflow-x: hidden;
        display: flex;
        flex-direction: column;
      }

      /* Panel açıkken */
      #tcp-control-panel.tcp-panel-open {
        transform: translateX(0);
      }

      /* Cesium container'ı hareket ettir panel açıldığında */
      #tcp-control-panel.tcp-panel-open ~ #cesiumContainer {
        margin-left: 380px;
      }

      /* Panel Header */
      .tcp-panel-header {
        flex-shrink: 0;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        padding: 16px;
        border-bottom: 2px solid rgba(255, 255, 255, 0.2);
        display: flex;
        justify-content: space-between;
        align-items: center;
        z-index: 10001;
      }

      .tcp-panel-title {
        margin: 0;
        font-size: 16px;
        font-weight: bold;
        color: white;
      }

      .tcp-toggle-btn {
        background: rgba(255, 255, 255, 0.2);
        color: white;
        border: none;
        padding: 6px 12px;
        border-radius: 4px;
        cursor: pointer;
        font-size: 18px;
        font-weight: bold;
        transition: all 0.3s ease;
        display: flex;
        align-items: center;
        justify-content: center;
        width: 36px;
        height: 36px;
      }

      .tcp-toggle-btn:hover {
        background: rgba(255, 255, 255, 0.3);
        transform: scale(1.1);
      }

      .tcp-toggle-btn:active {
        transform: scale(0.95);
      }

      /* ============= TAB NAVIGATION ============= */
      .tcp-tab-nav {
        flex-shrink: 0;
        display: flex;
        gap: 0;
        padding: 0;
        margin: 0;
        background: rgba(0, 0, 0, 0.2);
        border-bottom: 2px solid rgba(102, 126, 234, 0.2);
      }

      .tcp-tab-btn {
        flex: 1;
        padding: 12px 8px;
        background: transparent;
        border: none;
        color: #a0a0ff;
        cursor: pointer;
        font-size: 12px;
        font-weight: bold;
        text-align: center;
        transition: all 0.3s ease;
        border-bottom: 3px solid transparent;
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
      }

      .tcp-tab-btn:hover {
        background: rgba(102, 126, 234, 0.1);
        color: #d0d0ff;
      }

      .tcp-tab-btn.active {
        color: white;
        border-bottom-color: #667eea;
        background: rgba(102, 126, 234, 0.1);
      }

      /* ============= TAB CONTENT AREA ============= */
      .tcp-tab-content-area {
        flex: 1;
        overflow-y: auto;
        padding: 0;
        position: relative;
      }

      .tcp-tab-content {
        display: none;
        padding: 16px;
        height: 100%;
        overflow-y: auto;
      }

      .tcp-tab-content.active {
        display: block;
      }

      /* Sections */
      .tcp-status-section,
      .tcp-settings-section,
      .tcp-buttons-section,
      .tcp-track-list-section,
      .tcp-log-section,
      .tcp-track-buttons-section {
        margin-bottom: 16px;
        padding-bottom: 16px;
        border-bottom: 1px solid rgba(255, 255, 255, 0.1);
      }

      .tcp-status-section:last-child,
      .tcp-log-section:last-child {
        border-bottom: none;
      }

      /* Labels ve Inputs */
      label {
        display: block;
        font-weight: bold;
        color: #a0a0ff;
        margin-bottom: 6px;
        font-size: 11px;
        text-transform: uppercase;
        letter-spacing: 0.5px;
      }

      input[type="text"],
      input[type="number"],
      input[type="checkbox"] {
        font-family: inherit;
      }

      input[type="text"],
      input[type="number"] {
        width: 100%;
        padding: 8px;
        margin-bottom: 8px;
        border: 1px solid rgba(102, 126, 234, 0.3);
        border-radius: 4px;
        font-size: 12px;
        box-sizing: border-box;
        background: rgba(255, 255, 255, 0.05);
        color: #f5f5f5;
      }

      input[type="text"]:focus,
      input[type="number"]:focus {
        outline: none;
        border-color: #667eea;
        background: rgba(102, 126, 234, 0.1);
        box-shadow: 0 0 8px rgba(102, 126, 234, 0.4);
      }

      /* Status Badge */
      #tcp-status-value {
        display: inline-block;
        padding: 6px 12px;
        border-radius: 4px;
        font-size: 12px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 1px;
      }

      .status-connected {
        background: #4caf50;
        color: white;
      }

      .status-connecting {
        background: #ff9800;
        color: white;
        animation: pulse 1s infinite;
      }

      @keyframes pulse {
        0%, 100% { opacity: 1; }
        50% { opacity: 0.7; }
      }

      .status-disconnected {
        background: #f44336;
        color: white;
      }

      .status-failed {
        background: #d32f2f;
        color: white;
      }

      .status-fallback {
        background: #2196f3;
        color: white;
      }

      /* Buttons */
      .tcp-action-btn {
        width: 100%;
        padding: 10px;
        margin-bottom: 8px;
        border: none;
        border-radius: 4px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        cursor: pointer;
        font-size: 12px;
        font-weight: bold;
        transition: all 0.2s ease;
      }

      .tcp-action-btn:hover:not(:disabled) {
        transform: translateY(-2px);
        box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
      }

      .tcp-action-btn:active:not(:disabled) {
        transform: translateY(0);
      }

      .tcp-action-btn:disabled {
        background: rgba(255, 255, 255, 0.1);
        cursor: not-allowed;
        opacity: 0.5;
      }

      .tcp-send-btn {
        background: linear-gradient(135deg, #4caf50 0%, #45a049 100%);
      }

      .tcp-send-btn:hover:not(:disabled) {
        box-shadow: 0 4px 12px rgba(76, 175, 80, 0.4);
      }

      .tcp-reset-btn {
        background: linear-gradient(135deg, #ff9800 0%, #f57c00 100%);
      }

      .tcp-reset-btn:hover:not(:disabled) {
        box-shadow: 0 4px 12px rgba(255, 152, 0, 0.4);
      }

      /* Track List */
      .tcp-track-list {
        list-style: none;
        padding: 0;
        margin: 0;
        max-height: 200px;
        overflow-y: auto;
        border: 1px solid rgba(102, 126, 234, 0.2);
        border-radius: 4px;
        background: rgba(0, 0, 0, 0.2);
      }

      .track-item {
        padding: 8px;
        border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        font-size: 11px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        flex-wrap: wrap;
        color: #a0d0ff;
      }

      .track-item:last-child {
        border-bottom: none;
      }

      .track-type {
        background: rgba(33, 150, 243, 0.2);
        color: #64b5f6;
        padding: 2px 6px;
        border-radius: 3px;
        font-size: 10px;
        margin: 0 2px;
      }

      .track-waypoints {
        background: rgba(156, 39, 176, 0.2);
        color: #ce93d8;
        padding: 2px 6px;
        border-radius: 3px;
        font-size: 10px;
      }

      /* Log Area */
      .tcp-log-area {
        width: 100%;
        padding: 8px;
        border: 1px solid rgba(102, 126, 234, 0.2);
        border-radius: 4px;
        font-family: 'Courier New', monospace;
        font-size: 10px;
        resize: none;
        background: rgba(0, 0, 0, 0.3);
        color: #00ff00;
        box-sizing: border-box;
      }

      /* Settings Group */
      .tcp-settings-group {
        padding: 12px;
        background: rgba(102, 126, 234, 0.1);
        border-radius: 6px;
        border-left: 3px solid #667eea;
      }

      .tcp-settings-group p {
        margin: 0 0 12px 0;
        font-size: 12px;
        color: #a0a0ff;
      }

      .tcp-checkbox-group {
        display: flex;
        align-items: center;
        gap: 8px;
        margin-bottom: 8px;
      }

      .tcp-checkbox-group input[type="checkbox"] {
        width: 16px;
        height: 16px;
        cursor: pointer;
      }

      .tcp-checkbox-group span {
        font-size: 12px;
        color: #f5f5f5;
      }

      /* Scrollbar Styling */
      .tcp-tab-content::-webkit-scrollbar,
      .tcp-track-list::-webkit-scrollbar {
        width: 6px;
      }

      .tcp-tab-content::-webkit-scrollbar-track,
      .tcp-track-list::-webkit-scrollbar-track {
        background: rgba(255, 255, 255, 0.05);
      }

      .tcp-tab-content::-webkit-scrollbar-thumb,
      .tcp-track-list::-webkit-scrollbar-thumb {
        background: rgba(102, 126, 234, 0.3);
        border-radius: 3px;
      }

      .tcp-tab-content::-webkit-scrollbar-thumb:hover,
      .tcp-track-list::-webkit-scrollbar-thumb:hover {
        background: rgba(102, 126, 234, 0.5);
      }

      /* Mobile */
      @media (max-width: 600px) {
        #tcp-control-panel {
          width: 100%;
          max-width: 100vw;
        }

        .tcp-tab-btn {
          font-size: 10px;
          padding: 10px 4px;
        }
      }
    `;

    document.head.appendChild(style);
  }
}

export default ControlPanel;
