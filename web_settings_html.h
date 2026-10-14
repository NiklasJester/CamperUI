#ifndef WEB_SETTINGS_HTML_H
#define WEB_SETTINGS_HTML_H

#include <Arduino.h>

static const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>CamperUI - Einstellungen</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: #0b0e14;
      color: #f1f5f9;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      padding-bottom: 90px;
    }

    /* Header */
    header {
      background: #141822;
      border-bottom: 1px solid #232c3d;
      padding: 12px 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: sticky;
      top: 0;
      z-index: 100;
      backdrop-filter: blur(8px);
    }
    .header-left {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    h1 {
      font-size: 18px;
      font-weight: 700;
      color: #f8fafc;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .badge {
      font-size: 11px;
      font-weight: 600;
      padding: 2px 8px;
      border-radius: 12px;
      background: rgba(14, 165, 233, 0.15);
      color: #38bdf8;
      border: 1px solid rgba(14, 165, 233, 0.3);
    }
    .nav-links {
      display: flex;
      gap: 6px;
    }
    .nav-btn {
      background: #1e2430;
      color: #cbd5e1;
      text-decoration: none;
      font-size: 12px;
      font-weight: 600;
      padding: 7px 11px;
      border-radius: 8px;
      border: 1px solid #2c3649;
      display: inline-flex;
      align-items: center;
      gap: 4px;
      transition: all 0.2s;
    }
    .nav-btn:hover, .nav-btn.active {
      background: #0284c7;
      color: #fff;
      border-color: #38bdf8;
    }

    /* Horizontal Category Scroll (Mobile-First) */
    .cat-scroll-wrap {
      background: #10141d;
      border-bottom: 1px solid #1f2736;
      padding: 8px 12px;
      position: sticky;
      top: 53px;
      z-index: 90;
      overflow-x: auto;
      white-space: nowrap;
      -webkit-overflow-scrolling: touch;
      scrollbar-width: none;
    }
    .cat-scroll-wrap::-webkit-scrollbar { display: none; }
    .cat-pills {
      display: inline-flex;
      gap: 6px;
    }
    .cat-pill {
      background: #181f2b;
      color: #94a3b8;
      border: 1px solid #252f40;
      border-radius: 20px;
      padding: 6px 14px;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      gap: 6px;
      transition: all 0.15s;
    }
    .cat-pill:hover {
      background: #222b3c;
      color: #f1f5f9;
    }
    .cat-pill.active {
      background: #0ea5e9;
      color: #fff;
      border-color: #38bdf8;
      box-shadow: 0 2px 8px rgba(14, 165, 233, 0.35);
    }

    /* Container & Cards */
    .container {
      max-width: 800px;
      width: 100%;
      margin: 0 auto;
      padding: 14px 14px 0 14px;
    }
    .tab-section {
      display: none;
      animation: fadeIn 0.2s ease-in-out;
    }
    .tab-section.active {
      display: block;
    }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(4px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .card {
      background: #141822;
      border: 1px solid #232c3d;
      border-radius: 14px;
      padding: 16px;
      margin-bottom: 14px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.25);
    }
    .card-title {
      font-size: 15px;
      font-weight: 700;
      color: #38bdf8;
      margin-bottom: 14px;
      display: flex;
      align-items: center;
      gap: 8px;
      border-bottom: 1px solid #1f2736;
      padding-bottom: 8px;
    }

    /* Form Fields */
    .field {
      margin-bottom: 14px;
    }
    .field:last-child {
      margin-bottom: 0;
    }
    .label {
      display: block;
      font-size: 13px;
      font-weight: 600;
      color: #cbd5e1;
      margin-bottom: 6px;
    }
    .subtext {
      font-size: 11px;
      color: #64748b;
      margin-top: 4px;
      line-height: 1.4;
    }

    input[type="text"], input[type="password"], input[type="number"], select {
      width: 100%;
      background: #0a0d13;
      border: 1px solid #2a3445;
      border-radius: 9px;
      padding: 11px 12px;
      color: #f8fafc;
      font-size: 15px;
      outline: none;
      transition: all 0.2s;
      appearance: none;
      -webkit-appearance: none;
    }
    select {
      background-image: url("data:image/svg+xml;charset=UTF-8,%3csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%2394a3b8' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'%3e%3cpolyline points='6 9 12 15 18 9'%3e%3c/polyline%3e%3c/svg%3e");
      background-repeat: no-repeat;
      background-position: right 12px center;
      background-size: 16px;
      padding-right: 36px;
    }
    input:focus, select:focus {
      border-color: #0ea5e9;
      box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.2);
    }

    .input-row {
      display: flex;
      gap: 8px;
    }
    .btn-action {
      background: #1e2430;
      color: #38bdf8;
      border: 1px solid #2a3445;
      border-radius: 9px;
      padding: 0 14px;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      gap: 6px;
      white-space: nowrap;
      transition: all 0.2s;
    }
    .btn-action:hover {
      background: #283344;
      border-color: #38bdf8;
      color: #fff;
    }

    /* Grid & Two-Col */
    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    @media (max-width: 480px) {
      .grid-2 {
        grid-template-columns: 1fr;
      }
    }

    /* Row with Toggle */
    .toggle-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px 0;
      border-bottom: 1px solid #1a202c;
    }
    .toggle-row:last-child {
      border-bottom: none;
      padding-bottom: 0;
    }
    .toggle-row:first-child {
      padding-top: 0;
    }
    .toggle-info {
      flex: 1;
      padding-right: 12px;
    }
    .toggle-title {
      font-size: 14px;
      font-weight: 600;
      color: #f1f5f9;
    }
    .toggle-desc {
      font-size: 12px;
      color: #64748b;
      margin-top: 2px;
    }

    /* Modern Switch */
    .switch {
      position: relative;
      display: inline-block;
      width: 48px;
      height: 28px;
      flex-shrink: 0;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #232c3d;
      transition: .2s;
      border-radius: 28px;
      border: 1px solid #323e54;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 22px;
      width: 22px;
      left: 2px;
      bottom: 2px;
      background-color: #cbd5e1;
      transition: .2s;
      border-radius: 50%;
      box-shadow: 0 2px 4px rgba(0,0,0,0.3);
    }
    input:checked + .slider {
      background-color: #10b981;
      border-color: #34d399;
    }
    input:checked + .slider:before {
      transform: translateX(20px);
      background-color: #ffffff;
    }

    /* Range Slider */
    .range-wrap {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 6px;
    }
    input[type="range"] {
      flex: 1;
      height: 8px;
      border-radius: 4px;
      background: #1e2430;
      outline: none;
      -webkit-appearance: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: #0ea5e9;
      cursor: pointer;
      border: 2px solid #ffffff;
      box-shadow: 0 2px 6px rgba(0,0,0,0.4);
    }
    .range-val {
      font-size: 14px;
      font-weight: 700;
      color: #38bdf8;
      min-width: 44px;
      text-align: right;
    }

    /* Tab Reorder List */
    .reorder-list {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    .reorder-item {
      background: #0e1219;
      border: 1px solid #232c3d;
      border-radius: 10px;
      padding: 10px 12px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 8px;
    }
    .reorder-left {
      display: flex;
      align-items: center;
      gap: 10px;
      flex: 1;
    }
    .reorder-icon {
      font-size: 18px;
      width: 24px;
      text-align: center;
    }
    .reorder-name {
      font-size: 14px;
      font-weight: 600;
    }
    .reorder-btns {
      display: flex;
      gap: 4px;
    }
    .btn-arrow {
      background: #1a202c;
      color: #94a3b8;
      border: 1px solid #2c3649;
      border-radius: 6px;
      width: 32px;
      height: 32px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      font-size: 14px;
      transition: all 0.15s;
    }
    .btn-arrow:hover:not(:disabled) {
      background: #0ea5e9;
      color: #fff;
    }
    .btn-arrow:disabled {
      opacity: 0.3;
      cursor: not-allowed;
    }

    /* Sticky Bottom Action Bar */
    .bottom-bar {
      position: fixed;
      bottom: 0;
      left: 0;
      right: 0;
      background: rgba(16, 20, 29, 0.92);
      backdrop-filter: blur(12px);
      border-top: 1px solid #232c3d;
      padding: 12px 16px;
      z-index: 100;
      display: flex;
      justify-content: center;
    }
    .bottom-bar-inner {
      max-width: 800px;
      width: 100%;
      display: flex;
      gap: 10px;
    }
    .btn-save {
      flex: 1;
      background: #10b981;
      color: #fff;
      border: none;
      border-radius: 12px;
      padding: 14px;
      font-size: 16px;
      font-weight: 700;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      box-shadow: 0 4px 14px rgba(16, 185, 129, 0.4);
      transition: all 0.2s;
    }
    .btn-save:hover {
      background: #059669;
    }
    .btn-save.dirty {
      background: #0ea5e9;
      box-shadow: 0 4px 14px rgba(14, 165, 233, 0.5);
    }
    .btn-reboot {
      background: #222938;
      color: #f87171;
      border: 1px solid #374151;
      border-radius: 12px;
      padding: 14px 16px;
      font-size: 14px;
      font-weight: 600;
      cursor: pointer;
      display: flex;
      align-items: center;
      gap: 6px;
      white-space: nowrap;
      transition: all 0.2s;
    }
    .btn-reboot:hover {
      background: #7f1d1d;
      color: #fff;
    }

    /* Toast Notification */
    .toast {
      position: fixed;
      bottom: 84px;
      left: 50%;
      transform: translateX(-50%) translateY(20px);
      background: #10b981;
      color: white;
      padding: 10px 20px;
      border-radius: 30px;
      font-size: 14px;
      font-weight: 600;
      box-shadow: 0 8px 24px rgba(0,0,0,0.5);
      opacity: 0;
      visibility: hidden;
      transition: all 0.3s cubic-bezier(0.16, 1, 0.3, 1);
      z-index: 200;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .toast.show {
      opacity: 1;
      visibility: visible;
      transform: translateX(-50%) translateY(0);
    }
    .toast.error {
      background: #ef4444;
    }

    /* WiFi Scanner Modal */
    .modal {
      display: none;
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0,0,0,0.7);
      z-index: 300;
      align-items: center;
      justify-content: center;
      padding: 16px;
    }
    .modal.show {
      display: flex;
    }
    .modal-box {
      background: #141822;
      border: 1px solid #232c3d;
      border-radius: 16px;
      max-width: 420px;
      width: 100%;
      max-height: 80vh;
      overflow-y: auto;
      padding: 20px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.6);
    }
    .modal-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 14px;
    }
    .modal-head h3 { font-size: 16px; color: #f8fafc; }
    .btn-close {
      background: none; border: none; color: #94a3b8; font-size: 20px; cursor: pointer;
    }
    .wifi-list { display: flex; flex-direction: column; gap: 8px; }
    .wifi-item {
      background: #0e1219;
      border: 1px solid #232c3d;
      border-radius: 10px;
      padding: 10px 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      cursor: pointer;
      transition: all 0.15s;
    }
    .wifi-item:hover {
      background: #1e2430;
      border-color: #0ea5e9;
    }
    .wifi-name { font-weight: 600; font-size: 14px; }
    .wifi-rssi { font-size: 12px; color: #94a3b8; }
  </style>
</head>
<body>

  <!-- Top Header -->
  <header>
    <div class="header-left">
      <h1>🚐 CamperUI</h1>
      <span class="badge" id="badgeVer">Lade...</span>
    </div>
    <div class="nav-links">
      <a href="/settings" class="nav-btn active">⚙️ Einstellungen</a>
      <a href="/status" class="nav-btn">📟 Terminal</a>
      <a href="/update" class="nav-btn">⚡ Update</a>
    </div>
  </header>

  <!-- Category Nav Pills (Scrollable Mobile-First) -->
  <div class="cat-scroll-wrap">
    <div class="cat-pills">
      <div class="cat-pill active" onclick="showTab('net')">🌐 Netzwerk</div>
      <div class="cat-pill" onclick="showTab('display')">📺 Display</div>
      <div class="cat-pill" onclick="showTab('tabs')">📑 Menü & Tabs</div>
      <div class="cat-pill" onclick="showTab('water')">💧 Wassertanks</div>
      <div class="cat-pill" onclick="showTab('switches')">⚡ Schalter & Dimmer</div>
      <div class="cat-pill" onclick="showTab('sensors')">🌡️ Sensoren & Klima</div>
      <div class="cat-pill" onclick="showTab('home')">🏠 Home Kacheln</div>
    </div>
  </div>

  <div class="container">
    <form id="settingsForm" onsubmit="event.preventDefault(); saveSettings();">

      <!-- ================= TAB: NETZWERK ================= -->
      <div id="tab-net" class="tab-section active">
        <div class="card">
          <div class="card-title">🌐 WLAN & VanPi Gateway</div>
          <div class="field">
            <label class="label">WLAN-Name (SSID)</label>
            <div class="input-row">
              <input type="text" id="wifi_ssid" placeholder="WLAN Name eingeben">
              <button type="button" class="btn-action" onclick="scanWifi()">🔍 Scannen</button>
            </div>
          </div>
          <div class="field">
            <label class="label">WLAN-Passwort</label>
            <div class="input-row">
              <input type="password" id="wifi_pass" placeholder="Passwort eingeben">
              <button type="button" class="btn-action" onclick="togglePass()">👁️</button>
            </div>
          </div>
          <div class="field">
            <label class="label">VanPi Gateway IP</label>
            <input type="text" id="vanpi_ip" placeholder="192.168.1.xxx">
            <div class="subtext">IP des VanPi Systems für Port 1880 REST API</div>
          </div>
        </div>
      </div>

      <!-- ================= TAB: DISPLAY & HARDWARE ================= -->
      <div id="tab-display" class="tab-section">
        <div class="card">
          <div class="card-title">📺 Anzeige & Hardware</div>
          <div class="toggle-row">
            <div class="toggle-info">
              <div class="toggle-title">Dunkles Design (Dark Mode)</div>
              <div class="toggle-desc">Modernes OLED-Schwarz oder helles Tag-Design</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="dark_mode">
              <span class="slider"></span>
            </label>
          </div>
          <div class="toggle-row">
            <div class="toggle-info">
              <div class="toggle-title">Touch-Ton (Buzzer)</div>
              <div class="toggle-desc">Akustisches Feedback bei Fingertipp</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="buzzer_enabled">
              <span class="slider"></span>
            </label>
          </div>
          <div class="toggle-row">
            <div class="toggle-info">
              <div class="toggle-title">Demo / Simulation</div>
              <div class="toggle-desc">Generiert Testdaten ohne VanPi-Verbindung</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="debug_mode">
              <span class="slider"></span>
            </label>
          </div>
        </div>

        <div class="card">
          <div class="card-title">💡 Helligkeit & Standby</div>
          <div class="field">
            <label class="label">Display Helligkeit</label>
            <div class="range-wrap">
              <input type="range" id="display_brightness" min="10" max="100" step="5" oninput="document.getElementById('val_bright').innerText = this.value + '%'">
              <span class="range-val" id="val_bright">80%</span>
            </div>
          </div>
          <div class="field">
            <label class="label">Display-Standby / Timeout</label>
            <select id="display_timeout">
              <option value="0">Immer an (Kein Standby)</option>
              <option value="15">15 Sekunden</option>
              <option value="30">30 Sekunden</option>
              <option value="60">1 Minute</option>
              <option value="120">2 Minuten</option>
              <option value="300">5 Minuten</option>
            </select>
          </div>
          <div class="toggle-row" style="margin-top:12px;">
            <div class="toggle-info">
              <div class="toggle-title">Screensaver</div>
              <div class="toggle-desc">Zeigt Uhrzeit &amp; Restlaufzeit in Dunkelrot bei 1% Helligkeit nach Timeout</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="screensaver_enabled">
              <span class="slider"></span>
            </label>
          </div>
        </div>
      </div>

      <!-- ================= TAB: MENÜ & TABS ================= -->
      <div id="tab-tabs" class="tab-section">
        <div class="card">
          <div class="card-title">📑 Menüleiste anpassen</div>
          <div class="subtext" style="margin-bottom:12px;">Aktiviere oder deaktiviere Tabs und ändere die Reihenfolge in der unteren Navigationsleiste:<br><small style="color:#94a3b8; display:inline-block; margin-top:5px;">💡 Notfall-Zugriff: Wenn Settings ausgeblendet ist, halte 5 Sekunden lang irgendwo die untere Tableiste gedrückt.</small></div>
          <div class="reorder-list" id="tabReorderList">
            <!-- Dynamisch befüllt -->
          </div>
        </div>
      </div>

      <!-- ================= TAB: WASSER ================= -->
      <div id="tab-water" class="tab-section">
        <div class="card">
          <div class="card-title">💧 Wassertanks Konfiguration</div>
          <div class="field">
            <label class="label">Anzahl sichtbare Tanks</label>
            <select id="tank_count" onchange="updateTankInputs()">
              <option value="1">1 Tank</option>
              <option value="2">2 Tanks</option>
              <option value="3">3 Tanks</option>
              <option value="4">4 Tanks</option>
            </select>
          </div>
          <div class="toggle-row" style="margin-bottom:12px;">
            <div class="toggle-info">
              <div class="toggle-title">Einheit/Menge unter Prozent anzeigen</div>
              <div class="toggle-desc">Berechnet Restmenge (L / kg) anhand der Kapazität</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="tank_show_liters">
              <span class="slider"></span>
            </label>
          </div>
          <div class="grid-2">
            <div class="field" id="wrap_tank0">
              <label class="label">Kapazität &amp; Einheit Tank 1</label>
              <div style="display:flex; gap:8px;">
                <input type="number" id="tank_max_0" min="10" max="1000" style="flex:1;">
                <select id="tank_unit_0" style="width:75px;">
                  <option value="0">L</option>
                  <option value="1">kg</option>
                </select>
              </div>
            </div>
            <div class="field" id="wrap_tank1">
              <label class="label">Kapazität &amp; Einheit Tank 2</label>
              <div style="display:flex; gap:8px;">
                <input type="number" id="tank_max_1" min="10" max="1000" style="flex:1;">
                <select id="tank_unit_1" style="width:75px;">
                  <option value="0">L</option>
                  <option value="1">kg</option>
                </select>
              </div>
            </div>
            <div class="field" id="wrap_tank2">
              <label class="label">Kapazität &amp; Einheit Tank 3</label>
              <div style="display:flex; gap:8px;">
                <input type="number" id="tank_max_2" min="10" max="1000" style="flex:1;">
                <select id="tank_unit_2" style="width:75px;">
                  <option value="0">L</option>
                  <option value="1">kg</option>
                </select>
              </div>
            </div>
            <div class="field" id="wrap_tank3">
              <label class="label">Kapazität &amp; Einheit Tank 4</label>
              <div style="display:flex; gap:8px;">
                <input type="number" id="tank_max_3" min="10" max="1000" style="flex:1;">
                <select id="tank_unit_3" style="width:75px;">
                  <option value="0">L</option>
                  <option value="1">kg</option>
                </select>
              </div>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🚰 Pumpen- und Ablaufsteuerung</div>
          <div class="grid-2">
            <div class="field">
              <label class="label">Frischwasser-Pumpe Relais</label>
              <select id="pump_relay"></select>
            </div>
            <div class="field">
              <label class="label">Abwasserventil Relais</label>
              <select id="drain_relay"></select>
            </div>
          </div>
        </div>
      </div>

      <!-- ================= TAB: SCHALTER & DIMMER ================= -->
      <div id="tab-switches" class="tab-section">
        <div class="card">
          <div class="card-title">🔘 Relais Sichtbarkeit (8 Hauptrelais)</div>
          <div id="relaysList"></div>
        </div>

        <div class="card">
          <div class="card-title">💡 Dimmer Sichtbarkeit (8 Dimmer)</div>
          <div id="dimmersList"></div>
        </div>

        <div class="card">
          <div class="card-title">📶 WiFi-Relais (Zusatzrelais)</div>
          <div class="toggle-row" style="margin-bottom:12px;">
            <div class="toggle-info">
              <div class="toggle-title">WiFi-Relais aktivieren</div>
              <div class="toggle-desc">Blendet WLAN-Schalter ein</div>
            </div>
            <label class="switch">
              <input type="checkbox" id="show_wrelay" onchange="document.getElementById('wrelaysList').style.display = this.checked ? 'block' : 'none'">
              <span class="slider"></span>
            </label>
          </div>
          <div id="wrelaysList"></div>
        </div>
      </div>

      <!-- ================= TAB: SENSOREN & ALARME ================= -->
      <div id="tab-sensors" class="tab-section">
        <div class="card">
          <div class="card-title">🔥 Heizung &amp; Klima</div>
          <div class="field">
            <label class="label">Verbaute Standheizung</label>
            <select id="heater_type"></select>
          </div>
          <div class="subtext" style="margin-top:6px; color:#94a3b8; font-size:12px;">Wähle das verbaute Heizungssystem (z.B. Autoterm, Truma).</div>
        </div>

        <div class="card">
          <div class="card-title">🌡️ Sensor Zuordnung</div>
          <div class="field">
            <label class="label">Außentemperatur-Sensor</label>
            <select id="outdoor_temp_sensor"></select>
          </div>
          <div class="field">
            <label class="label">Innentemperatur-Sensor</label>
            <select id="indoor_temp_sensor"></select>
          </div>
          <div class="grid-2">
            <div class="field">
              <label class="label">Zusatz-Sensor 1</label>
              <select id="temps_custom_sensor_0"></select>
            </div>
            <div class="field">
              <label class="label">Zusatz-Sensor 2</label>
              <select id="temps_custom_sensor_1"></select>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🚨 Smart Header Alarmschwellen</div>
          <div class="grid-2">
            <div class="field">
              <label class="label">Frostgrenze Außen (&le; &deg;C)</label>
              <input type="number" step="0.5" id="warn_frost_temp">
            </div>
            <div class="field">
              <label class="label">Batterie Rot (&le; % SoC)</label>
              <input type="number" id="warn_bat_soc" min="0" max="100">
            </div>
            <div class="field">
              <label class="label">Batterie Gelb (&le; % SoC)</label>
              <input type="number" id="warn_bat_yellow_soc" min="0" max="100">
            </div>
            <div class="field">
              <label class="label">Frischwasser Alarm (&le; %)</label>
              <input type="number" id="warn_fresh_min" min="0" max="100">
            </div>
            <div class="field">
              <label class="label">Abwasser Alarm (&ge; %)</label>
              <input type="number" id="warn_waste_max" min="0" max="100">
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🔋 Batterie & Solar Limits</div>
          <div class="grid-2">
            <div class="field">
              <label class="label">Solar Maximum (W)</label>
              <input type="number" id="solar_max_w">
            </div>
            <div class="field">
              <label class="label">Batterie Kapazität (Ah)</label>
              <input type="number" id="bat_capacity_ah">
            </div>
          </div>
        </div>
      </div>

      <!-- ================= TAB: HOME KACHELN ================= -->
      <div id="tab-home" class="tab-section">
        <div class="card">
          <div class="card-title">🏠 Home-Screen Widgets</div>
          <div class="toggle-row">
            <div class="toggle-info"><div class="toggle-title">Batterie-Kachel</div></div>
            <label class="switch"><input type="checkbox" id="home_show_battery"><span class="slider"></span></label>
          </div>
          <div class="toggle-row">
            <div class="toggle-info"><div class="toggle-title">Starter-Batterie</div></div>
            <label class="switch"><input type="checkbox" id="home_show_starter"><span class="slider"></span></label>
          </div>
          <div class="toggle-row">
            <div class="toggle-info"><div class="toggle-title">Solar-Kachel</div></div>
            <label class="switch"><input type="checkbox" id="home_show_solar"><span class="slider"></span></label>
          </div>
          <div class="toggle-row">
            <div class="toggle-info"><div class="toggle-title">Wasser-Kachel</div></div>
            <label class="switch"><input type="checkbox" id="home_show_water"><span class="slider"></span></label>
          </div>
          <div class="field" style="margin-top:12px;">
            <label class="label">Wasser-Quelle auf Home</label>
            <select id="home_water_source">
              <option value="0">Tank 1 (Frischwasser)</option>
              <option value="1">Tank 2 (Grauwasser)</option>
              <option value="2">Tank 3</option>
              <option value="3">Tank 4</option>
            </select>
          </div>
          <div class="grid-2" style="margin-top:12px;">
            <div class="field">
              <label class="label">Favoriten-Schalter 1</label>
              <select id="home_favorite_0"></select>
            </div>
            <div class="field">
              <label class="label">Favoriten-Schalter 2</label>
              <select id="home_favorite_1"></select>
            </div>
          </div>
        </div>
      </div>

    </form>
  </div>

  <!-- Sticky Bottom Save Bar -->
  <div class="bottom-bar">
    <div class="bottom-bar-inner">
      <button type="button" class="btn-save" id="btnSave" onclick="saveSettings()">
        <span>💾</span> Einstellungen speichern
      </button>
      <button type="button" class="btn-reboot" onclick="rebootEsp()">
        <span>🔄</span> Neustart
      </button>
    </div>
  </div>

  <!-- Toast -->
  <div class="toast" id="toast">✅ Gespeichert!</div>

  <!-- WiFi Scanner Modal -->
  <div class="modal" id="wifiModal">
    <div class="modal-box">
      <div class="modal-head">
        <h3>Gefundene WLAN-Netzwerke</h3>
        <button type="button" class="btn-close" onclick="closeWifiModal()">&times;</button>
      </div>
      <div class="wifi-list" id="wifiList">Scanne...</div>
    </div>
  </div>

  <script>
    let currentSettings = {};
    let tabOrder = [];
    let tabEnabled = {};
    const tabMeta = [
      { id: 0, name: "Home", icon: "🏠" },
      { id: 1, name: "Dimmer", icon: "💡" },
      { id: 2, name: "Power", icon: "🔋" },
      { id: 3, name: "Wasser", icon: "💧" },
      { id: 4, name: "Klima", icon: "🔥" },
      { id: 5, name: "Temps", icon: "🌡️" },
      { id: 6, name: "MaxxFan", icon: "💨" },
      { id: 7, name: "Schalter", icon: "🔘" },
      { id: 8, name: "Level", icon: "⚖️" },
      { id: 9, name: "Einstellungen", icon: "⚙️" },
      { id: 10, name: "Truma", icon: "🔥" }
    ];

    function showTab(id) {
      document.querySelectorAll('.tab-section').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.cat-pill').forEach(el => el.classList.remove('active'));
      const sec = document.getElementById('tab-' + id);
      if (sec) sec.classList.add('active');
      event.currentTarget.classList.add('active');
      window.scrollTo({ top: 0, behavior: 'smooth' });
    }

    function togglePass() {
      const p = document.getElementById('wifi_pass');
      p.type = p.type === 'password' ? 'text' : 'password';
    }

    function showToast(msg, isError = false) {
      const t = document.getElementById('toast');
      t.innerText = msg;
      t.className = 'toast show' + (isError ? ' error' : '');
      setTimeout(() => { t.className = 'toast'; }, 3000);
    }

    function loadSettings() {
      fetch('/api/settings')
        .then(r => r.json())
        .then(data => {
          currentSettings = data;
          document.getElementById('badgeVer').innerText = data.version || 'V1.7.0';
          
          // Network
          document.getElementById('wifi_ssid').value = data.wifi_ssid || '';
          document.getElementById('wifi_pass').value = data.wifi_pass || '';
          document.getElementById('vanpi_ip').value = data.vanpi_ip || '';

          // Display
          document.getElementById('dark_mode').checked = !!data.dark_mode;
          document.getElementById('buzzer_enabled').checked = !!data.buzzer_enabled;
          document.getElementById('debug_mode').checked = !!data.debug_mode;
          document.getElementById('display_brightness').value = data.display_brightness || 80;
          document.getElementById('val_bright').innerText = (data.display_brightness || 80) + '%';
          document.getElementById('display_timeout').value = data.display_timeout !== undefined ? data.display_timeout : 60;
          document.getElementById('screensaver_enabled').checked = !!data.screensaver_enabled;

          // Water
          document.getElementById('tank_count').value = data.tank_count || 1;
          document.getElementById('tank_show_liters').checked = !!data.tank_show_liters;
          for (let i = 0; i < 4; i++) {
            const el = document.getElementById('tank_max_' + i);
            if (el) el.value = (data.tank_max && data.tank_max[i]) ? data.tank_max[i] : 100;
            const elU = document.getElementById('tank_unit_' + i);
            if (elU) elU.value = (data.tank_unit && data.tank_unit[i] !== undefined) ? data.tank_unit[i] : 0;
          }
          updateTankInputs();

          // Dropdowns: Relays
          populateRelaySelect('pump_relay', data.relays, data.pump_relay);
          populateRelaySelect('drain_relay', data.relays, data.drain_relay);

          // Heater
          populateHeaterTypes(data.heater_types, data.heater_type !== undefined ? data.heater_type : 0);

          // Relays & Dimmers lists
          buildRelaysList(data.switch_names, data.switch_visible);
          buildDimmersList(data.dimmer_names, data.dimmer_visible);
          document.getElementById('show_wrelay').checked = !!data.show_wrelay;
          document.getElementById('wrelaysList').style.display = data.show_wrelay ? 'block' : 'none';
          buildWRelaysList(data.wrelay_names, data.wrelay_visible);

          // Sensors
          populateTempSensorsSelect('outdoor_temp_sensor', data.temp_sensors, data.outdoor_temp_sensor, false);
          populateTempSensorsSelect('indoor_temp_sensor', data.temp_sensors, data.indoor_temp_sensor, false);
          populateTempSensorsSelect('temps_custom_sensor_0', data.temp_sensors, (data.temps_custom_sensor ? data.temps_custom_sensor[0] : -1), true);
          populateTempSensorsSelect('temps_custom_sensor_1', data.temp_sensors, (data.temps_custom_sensor ? data.temps_custom_sensor[1] : -1), true);

          // Alarms
          document.getElementById('warn_frost_temp').value = data.warn_frost_temp !== undefined ? data.warn_frost_temp : 3.0;
          document.getElementById('warn_bat_soc').value = data.warn_bat_soc !== undefined ? data.warn_bat_soc : 15;
          document.getElementById('warn_bat_yellow_soc').value = data.warn_bat_yellow_soc !== undefined ? data.warn_bat_yellow_soc : 30;
          document.getElementById('warn_fresh_min').value = data.warn_fresh_min !== undefined ? data.warn_fresh_min : 15;
          document.getElementById('warn_waste_max').value = data.warn_waste_max !== undefined ? data.warn_waste_max : 85;
          document.getElementById('solar_max_w').value = data.solar_max_w || 200;
          document.getElementById('bat_capacity_ah').value = data.bat_capacity_ah || 100;

          // Home Widgets
          document.getElementById('home_show_battery').checked = !!data.home_show_battery;
          document.getElementById('home_show_starter').checked = !!data.home_show_starter;
          document.getElementById('home_show_solar').checked = !!data.home_show_solar;
          document.getElementById('home_show_water').checked = !!data.home_show_water;
          document.getElementById('home_water_source').value = data.home_water_source || 0;
          populateFavoriteSelect('home_favorite_0', data.relays, data.dimmers, data.home_favorite ? data.home_favorite[0] : 0);
          populateFavoriteSelect('home_favorite_1', data.relays, data.dimmers, data.home_favorite ? data.home_favorite[1] : 0);

          // Tabs
          tabOrder = (data.tab_order && data.tab_order.length) ? data.tab_order.slice() : [0,1,2,3,4,5,6,7,8,9,10];
          tabMeta.forEach(m => {
            if (!tabOrder.includes(m.id)) tabOrder.push(m.id);
          });
          tabEnabled = {};
          if (data.tab_enabled) {
            for (let i = 0; i < data.tab_enabled.length; i++) {
              tabEnabled[i] = !!data.tab_enabled[i];
            }
          }
          renderTabReorderList();
        })
        .catch(err => {
          console.error(err);
          showToast("Fehler beim Laden der Einstellungen", true);
        });
    }

    function populateRelaySelect(elemId, relays, selectedVal) {
      const sel = document.getElementById(elemId);
      if (!sel) return;
      sel.innerHTML = '<option value="-1">Nicht verfügbar</option>';
      if (relays) {
        relays.forEach(r => {
          const opt = document.createElement('option');
          opt.value = r.id;
          opt.text = (r.is_wrelay ? '📶 ' : '🔘 ') + r.name;
          if (r.id === selectedVal) opt.selected = true;
          sel.appendChild(opt);
        });
      }
    }

    function populateDimmerSelect(elemId, dimmers, selectedVal) {
      const sel = document.getElementById(elemId);
      if (!sel) return;
      sel.innerHTML = '';
      if (dimmers) {
        dimmers.forEach(d => {
          const opt = document.createElement('option');
          opt.value = d.id;
          opt.text = '💡 ' + d.name;
          if (d.id === selectedVal) opt.selected = true;
          sel.appendChild(opt);
        });
      }
    }

    function populateTempSensorsSelect(elemId, sensors, selectedVal, includeNone) {
      const sel = document.getElementById(elemId);
      if (!sel) return;
      sel.innerHTML = includeNone ? '<option value="-1">Nicht anzeigen</option>' : '';
      if (sensors) {
        sensors.forEach(s => {
          const opt = document.createElement('option');
          opt.value = s.idx;
          opt.text = s.name + (s.val !== undefined ? ` (${s.val.toFixed(1)} °C)` : '');
          if (s.idx === selectedVal) opt.selected = true;
          sel.appendChild(opt);
        });
      }
    }

    function populateHeaterTypes(heaterTypes, selectedType) {
      const sel = document.getElementById('heater_type');
      if (!sel) return;
      sel.innerHTML = '';
      const list = heaterTypes || [
        { id: 0, name: 'Autoterm (Air 2D/4D)' },
        { id: 1, name: 'Truma (Gas/Diesel)' },
        { id: 2, name: 'Truma + Elektro (Combi E)' },
        { id: 3, name: 'Keine / Deaktiviert' }
      ];
      list.forEach(h => {
        const opt = document.createElement('option');
        opt.value = h.id;
        opt.text = h.name;
        if (h.id === selectedType) opt.selected = true;
        sel.appendChild(opt);
      });
      sel.onchange = function() {
        const val = parseInt(this.value);
        if (val === 0) { // Autoterm
          tabEnabled[4] = true;
          tabEnabled[10] = false;
        } else if (val === 1 || val === 2) { // Truma (pure or electric)
          tabEnabled[4] = false;
          tabEnabled[10] = true;
        } else { // None
          tabEnabled[4] = false;
          tabEnabled[10] = false;
        }
        renderTabReorderList();
      };
    }

    function populateFavoriteSelect(elemId, relays, dimmers, selectedVal) {
      const sel = document.getElementById(elemId);
      if (!sel) return;
      sel.innerHTML = '<option value="0">Aus (Nicht belegt)</option>';
      if (relays) {
        relays.forEach(r => {
          if (!r.is_wrelay) {
            const opt = document.createElement('option');
            opt.value = r.id + 1; // 1..8
            opt.text = '🔘 ' + r.name;
            if (opt.value == selectedVal) opt.selected = true;
            sel.appendChild(opt);
          }
        });
      }
      if (dimmers) {
        dimmers.forEach(d => {
          const opt = document.createElement('option');
          opt.value = d.id + 9; // 9..16
          opt.text = '💡 ' + d.name;
          if (opt.value == selectedVal) opt.selected = true;
          sel.appendChild(opt);
        });
      }
    }

    function updateTankInputs() {
      const cnt = parseInt(document.getElementById('tank_count').value) || 1;
      for (let i = 0; i < 4; i++) {
        const w = document.getElementById('wrap_tank' + i);
        if (w) w.style.display = (i < cnt) ? 'block' : 'none';
      }
    }

    function buildRelaysList(names, visible) {
      const wrap = document.getElementById('relaysList');
      wrap.innerHTML = '';
      for (let i = 0; i < 8; i++) {
        const name = (names && names[i]) ? names[i] : `Relais ${i + 1}`;
        const vis = visible ? !!visible[i] : true;
        const row = document.createElement('div');
        row.className = 'toggle-row';
        row.innerHTML = `
          <div class="toggle-info"><div class="toggle-title">🔘 ${name}</div></div>
          <label class="switch">
            <input type="checkbox" id="switch_vis_${i}" ${vis ? 'checked' : ''}>
            <span class="slider"></span>
          </label>
        `;
        wrap.appendChild(row);
      }
    }

    function buildDimmersList(names, visible) {
      const wrap = document.getElementById('dimmersList');
      wrap.innerHTML = '';
      for (let i = 0; i < 8; i++) {
        const name = (names && names[i]) ? names[i] : `Dimmer ${i + 1}`;
        const vis = visible ? !!visible[i] : true;
        const row = document.createElement('div');
        row.className = 'toggle-row';
        row.innerHTML = `
          <div class="toggle-info"><div class="toggle-title">💡 ${name}</div></div>
          <label class="switch">
            <input type="checkbox" id="dimmer_vis_${i}" ${vis ? 'checked' : ''}>
            <span class="slider"></span>
          </label>
        `;
        wrap.appendChild(row);
      }
    }

    function buildWRelaysList(names, visible) {
      const wrap = document.getElementById('wrelaysList');
      wrap.innerHTML = '';
      for (let i = 0; i < 8; i++) {
        const name = (names && names[i]) ? names[i] : `WLAN Relais ${i + 1}`;
        const vis = visible ? !!visible[i] : true;
        const row = document.createElement('div');
        row.className = 'toggle-row';
        row.innerHTML = `
          <div class="toggle-info"><div class="toggle-title">📶 ${name}</div></div>
          <label class="switch">
            <input type="checkbox" id="wrelay_vis_${i}" ${vis ? 'checked' : ''}>
            <span class="slider"></span>
          </label>
        `;
        wrap.appendChild(row);
      }
    }

    function renderTabReorderList() {
      const wrap = document.getElementById('tabReorderList');
      wrap.innerHTML = '';
      tabOrder.forEach((tabId, idx) => {
        if (tabId >= tabMeta.length) return;
        const meta = tabMeta[tabId];
        const isEn = tabEnabled[tabId] !== undefined ? tabEnabled[tabId] : true;
        const item = document.createElement('div');
        item.className = 'reorder-item';
        item.innerHTML = `
          <div class="reorder-left">
            <span class="reorder-icon">${meta.icon}</span>
            <span class="reorder-name">${meta.name}</span>
          </div>
          <div style="display:flex; align-items:center; gap:10px;">
            <label class="switch">
              <input type="checkbox" onchange="tabEnabled[${tabId}] = this.checked" ${isEn ? 'checked' : ''}>
              <span class="slider"></span>
            </label>
            <div class="reorder-btns">
              <button type="button" class="btn-arrow" onclick="moveTab(${idx}, -1)" ${idx === 0 ? 'disabled' : ''}>▲</button>
              <button type="button" class="btn-arrow" onclick="moveTab(${idx}, 1)" ${idx === tabOrder.length - 1 ? 'disabled' : ''}>▼</button>
            </div>
          </div>
        `;
        wrap.appendChild(item);
      });
    }

    function moveTab(idx, dir) {
      const target = idx + dir;
      if (target < 0 || target >= tabOrder.length) return;
      const tmp = tabOrder[idx];
      tabOrder[idx] = tabOrder[target];
      tabOrder[target] = tmp;
      renderTabReorderList();
    }

    function saveSettings() {
      const btn = document.getElementById('btnSave');
      btn.innerText = "⏳ Speichere...";
      btn.disabled = true;

      const payload = {
        wifi_ssid: document.getElementById('wifi_ssid').value,
        wifi_pass: document.getElementById('wifi_pass').value,
        vanpi_ip: document.getElementById('vanpi_ip').value,

        dark_mode: document.getElementById('dark_mode').checked,
        buzzer_enabled: document.getElementById('buzzer_enabled').checked,
        debug_mode: document.getElementById('debug_mode').checked,
        display_brightness: parseInt(document.getElementById('display_brightness').value),
        display_timeout: parseInt(document.getElementById('display_timeout').value),
        screensaver_enabled: document.getElementById('screensaver_enabled').checked,

        tank_count: parseInt(document.getElementById('tank_count').value),
        tank_show_liters: document.getElementById('tank_show_liters').checked,
        tank_max: [
          parseInt(document.getElementById('tank_max_0').value) || 100,
          parseInt(document.getElementById('tank_max_1').value) || 100,
          parseInt(document.getElementById('tank_max_2').value) || 100,
          parseInt(document.getElementById('tank_max_3').value) || 100
        ],
        tank_unit: [
          parseInt(document.getElementById('tank_unit_0').value) || 0,
          parseInt(document.getElementById('tank_unit_1').value) || 0,
          parseInt(document.getElementById('tank_unit_2').value) || 0,
          parseInt(document.getElementById('tank_unit_3').value) || 0
        ],

        pump_relay: parseInt(document.getElementById('pump_relay').value),
        drain_relay: parseInt(document.getElementById('drain_relay').value),
        heater_type: parseInt(document.getElementById('heater_type').value) || 0,

        show_wrelay: document.getElementById('show_wrelay').checked,
        switch_visible: [],
        dimmer_visible: [],
        wrelay_visible: [],

        outdoor_temp_sensor: parseInt(document.getElementById('outdoor_temp_sensor').value),
        indoor_temp_sensor: parseInt(document.getElementById('indoor_temp_sensor').value),
        temps_custom_sensor: [
          parseInt(document.getElementById('temps_custom_sensor_0').value),
          parseInt(document.getElementById('temps_custom_sensor_1').value)
        ],

        warn_frost_temp: parseFloat(document.getElementById('warn_frost_temp').value),
        warn_bat_soc: parseInt(document.getElementById('warn_bat_soc').value),
        warn_bat_yellow_soc: parseInt(document.getElementById('warn_bat_yellow_soc').value),
        warn_fresh_min: parseInt(document.getElementById('warn_fresh_min').value),
        warn_waste_max: parseInt(document.getElementById('warn_waste_max').value),
        solar_max_w: parseFloat(document.getElementById('solar_max_w').value),
        bat_capacity_ah: parseFloat(document.getElementById('bat_capacity_ah').value),

        home_show_battery: document.getElementById('home_show_battery').checked,
        home_show_starter: document.getElementById('home_show_starter').checked,
        home_show_solar: document.getElementById('home_show_solar').checked,
        home_show_water: document.getElementById('home_show_water').checked,
        home_water_source: parseInt(document.getElementById('home_water_source').value),
        home_favorite: [
          parseInt(document.getElementById('home_favorite_0').value),
          parseInt(document.getElementById('home_favorite_1').value)
        ],

        tab_order: tabOrder,
        tab_enabled: []
      };

      for (let i = 0; i < 8; i++) {
        const sw = document.getElementById('switch_vis_' + i);
        payload.switch_visible.push(sw ? sw.checked : true);
        const dm = document.getElementById('dimmer_vis_' + i);
        payload.dimmer_visible.push(dm ? dm.checked : true);
        const wr = document.getElementById('wrelay_vis_' + i);
        payload.wrelay_visible.push(wr ? wr.checked : true);
      }

      for (let i = 0; i < tabMeta.length; i++) {
        payload.tab_enabled.push(tabEnabled[i] !== undefined ? tabEnabled[i] : true);
      }

      fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      .then(r => r.json())
      .then(res => {
        btn.innerText = "💾 Gespeichert!";
        showToast("✅ Einstellungen erfolgreich auf Display & NVS gespeichert!");
        setTimeout(() => {
          btn.innerText = "💾 Einstellungen speichern";
          btn.disabled = false;
        }, 1500);
      })
      .catch(err => {
        console.error(err);
        btn.innerText = "❌ Fehler";
        btn.disabled = false;
        showToast("Fehler beim Speichern: " + err.message, true);
      });
    }

    function rebootEsp() {
      if (!confirm("ESP32-S3 jetzt neu starten?")) return;
      fetch('/api/restart', { method: 'POST' })
        .then(() => {
          showToast("ESP32 wird neu gestartet...");
          setTimeout(() => { location.reload(); }, 4000);
        });
    }

    function scanWifi() {
      const modal = document.getElementById('wifiModal');
      const list = document.getElementById('wifiList');
      modal.classList.add('show');
      list.innerHTML = '<div style="padding:12px; text-align:center; color:#94a3b8;">🔍 Suche WLAN-Netzwerke...</div>';

      fetch('/api/wifi/scan')
        .then(r => r.json())
        .then(networks => {
          if (!networks || !networks.length) {
            list.innerHTML = '<div style="padding:12px; text-align:center; color:#94a3b8;">Keine Netzwerke gefunden</div>';
            return;
          }
          list.innerHTML = '';
          networks.forEach(net => {
            const item = document.createElement('div');
            item.className = 'wifi-item';
            item.innerHTML = `
              <span class="wifi-name">${net.ssid}</span>
              <span class="wifi-rssi">${net.rssi} dBm ${net.secure ? '🔒' : ''}</span>
            `;
            item.onclick = () => {
              document.getElementById('wifi_ssid').value = net.ssid;
              closeWifiModal();
              document.getElementById('wifi_pass').focus();
            };
            list.appendChild(item);
          });
        })
        .catch(err => {
          list.innerHTML = '<div style="padding:12px; text-align:center; color:#ef4444;">Scan fehlgeschlagen</div>';
        });
    }

    function closeWifiModal() {
      document.getElementById('wifiModal').classList.remove('show');
    }

    // Init
    window.addEventListener('DOMContentLoaded', loadSettings);
  </script>
</body>
</html>
)rawliteral";

#endif // WEB_SETTINGS_HTML_H
