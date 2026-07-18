#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "config.h"
#include "heartbeat.h"
#include "rain.h"
#include "web.h"

extern const char* programVers;

extern bool bmeOK;
extern bool lightOK;
extern bool runtimeSensorFaultActive;
extern bool accessPointModeActive;
extern bool clockSynchronized;
extern float temperature;
extern float humidity;
extern float pressure;
extern float seaLevelPressure;
extern float lightLux;
extern float lightWm2;
extern int rssi;

// WebServer instance on port 80
WebServer server(80);

File uploadFile;

namespace {

String htmlEscape(const String& value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  escaped.replace("'", "&#39;");
  return escaped;
}

String formatFloatValue(float value, uint8_t decimals = 1, const char* suffix = "") {
  if (isnan(value)) {
    return "N/A";
  }
  return String(value, static_cast<unsigned int>(decimals)) + suffix;
}

String formatIntValue(int value, const char* suffix = "") {
  return String(value) + suffix;
}

String formatBoolBadge(bool enabled, const char* onLabel = "Active", const char* offLabel = "Off") {
  return String("<span class='status-pill ") + (enabled ? "ok'>" : "muted'>") + (enabled ? onLabel : offLabel) + "</span>";
}

String logEscape(const String& value) {
  String escaped = htmlEscape(value);
  escaped.replace("\n", "<br>");
  return escaped;
}

String formatRuntimeState() {
  if (runtimeSensorFaultActive) {
    return "<span class='status-pill danger'>Sensor fault</span>";
  }
  if (accessPointModeActive) {
    return "<span class='status-pill warn'>WiFi Manager</span>";
  }
  if (WiFi.status() == WL_CONNECTED) {
    return "<span class='status-pill ok'>Online</span>";
  }
  return "<span class='status-pill muted'>Offline</span>";
}

String wifiSsidValue() {
  return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String("Not connected");
}

String localIpValue() {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("N/A");
}

String formatUptime() {
  unsigned long totalSeconds = millis() / 1000UL;
  unsigned long days = totalSeconds / 86400UL;
  totalSeconds %= 86400UL;
  unsigned long hours = totalSeconds / 3600UL;
  totalSeconds %= 3600UL;
  unsigned long minutes = totalSeconds / 60UL;

  String out;
  if (days > 0) {
    out += String(days) + "d ";
  }
  if (hours > 0 || days > 0) {
    out += String(hours) + "h ";
  }
  out += String(minutes) + "m";
  return out;
}

String buildHead(const char* title) {
  return String()
    + "<!DOCTYPE html><html lang='en'><head>"
    + "<meta charset='UTF-8'>"
    + "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    + "<link href='http://api.ok1kky.cz/wx/favicon.ico' rel='icon' type='image/x-icon'>"
    + "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    + "<link href='https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css' rel='stylesheet'>"
    + "<title>" + String(title) + "</title>"
    + "<style>"
      ":root {"
        "--bg: #181c20;"
        "--surface: #212529;"
        "--surface-2: #2b3035;"
        "--surface-3: #343a40;"
        "--border: #495057;"
        "--text: #f8f9fa;"
        "--muted: #adb5bd;"
        "--accent: #20c997;"
        "--warn: #ffc107;"
        "--danger: #dc3545;"
        "--shadow: 0 0.5rem 1rem rgba(0,0,0,.22);"
      "}"
      "body { background-color: var(--bg); color: var(--text); min-height: 100vh; display: flex; flex-direction: column; }"
      "a { color: #8ecae6; }"
      ".page-shell { max-width: 1080px; }"
      ".page-content { flex: 1 0 auto; }"
      ".navbar { background: var(--surface) !important; border-bottom: 1px solid rgba(255,255,255,.08); }"
      ".navbar-brand, .nav-link, .dropdown-item, .navbar-toggler { color: var(--text) !important; }"
      ".navbar-toggler { border-color: rgba(255,255,255,.12); }"
      ".navbar-toggler-icon { filter: invert(1) brightness(1.7); }"
      ".nav-link.active { color: var(--accent) !important; }"
      ".nav-link:hover { color: #ffffff !important; }"
      ".dropdown-menu { background: var(--surface-2); border: 1px solid rgba(255,255,255,.08); box-shadow: var(--shadow); }"
      ".dropdown-item:hover, .dropdown-item:focus { background: var(--surface-3); color: var(--text) !important; }"
      ".btn-success { background: var(--accent); border-color: var(--accent); color: #102418; font-weight: 600; }"
      ".btn-success:hover { background: #1db589; border-color: #1db589; color: #0d1f15; }"
      ".btn-outline-light { border-color: rgba(255,255,255,.18); color: var(--text); }"
      ".btn-outline-light:hover { background: var(--surface-3); border-color: var(--surface-3); color: var(--text); }"
      ".info-card, .panel, section { background: var(--surface); border: 1px solid rgba(255,255,255,.08); border-radius: .75rem; box-shadow: var(--shadow); }"
      ".info-card { padding: 1rem; height: 100%; position: relative; }"
      ".info-card::before { content: ''; position: absolute; top: 0; left: 0; right: 0; height: 3px; background: linear-gradient(90deg, var(--accent), transparent 70%); border-radius: .75rem .75rem 0 0; opacity: .85; }"
      ".metric-value { font-size: clamp(1.6rem, 3vw, 2.4rem); font-weight: 700; line-height: 1.1; }"
      ".metric-label, .muted-text, .form-text { color: var(--muted) !important; }"
      ".status-pill { display: inline-flex; align-items: center; gap: .35rem; padding: .35rem .75rem; border-radius: 999px; font-size: .82rem; font-weight: 600; border: 1px solid transparent; }"
      ".status-pill.ok { color: #8ff0c7; background: rgba(32, 201, 151, .16); border-color: rgba(32,201,151,.35); }"
      ".status-pill.warn { color: #ffe08a; background: rgba(255, 193, 7, .16); border-color: rgba(255,193,7,.3); }"
      ".status-pill.danger { color: #ff9aa5; background: rgba(220, 53, 69, .16); border-color: rgba(220,53,69,.3); }"
      ".status-pill.muted { color: #ced4da; background: rgba(173,181,189,.12); border-color: rgba(173,181,189,.18); }"
      ".panel, section { padding: 1.25rem; margin-bottom: 1.25rem; }"
      "section.last { margin-bottom: 0; }"
      "section h5 { margin-bottom: 1rem; color: var(--text); }"
      ".list-table { width: 100%; }"
      ".list-table td { padding: .55rem 0; border-bottom: 1px solid rgba(255,255,255,.08); }"
      ".list-table tr:last-child td { border-bottom: none; }"
      ".list-table td:last-child { text-align: right; color: var(--text); font-weight: 500; }"
      ".card-grid { row-gap: 1rem; }"
      ".form-control, .input-group-text, .form-select { background: var(--surface-2); border: 1px solid var(--border); color: var(--text); }"
      ".form-control:focus, .form-select:focus { background: var(--surface-2); color: var(--text); border-color: rgba(32,201,151,.55); box-shadow: 0 0 0 .2rem rgba(32,201,151,.12); }"
      ".form-control::placeholder { color: #708497; }"
      ".input-group-text { color: var(--muted); }"
      ".form-check-input { background-color: #24313d; border-color: #435567; }"
      ".form-check-input:checked { background-color: var(--accent); border-color: var(--accent); }"
      ".alert-info { background: rgba(13, 110, 253, .15); color: #d7e7ff; border: 1px solid rgba(13,110,253,.28); }"
      ".footer { color: var(--muted); border-top: 1px solid rgba(255,255,255,.08); margin-top: auto; }"
      ".mini-note { font-size: .88rem; color: var(--muted); }"
      "@media (max-width: 767.98px) {"
        ".info-card, .panel, section { border-radius: .75rem; }"
      "}"
    + "</style>"
    + "</head><body>";
}

String buildNavbar(const String& activePath, bool showSaveButton = false) {
  String html =
    "<nav class='navbar shadow-sm sticky-top navbar-expand-lg'>"
      "<div class='container page-shell mx-auto'>"
        "<a class='navbar-brand fw-semibold' href='/'>WX Station</a>"
        "<button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarNav' aria-controls='navbarNav' aria-expanded='false' aria-label='Toggle navigation'>"
          "<span class='navbar-toggler-icon'></span>"
        "</button>"
        "<div class='collapse navbar-collapse' id='navbarNav'>"
          "<ul class='navbar-nav me-auto mb-2 mb-lg-0'>"
            "<li class='nav-item'><a class='nav-link " + String(activePath == "/" ? "active" : "") + "' href='/'>Dashboard</a></li>"
            "<li class='nav-item'><a class='nav-link " + String(activePath == "/setting" ? "active" : "") + "' href='/setting'>Settings</a></li>"
            "<li class='nav-item dropdown'>"
              "<a class='nav-link dropdown-toggle' href='#' id='backupDropdown' role='button' data-bs-toggle='dropdown' aria-expanded='false'>Backup</a>"
              "<ul class='dropdown-menu' aria-labelledby='backupDropdown'>"
                "<li><a class='dropdown-item' href='/download'>Download</a></li>"
                "<li>"
                  "<form id='restoreForm' method='POST' action='/restore' enctype='multipart/form-data' style='display:none;'>"
                    "<input type='file' name='restoreFile' id='restoreFile' onchange='document.getElementById(\"restoreForm\").submit();'>"
                  "</form>"
                  "<a class='dropdown-item' href='#' onclick='document.getElementById(\"restoreFile\").click(); return false;'>Restore</a>"
                "</li>"
              "</ul>"
            "</li>"
            "<li class='nav-item dropdown'>"
              "<a class='nav-link dropdown-toggle' href='#' id='deviceDropdown' role='button' data-bs-toggle='dropdown' aria-expanded='false'>Device</a>"
              "<ul class='dropdown-menu' aria-labelledby='deviceDropdown'>"
                "<li><a class='dropdown-item' href='#' onclick='startWifiManager(); return false;'>WiFi Manager</a></li>"
                "<li><a class='dropdown-item' href='#' onclick='confirmFactoryReset(); return false;'>Factory reset</a></li>"
                "<li><a class='dropdown-item' href='#' onclick='confirmReboot(); return false;'>Reboot</a></li>"
              "</ul>"
            "</li>"
            "<li class='nav-item dropdown'>"
              "<a class='nav-link dropdown-toggle' href='#' id='moreDropdown' role='button' data-bs-toggle='dropdown' aria-expanded='false'>More</a>"
              "<ul class='dropdown-menu dropdown-menu-end' aria-labelledby='moreDropdown'>"
                "<li><a class='dropdown-item' href='/debug'>Debug</a></li>"
                "<li><a class='dropdown-item' href='/config.json' target='_blank'>Config</a></li>"
              "</ul>"
            "</li>"
          "</ul>";

  if (showSaveButton) {
    html += "<button class='btn btn-success ms-auto' type='submit' form='configForm'>Save</button>";
  } else {
    html += "<div class='ms-auto d-flex align-items-center gap-2'><span class='mini-note'>"
            + htmlEscape(config.stationName.length() ? config.stationName : String("Weather station"))
            + "</span></div>";
  }

  html +=
        "</div>"
      "</div>"
    "</nav>";

  return html;
}

String buildSweetAlertScript() {
  String flashTitle;
  String flashText;
  String flashIcon;

  if (server.hasArg("saved")) {
    flashTitle = "Settings saved";
    flashText = "Configuration was saved successfully.";
    flashIcon = "success";
  } else if (server.hasArg("restored")) {
    flashTitle = "Backup restored";
    flashText = "Configuration was restored successfully.";
    flashIcon = "success";
  } else if (server.hasArg("factory")) {
    flashTitle = "Factory reset complete";
    flashText = "Default configuration has been loaded.";
    flashIcon = "success";
  }

  String script =
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "<script src='https://cdn.jsdelivr.net/npm/sweetalert2@11'></script>"
    "<script>"
      "const swalTheme={background:'#212529',color:'#f8f9fa',confirmButtonColor:'#20c997',cancelButtonColor:'#6c757d'};"
      "function showToast(icon,title,text){Swal.fire(Object.assign({},swalTheme,{toast:true,position:'bottom-end',icon:icon,title:title,text:text,showConfirmButton:false,timer:2200,timerProgressBar:true}));}"
      "function showDialog(icon,title,text){return Swal.fire(Object.assign({},swalTheme,{icon:icon,title:title,text:text}));}"
      "async function startWifiManager(){await showDialog('info','WiFi Manager','After starting WiFi Manager, this page will be deactivated until the manager is closed.');fetch('/wifi');}"
      "async function confirmFactoryReset(){const result=await Swal.fire(Object.assign({},swalTheme,{icon:'warning',title:'Factory reset?',text:'All settings will be deleted.',showCancelButton:true,confirmButtonText:'Reset',cancelButtonText:'Cancel'}));if(result.isConfirmed){window.location.href='/factory';}}"
      "async function confirmReboot(){const result=await Swal.fire(Object.assign({},swalTheme,{icon:'question',title:'Reboot device?',text:'WX Station will restart immediately.',showCancelButton:true,confirmButtonText:'Reboot',cancelButtonText:'Cancel'}));if(result.isConfirmed){showDialog('success','Rebooting','WX Station is restarting...');fetch('/reboot');}}";

  if (flashTitle.length()) {
    script += "document.addEventListener('DOMContentLoaded',function(){showToast('"
      + flashIcon + "','"
      + htmlEscape(flashTitle) + "','"
      + htmlEscape(flashText) + "');});";
  }

  script += "</script>";
  return script;
}

String buildFooter() {
  return String()
    + "<footer class='footer text-center py-3 mt-4 small'>"
      "Made with ❤️ by <a href='https://www.ok1kky.cz' target='_blank'>OK1KKY</a> | "
      "WX-Station " + String(programVers) +
    "</footer>"
    + buildSweetAlertScript()
    + "</body></html>";
}

String buildDashboardRefreshScript() {
  return String()
    + "<script>"
      "function setText(id,value){const el=document.getElementById(id);if(el){el.textContent=value;}}"
      "function setHtml(id,value){const el=document.getElementById(id);if(el){el.innerHTML=value;}}"
      "async function refreshDashboard(){"
        "try{"
          "const response=await fetch('/status',{cache:'no-store'});"
          "if(!response.ok)return;"
          "const data=await response.json();"
          "setText('card-temperature',data.temperature);"
          "setText('card-humidity',data.humidity);"
          "setText('card-pressure',data.pressureRel);"
          "setText('sensor-temperature',data.temperature);"
          "setText('sensor-humidity',data.humidity);"
          "setText('sensor-pressure-abs',data.pressureAbs);"
          "setText('sensor-pressure-rel',data.pressureRel);"
          "setText('sensor-light',data.light);"
          "setText('sensor-rain-1h',data.rain1h);"
          "setText('sensor-rain-24h',data.rain24h);"
          "setText('sys-uptime',data.uptime);"
          "setText('sys-ssid',data.ssid);"
          "setText('sys-ip',data.ip);"
          "setText('sys-rssi',data.rssi);"
          "setText('sys-aprs',data.aprs);"
          "setText('sys-mqtt',data.mqtt);"
          "setText('sys-syslog',data.syslog);"
          "setHtml('system-status',data.runtimeState);"
        "}catch(e){}"
      "}"
      "document.addEventListener('DOMContentLoaded',function(){setInterval(refreshDashboard,300000);});"
    + "</script>";
}

String buildDebugRefreshScript() {
  return String()
    + "<script>"
      "async function clearDebugLog(){"
        "const result=await Swal.fire(Object.assign({},swalTheme,{icon:'warning',title:'Clear debug log?',text:'Stored web debug output will be removed.',showCancelButton:true,confirmButtonText:'Clear',cancelButtonText:'Cancel'}));"
        "if(!result.isConfirmed)return;"
        "try{await fetch('/debug/clear',{method:'POST'});refreshDebugLog();showToast('success','Log cleared','Debug output has been removed.');}catch(e){}"
      "}"
      "async function refreshDebugLog(){"
        "try{"
          "const response=await fetch('/debug/logs',{cache:'no-store'});"
          "if(!response.ok)return;"
          "const html=await response.text();"
          "const log=document.getElementById('debug-log');"
          "if(!log)return;"
          "const atBottom=(log.scrollTop+log.clientHeight)>=log.scrollHeight-24;"
          "log.innerHTML=html;"
          "if(atBottom){log.scrollTop=log.scrollHeight;}"
        "}catch(e){}"
      "}"
      "document.addEventListener('DOMContentLoaded',function(){const log=document.getElementById('debug-log');if(log){log.scrollTop=log.scrollHeight;}setInterval(refreshDebugLog,2000);});"
    + "</script>";
}

String buildStatusJson() {
  DynamicJsonDocument doc(1024);
  doc["temperature"] = formatFloatValue(temperature, 1, " °C");
  doc["humidity"] = formatFloatValue(humidity, 1, " %");
  doc["pressureAbs"] = formatFloatValue(pressure, 1, " hPa");
  doc["pressureRel"] = formatFloatValue(seaLevelPressure, 1, " hPa");
  doc["light"] = config.activeLight ? formatFloatValue(lightWm2, 2, " W/m²") : String("Disabled");
  doc["rain1h"] = config.activeRain ? formatFloatValue(RainGauge::getRainLastHourMm(), 2, " mm") : String("Disabled");
  doc["rain24h"] = config.activeRain ? formatFloatValue(RainGauge::getRainLast24HoursMm(), 2, " mm") : String("Disabled");
  doc["uptime"] = formatUptime();
  doc["ssid"] = wifiSsidValue();
  doc["ip"] = localIpValue();
  doc["rssi"] = formatIntValue(rssi, " dBm");
  doc["aprs"] = config.activeAPRS ? "Enabled" : "Disabled";
  doc["mqtt"] = config.activeMQTT ? "Enabled" : "Disabled";
  doc["syslog"] = config.activeSYSLOG ? "Enabled" : "Disabled";
  doc["runtimeState"] = formatRuntimeState();

  String json;
  serializeJson(doc, json);
  return json;
}

String buildDashboardPage() {
  String ssid = wifiSsidValue();
  String localIp = localIpValue();

  String html = buildHead("WX Dashboard");
  html += buildNavbar("/", false);
  html += "<main class='page-content'><div class='container page-shell mx-auto py-4'>";

  html += "<div class='row card-grid mb-4'>";
  html +=
    "<div class='col-6 col-md-4'><div class='info-card'><div class='metric-label'>Temperature</div><div id='card-temperature' class='metric-value'>"
    + formatFloatValue(temperature, 1, " °C")
    + "</div></div></div>";
  html +=
    "<div class='col-6 col-md-4'><div class='info-card'><div class='metric-label'>Humidity</div><div id='card-humidity' class='metric-value'>"
    + formatFloatValue(humidity, 1, " %")
    + "</div></div></div>";
  html +=
    "<div class='col-12 col-md-4'><div class='info-card'><div class='metric-label'>Pressure</div><div id='card-pressure' class='metric-value'>"
    + formatFloatValue(seaLevelPressure, 1, " hPa")
    + "</div></div></div>";
  html += "</div>";

  html += "<div class='row g-4'>";
  html +=
    "<div class='col-lg-7'>"
      "<div class='panel'>"
        "<h5 class='mb-3'><i class='bi bi-thermometer-half'></i> Sensors</h5>"
        "<table class='list-table'>"
          "<tr><td>Temperature</td><td id='sensor-temperature'>" + formatFloatValue(temperature, 1, " °C") + "</td></tr>"
          "<tr><td>Humidity</td><td id='sensor-humidity'>" + formatFloatValue(humidity, 1, " %") + "</td></tr>"
          "<tr><td>Pressure (abs)</td><td id='sensor-pressure-abs'>" + formatFloatValue(pressure, 1, " hPa") + "</td></tr>"
          "<tr><td>Pressure (rel)</td><td id='sensor-pressure-rel'>" + formatFloatValue(seaLevelPressure, 1, " hPa") + "</td></tr>"
          "<tr><td>Light</td><td id='sensor-light'>" + (config.activeLight ? formatFloatValue(lightWm2, 2, " W/m²") : String("Disabled")) + "</td></tr>"
          "<tr><td>Rain 1h</td><td id='sensor-rain-1h'>" + (config.activeRain ? formatFloatValue(RainGauge::getRainLastHourMm(), 2, " mm") : String("Disabled")) + "</td></tr>"
          "<tr><td>Rain 24h</td><td id='sensor-rain-24h'>" + (config.activeRain ? formatFloatValue(RainGauge::getRainLast24HoursMm(), 2, " mm") : String("Disabled")) + "</td></tr>"
        "</table>"
      "</div>"
    "</div>";

  html +=
    "<div class='col-lg-5'>"
      "<div class='panel'>"
        "<h5 class='mb-3'><i class='bi bi-cpu'></i> System</h5>"
        "<table class='list-table'>"
          "<tr><td>Uptime</td><td id='sys-uptime'>" + formatUptime() + "</td></tr>"
          "<tr><td>SSID</td><td id='sys-ssid'>" + htmlEscape(ssid) + "</td></tr>"
          "<tr><td>IP address</td><td id='sys-ip'>" + htmlEscape(localIp) + "</td></tr>"
          "<tr><td>RSSI</td><td id='sys-rssi'>" + formatIntValue(rssi, " dBm") + "</td></tr>"
          "<tr><td>APRS</td><td id='sys-aprs'>" + String(config.activeAPRS ? "Enabled" : "Disabled") + "</td></tr>"
          "<tr><td>MQTT</td><td id='sys-mqtt'>" + String(config.activeMQTT ? "Enabled" : "Disabled") + "</td></tr>"
          "<tr><td>Syslog</td><td id='sys-syslog'>" + String(config.activeSYSLOG ? "Enabled" : "Disabled") + "</td></tr>"
        "</table>"
      "</div>"
    "</div>";
  html += "</div>";

  html += "</div></main>";
  html += buildDashboardRefreshScript();
  html += buildFooter();
  return html;
}

String buildSettingsPage() {
  String html = buildHead("WX Settings");
  html += buildNavbar("/setting", true);

  html += "<main class='page-content'><div class='container page-shell py-4 pb-2 mx-auto'>"
          "<form id='configForm' method='POST' action='/save'>";

  html +=
    "<div class='alert alert-info alert-dismissible fade show text-center mb-4' role='alert'>"
      "Before you start configuring your WX-Station, please read the instructions at "
      "<a href='https://github.com/ondrahladik/WX-Station/tree/main/docs' class='alert-link' target='_blank'>GitHub Docs</a>."
    "</div>";

  html +=
    "<section>"
      "<h5><i class='bi bi-person-circle'></i> STATION</h5>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>Name / ASL</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<input type='text' class='form-control' name='stationName' value='" + htmlEscape(config.stationName) + "' placeholder='wx-station'>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' step='0.1' class='form-control' name='altitude' value='" + String(config.altitude, 1) + "' placeholder='230.0'>"
            "<span class='input-group-text'>m</span>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<h5><i class='bi bi-server'></i> DATA</h5>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>Temp / Offset</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<input type='text' class='form-control' name='dataTemp' value='" + htmlEscape(config.dataTemp) + "' placeholder='temperature'>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' step='0.1' class='form-control' name='offsetTemp' value='" + String(config.offsetTemp, 1) + "' placeholder='0.0'>"
            "<span class='input-group-text'>°C</span>"
          "</div>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>Humi / Offset</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<input type='text' class='form-control' name='dataHumi' value='" + htmlEscape(config.dataHumi) + "' placeholder='humidity'>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' step='0.1' class='form-control' name='offsetHumi' value='" + String(config.offsetHumi, 1) + "' placeholder='0.0'>"
            "<span class='input-group-text'>%</span>"
          "</div>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>Press / Offset</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<input type='text' class='form-control' name='dataPress' value='" + htmlEscape(config.dataPress) + "' placeholder='pressure'>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' step='0.1' class='form-control' name='offsetPress' value='" + String(config.offsetPress, 1) + "' placeholder='0.0'>"
            "<span class='input-group-text'>hPa</span>"
          "</div>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<div class='col-12 col-md-4 d-flex align-items-center'>"
          "<label class='col-form-label me-2'>Light</label>"
          "<div class='form-check form-switch m-0'>"
            "<input class='form-check-input' type='checkbox' name='activeLight' " + String(config.activeLight ? "checked" : "") + ">"
          "</div>"
        "</div>"
        "<div class='col-12 col-md-8'>"
          "<input type='text' class='form-control' name='dataLight' value='" + htmlEscape(config.dataLight) + "' placeholder='light'>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<div class='col-12 col-md-4 d-flex align-items-center'>"
          "<label class='col-form-label me-2'>Rain</label>"
          "<div class='form-check form-switch m-0'>"
            "<input class='form-check-input' type='checkbox' id='activeRain' name='activeRain' " + String(config.activeRain ? "checked" : "") + ">"
          "</div>"
        "</div>"
        "<div class='col-12 col-md-8'>"
          "<div class='input-group'>"
            "<input type='number' step='0.0001' class='form-control' name='rainTipMm' value='" + String(config.rainTipMm, 4) + "' placeholder='0.2794'>"
            "<span class='input-group-text'>mm/tip</span>"
          "</div>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>RSSI</label>"
        "<div class='col-12 col-md-8'>"
          "<input type='text' class='form-control' name='dataRssi' value='" + htmlEscape(config.dataRssi) + "' placeholder='rssi'>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<h5><i class='bi bi-hdd-stack-fill'></i> SERVER</h5>"
      "<div class='d-flex flex-wrap justify-content-between align-items-center gap-3 mb-3'>"
        "<div class='d-flex align-items-center'><p class='mb-0'>Server <i class='bi bi-info-lg'></i></p><div class='form-check form-switch ms-2 mb-0'><input class='form-check-input' type='checkbox' id='serverActive0' name='serverActive0' "
          + String(config.serverActive0 ? "checked" : "")
          + " onclick='document.getElementById(\"ser0Fields\").style.display=this.checked?\"block\":\"none\";'></div></div>"
        "<div class='d-flex align-items-center'><p class='mb-0'>Server 1</p><div class='form-check form-switch ms-2 mb-0'><input class='form-check-input' type='checkbox' id='serverActive1' name='serverActive1' "
          + String(config.serverActive1 ? "checked" : "")
          + " onclick='document.getElementById(\"ser1Fields\").style.display=this.checked?\"block\":\"none\";'></div></div>"
        "<div class='d-flex align-items-center'><p class='mb-0'>Server 2</p><div class='form-check form-switch ms-2 mb-0'><input class='form-check-input' type='checkbox' id='serverActive2' name='serverActive2' "
          + String(config.serverActive2 ? "checked" : "")
          + " onclick='document.getElementById(\"ser2Fields\").style.display=this.checked?\"block\":\"none\";'></div></div>"
        "<div class='d-flex align-items-center'><p class='mb-0'>Server 3</p><div class='form-check form-switch ms-2 mb-0'><input class='form-check-input' type='checkbox' id='serverActive3' name='serverActive3' "
          + String(config.serverActive3 ? "checked" : "")
          + " onclick='document.getElementById(\"ser3Fields\").style.display=this.checked?\"block\":\"none\";'></div></div>"
      "</div>"
      "<div id='ser0Fields' style='display:" + String(config.serverActive0 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server <i class='bi bi-info-lg'></i></label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='serverUrl0' value='" + htmlEscape(config.serverUrl0) + "' placeholder='http://example.com/'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='serverName0' value='" + htmlEscape(config.serverName0) + "' placeholder='wx-station'></div>"
        "</div>"
      "</div>"
      "<div id='ser1Fields' style='display:" + String(config.serverActive1 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server 1</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='serverUrl1' value='" + htmlEscape(config.serverUrl1) + "' placeholder='http://example.com/'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='serverName1' value='" + htmlEscape(config.serverName1) + "' placeholder='wx-station'></div>"
        "</div>"
      "</div>"
      "<div id='ser2Fields' style='display:" + String(config.serverActive2 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server 2</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='serverUrl2' value='" + htmlEscape(config.serverUrl2) + "' placeholder='http://example.com/'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='serverName2' value='" + htmlEscape(config.serverName2) + "' placeholder='wx-station'></div>"
        "</div>"
      "</div>"
      "<div id='ser3Fields' style='display:" + String(config.serverActive3 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server 3</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='serverUrl3' value='" + htmlEscape(config.serverUrl3) + "' placeholder='http://example.com/'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='serverName3' value='" + htmlEscape(config.serverName3) + "' placeholder='wx-station'></div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-broadcast'></i> APRS</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeAPRS' name='activeAPRS' "
          + String(config.activeAPRS ? "checked" : "")
          + " onclick='document.getElementById(\"aprsFields\").style.display=this.checked?\"block\":\"none\";'>"
        "</div>"
      "</div>"
      "<div id='aprsFields' style='display:" + String(config.activeAPRS ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Host / Port</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='aprsHost' value='" + htmlEscape(config.aprsHost) + "' placeholder='euro.aprs2.net'></div>"
          "<div class='col-12 col-md-4'><input type='number' class='form-control' name='aprsPort' value='" + String(config.aprsPort) + "' placeholder='14580'></div>"
        "</div>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Call / Pass</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='aprsCall' value='" + htmlEscape(config.aprsCall) + "' placeholder='NOCALL-13'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='aprsPass' value='" + htmlEscape(config.aprsPass) + "' placeholder='12345'></div>"
        "</div>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Lat / Lon</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='aprsLat' value='" + htmlEscape(config.aprsLat) + "' placeholder='0000.00N'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='aprsLon' value='" + htmlEscape(config.aprsLon) + "' placeholder='00000.00E'></div>"
        "</div>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Comment</label>"
          "<div class='col-12 col-md-8'><input type='text' class='form-control' name='aprsComment' value='" + htmlEscape(String(config.aprsComment)) + "' placeholder='WX-Station https://www.ok1kky.cz'></div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-cloud-fill'></i> MQTT</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeMQTT' name='activeMQTT' "
          + String(config.activeMQTT ? "checked" : "")
          + " onclick='document.getElementById(\"mqttFields\").style.display=this.checked?\"block\":\"none\";'>"
        "</div>"
      "</div>"
      "<div id='mqttFields' style='display:" + String(config.activeMQTT ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server / Port</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='mqttServer' value='" + htmlEscape(config.mqttServer) + "' placeholder='example.com'></div>"
          "<div class='col-12 col-md-4'><input type='number' class='form-control' name='mqttPort' value='" + String(config.mqttPort) + "' placeholder='1883'></div>"
        "</div>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Pub topic</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='mqttTopicPub1' value='" + htmlEscape(config.mqttTopicPub1) + "' placeholder='Pub topic 1'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='mqttTopicPub2' value='" + htmlEscape(config.mqttTopicPub2) + "' placeholder='Pub topic 2'></div>"
        "</div>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Sub topic</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='mqttTopicSub1' value='" + htmlEscape(config.mqttTopicSub1) + "' placeholder='Sub topic 1'></div>"
          "<div class='col-12 col-md-4'><input type='text' class='form-control' name='mqttTopicSub2' value='" + htmlEscape(config.mqttTopicSub2) + "' placeholder='Sub topic 2'></div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-file-earmark-text-fill'></i> SYSLOG</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeSYSLOG' name='activeSYSLOG' "
          + String(config.activeSYSLOG ? "checked" : "")
          + " onclick='document.getElementById(\"syslogFields\").style.display=this.checked?\"block\":\"none\";'>"
        "</div>"
      "</div>"
      "<div id='syslogFields' style='display:" + String(config.activeSYSLOG ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-12 col-md-4 col-form-label'>Server / Port</label>"
          "<div class='col-12 col-md-4 mb-3 mb-md-0'><input type='text' class='form-control' name='syslogServer' value='" + htmlEscape(config.syslogServer) + "' placeholder='example.com'></div>"
          "<div class='col-12 col-md-4'><input type='number' class='form-control' name='syslogPort' value='" + String(config.syslogPort) + "' placeholder='514'></div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<h5><i class='bi bi-clock-fill'></i> INTERVAL</h5>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>Reboot / Server</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<select class='form-select' name='restartMode'>"
            "<option value='0'" + String(config.restartMode == 0 ? " selected" : "") + ">Disable</option>"
            "<option value='1'" + String(config.restartMode == 1 ? " selected" : "") + ">6 hours</option>"
            "<option value='2'" + String(config.restartMode == 2 ? " selected" : "") + ">12 hours</option>"
            "<option value='3'" + String(config.restartMode == 3 ? " selected" : "") + ">24 hours</option>"
            "<option value='4'" + String(config.restartMode == 4 ? " selected" : "") + ">48 hours</option>"
          "</select>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' class='form-control' name='intervalHttp' value='" + String(config.intervalHttp / 60000) + "' placeholder='5' " + String(!(config.serverActive1 || config.serverActive2 || config.serverActive3) ? "disabled" : "") + ">"
            "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
      "</div>"
      "<div class='row mb-3'>"
        "<label class='col-12 col-md-4 col-form-label'>APRS / MQTT</label>"
        "<div class='col-12 col-md-4 mb-3 mb-md-0'>"
          "<div class='input-group'>"
            "<input type='number' class='form-control' name='intervalAprs' value='" + String(config.intervalAprs / 60000) + "' placeholder='10' " + String(!config.activeAPRS ? "disabled" : "") + ">"
            "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
        "<div class='col-12 col-md-4'>"
          "<div class='input-group'>"
            "<input type='number' class='form-control' name='intervalMqtt' value='" + String(config.intervalMqtt / 60000) + "' placeholder='1' " + String(!config.activeMQTT ? "disabled" : "") + ">"
            "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-heart-pulse-fill'></i> HEARTBEAT</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeHeartbeat' name='activeHeartbeat' " + String(config.activeHeartbeat ? "checked" : "") + ">"
        "</div>"
      "</div>"
    "</section>";

  html +=
    "<section class='last'>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-bug-fill'></i> DEBUG</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='debugMode' name='debugMode' " + String(config.debugMode ? "checked" : "") + ">"
        "</div>"
      "</div>"
    "</section>";

  html += "</form></div></main>";
  html += buildFooter();
  return html;
}

String buildDebugPage() {
  String html = buildHead("WX Debug");
  html += buildNavbar("/debug", false);
  html += "<main class='page-content'><div class='container page-shell mx-auto py-4'>";
  html +=
    "<div class='panel'>"
      "<div class='d-flex justify-content-between align-items-center mb-3'>"
        "<h5 class='mb-0'><i class='bi bi-terminal'></i> Debug</h5>"
        "<div class='d-flex align-items-center gap-2'>"
          + formatBoolBadge(config.debugMode, "Live", "Debug off")
          + "<button type='button' class='btn btn-sm btn-outline-light' onclick='clearDebugLog()'>Clear log</button>"
        "</div>"
      + "</div>"
      "<div class='mini-note mb-3'>This page mirrors the serial debug output. Refresh is automatic every 2 seconds.</div>"
      "<div id='debug-log' class='form-control' style='height:60vh; overflow:auto; white-space:pre-wrap; font-family:monospace;'>"
        + logEscape(getDebugLogBuffer())
      + "</div>"
    "</div>";
  html += "</div></main>";
  html += buildDebugRefreshScript();
  html += buildFooter();
  return html;
}

}  // namespace

// ====== Handle upload ======
void handleUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (LittleFS.exists("/config.json")) {
      LittleFS.remove("/config.json");
    }
    uploadFile = LittleFS.open("/config.json", "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    loadConfig();
    Heartbeat::setEnabled(config.activeHeartbeat);
    RainGauge::onConfigurationChanged(config.activeRain, config.rainTipMm);
  }
}

// ====== Handle root page ======
void handleRoot() {
  server.send(200, "text/html", buildDashboardPage());
}

void handleSettings() {
  server.send(200, "text/html", buildSettingsPage());
}

void handleDebug() {
  server.send(200, "text/html", buildDebugPage());
}

void handleDebugLogs() {
  server.send(200, "text/html", logEscape(getDebugLogBuffer()));
}

void handleDebugClear() {
  clearDebugLogBuffer();
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  server.send(200, "application/json", buildStatusJson());
}

// ====== Handle save config ======
void handleSave() {
  config.debugMode       = server.hasArg("debugMode");
  config.activeHeartbeat = server.hasArg("activeHeartbeat");
  config.activeAPRS      = server.hasArg("activeAPRS");
  config.activeMQTT      = server.hasArg("activeMQTT");
  config.activeSYSLOG    = server.hasArg("activeSYSLOG");

  if (server.hasArg("stationName")) config.stationName = server.arg("stationName");
  if (server.hasArg("altitude")) config.altitude = server.arg("altitude").toFloat();

  config.activeLight = server.hasArg("activeLight");
  config.activeRain  = server.hasArg("activeRain");

  if (server.hasArg("dataTemp"))  config.dataTemp  = server.arg("dataTemp");
  if (server.hasArg("dataHumi"))  config.dataHumi  = server.arg("dataHumi");
  if (server.hasArg("dataPress")) config.dataPress = server.arg("dataPress");
  if (server.hasArg("dataLight")) config.dataLight = server.arg("dataLight");
  if (server.hasArg("dataRssi"))  config.dataRssi  = server.arg("dataRssi");

  if (server.hasArg("offsetTemp"))  config.offsetTemp  = server.arg("offsetTemp").toFloat();
  if (server.hasArg("offsetHumi"))  config.offsetHumi  = server.arg("offsetHumi").toFloat();
  if (server.hasArg("offsetPress")) config.offsetPress = server.arg("offsetPress").toFloat();
  if (server.hasArg("rainTipMm"))   config.rainTipMm   = server.arg("rainTipMm").toFloat();

  config.serverActive0 = server.hasArg("serverActive0");
  if (server.hasArg("serverUrl0")) config.serverUrl0 = server.arg("serverUrl0");
  if (server.hasArg("serverName0")) config.serverName0 = server.arg("serverName0");
  config.serverActive1 = server.hasArg("serverActive1");
  if (server.hasArg("serverUrl1")) config.serverUrl1 = server.arg("serverUrl1");
  if (server.hasArg("serverName1")) config.serverName1 = server.arg("serverName1");
  config.serverActive2 = server.hasArg("serverActive2");
  if (server.hasArg("serverUrl2")) config.serverUrl2 = server.arg("serverUrl2");
  if (server.hasArg("serverName2")) config.serverName2 = server.arg("serverName2");
  config.serverActive3 = server.hasArg("serverActive3");
  if (server.hasArg("serverUrl3")) config.serverUrl3 = server.arg("serverUrl3");
  if (server.hasArg("serverName3")) config.serverName3 = server.arg("serverName3");

  if (server.hasArg("aprsHost")) config.aprsHost = server.arg("aprsHost");
  if (server.hasArg("aprsPort")) config.aprsPort = server.arg("aprsPort").toInt();
  if (server.hasArg("aprsCall")) config.aprsCall = server.arg("aprsCall");
  if (server.hasArg("aprsPass")) config.aprsPass = server.arg("aprsPass");
  if (server.hasArg("aprsLat")) config.aprsLat = server.arg("aprsLat");
  if (server.hasArg("aprsLon")) config.aprsLon = server.arg("aprsLon");
  if (server.hasArg("aprsComment")) strncpy(config.aprsComment, server.arg("aprsComment").c_str(), sizeof(config.aprsComment) - 1);

  if (server.hasArg("mqttServer")) config.mqttServer = server.arg("mqttServer");
  if (server.hasArg("mqttPort")) config.mqttPort = server.arg("mqttPort").toInt();
  if (server.hasArg("mqttTopicPub1")) config.mqttTopicPub1 = server.arg("mqttTopicPub1");
  if (server.hasArg("mqttTopicPub2")) config.mqttTopicPub2 = server.arg("mqttTopicPub2");
  if (server.hasArg("mqttTopicSub1")) config.mqttTopicSub1 = server.arg("mqttTopicSub1");
  if (server.hasArg("mqttTopicSub2")) config.mqttTopicSub2 = server.arg("mqttTopicSub2");

  if (server.hasArg("syslogServer")) config.syslogServer = server.arg("syslogServer");
  if (server.hasArg("syslogPort")) config.syslogPort = server.arg("syslogPort").toInt();

  if (server.hasArg("intervalHttp")) config.intervalHttp = server.arg("intervalHttp").toInt() * 60000;
  if (server.hasArg("intervalAprs")) config.intervalAprs = server.arg("intervalAprs").toInt() * 60000;
  if (server.hasArg("intervalMqtt")) config.intervalMqtt = server.arg("intervalMqtt").toInt() * 60000;
  if (server.hasArg("restartMode")) config.restartMode = server.arg("restartMode").toInt();

  config.aprsComment[sizeof(config.aprsComment) - 1] = '\0';

  saveConfig();
  Heartbeat::setEnabled(config.activeHeartbeat);
  RainGauge::onConfigurationChanged(config.activeRain, config.rainTipMm);

  server.sendHeader("Location", "/setting?saved=1", true);
  server.send(303, "text/plain", "");
}

// ====== Setup web ======
void setupWeb() {
  server.on("/", handleRoot);
  server.on("/setting", handleSettings);
  server.on("/debug", handleDebug);
  server.on("/debug/logs", HTTP_GET, handleDebugLogs);
  server.on("/debug/clear", HTTP_POST, handleDebugClear);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/save", HTTP_POST, handleSave);

  server.on("/download", HTTP_GET, []() {
    if (LittleFS.exists("/config.json")) {
      File file = LittleFS.open("/config.json", "r");
      server.sendHeader("Content-Disposition", "attachment; filename=config.json");
      server.streamFile(file, "application/json");
      file.close();
    } else {
      server.send(404, "text/plain", "Config file not found");
    }
  });

  server.on(
    "/restore",
    HTTP_POST,
    []() {
      server.sendHeader("Location", "/setting?restored=1", true);
      server.send(303, "text/plain", "");
    },
    handleUpload
  );

  server.on("/wifi", HTTP_GET, []() {
    server.sendHeader("Location", "/setting", true);
    server.send(303, "text/plain", "");
    delay(500);
    startCaptivePortal();
  });

  server.on("/factory", HTTP_GET, []() {
    if (LittleFS.exists("/config.json")) {
      LittleFS.remove("/config.json");
    }
    RainGauge::reset();
    loadConfig();
    Heartbeat::setEnabled(config.activeHeartbeat);
    RainGauge::onConfigurationChanged(config.activeRain, config.rainTipMm);
    server.sendHeader("Location", "/setting?factory=1", true);
    server.send(303, "text/plain", "");
  });

  server.on("/reboot", HTTP_GET, []() {
    RainGauge::flush();
    server.send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });

  server.on("/config.json", HTTP_GET, []() {
    if (LittleFS.exists("/config.json")) {
      File file = LittleFS.open("/config.json", "r");
      server.streamFile(file, "application/json");
      file.close();
    } else {
      server.send(404, "text/plain", "Config file not found");
    }
  });

  server.begin();
}
