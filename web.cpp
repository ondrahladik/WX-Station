#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"
#include "web.h"

extern const char* programVers;
    
// WebServer instance on port 80
WebServer server(80);

File uploadFile;

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
  }
}

// ====== Handle root page ======
void handleRoot() {
  String html = 
    "<!DOCTYPE html><html lang='en'><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<link href='http://api.ok1kky.cz/wx/favicon.ico' rel='icon' type='image/x-icon'>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
    "<link href='https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css' rel='stylesheet'>"
    "<title>WX Station</title>"
    "<style>"
      "section { margin-bottom: 2rem; padding-bottom: 1rem; border-bottom: 1px solid #ddd; }"
      "section.last { margin-bottom: 0; padding-bottom: 0; border-bottom: none; }"
      "section h4 { color: #333; margin-bottom: 1rem; }"
      ".narrow-container { max-width: 800px; }"
    "</style>"
    "</head><body class='bg-light'>"

    // ===== Navbar =====
    "<nav class='navbar bg-body-secondary shadow-sm border-bottom sticky-top navbar-expand-lg'>"
      "<div class='container narrow-container mx-auto'>"
        "<a class='navbar-brand' href='/'>WX Station</a>"

        "<button class='navbar-toggler' type='button' data-bs-toggle='collapse' data-bs-target='#navbarNav' aria-controls='navbarNav' aria-expanded='false' aria-label='Toggle navigation'>"
          "<span class='navbar-toggler-icon'></span>"
        "</button>"

        "<div class='collapse navbar-collapse' id='navbarNav'>"
          "<ul class='navbar-nav me-auto mb-2 mb-lg-0'>"
            // Backup dropdown
            "<li class='nav-item dropdown'>"
              "<a class='nav-link dropdown-toggle' href='#' id='backupDropdown' role='button' data-bs-toggle='dropdown' aria-expanded='false'>"
                "Backup"
              "</a>"
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
              "<a class='nav-link dropdown-toggle' href='#' id='backupDropdown' role='button' data-bs-toggle='dropdown' aria-expanded='false'>"
                "Device"
              "</a>"
              "<ul class='dropdown-menu' aria-labelledby='backupDropdown'>"
                "<li><a class='dropdown-item' href='#' onclick='alert(\"After starting WiFi Manager, this page will be deactivated until the manager is closed.\"); fetch(\"/wifi\"); return false;'>WiFi Manager</a></li>"
                "<li><a class='dropdown-item' href='/factory' onclick=\"return confirm('Are you sure? All settings will be deleted.');\">Factory reset</a></li>"
                "<li><a class='dropdown-item' href='#' onclick='alert(\"ESP is restarting...\"); fetch(\"/reboot\"); return false;'>Reboot</a></li>"
              "</ul>"
            "</li>"

            // config.json link
            "<li class='nav-item'>"
              "<a class='nav-link' href='/config.json' target='_blank'>config.json</a>"
            "</li>"
          "</ul>"

          "<button class='btn btn-success ms-auto' type='submit' form='configForm'>Save</button>"
        "</div>"
      "</div>"
    "</nav>"

    "<div class='container py-4 pb-2 narrow-container mx-auto'>"
      "<form id='configForm' method='POST' action='/save'>";

  // ===== ALERT =====
  html +=
    "<div class='alert alert-info alert-dismissible fade show text-center mb-3' role='alert'>"
      "Before you start configuring your WX-Station, please read the instructions at "
      "<a href='https://github.com/ondrahladik/WX-Station/wiki' class='alert-link' target='_blank'>GitHub Wiki</a>."
    "</div>";


  // ===== STATION =====
  html +=
    "<section>"
      "<h5><i class='bi bi-person-circle'></i> STATION</h5>"

      "<div class='row mb-3'>"
        "<label class='col-4 col-form-label'>Name / ASL</label>"
        "<div class='col-4'>"
          "<input type='text' class='form-control' name='stationName' value='" + config.stationName + "' placeholder='wx-station'>"
        "</div>"
        "<div class='col-4'>"
          "<div class='input-group'>"
            "<input type='number' step='0.1' class='form-control' name='altitude' value='" + String(config.altitude, 1) + "' placeholder='230,0'>"
            "<span class='input-group-text'>m</span>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

    // ===== DATA =====
    html +=
      "<section>"
        "<h5><i class='bi bi-server'></i> DATA</h5>"

        // Temp / Offset
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Temp / Offset</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='dataTemp' value='" + config.dataTemp + "' placeholder='temperature'>"
          "</div>"
          "<div class='col-4'>"
            "<div class='input-group'>"
              "<input type='number' step='0.1' class='form-control' name='offsetTemp' value='" + String(config.offsetTemp, 1) + "' placeholder='0,0'>"
              "<span class='input-group-text'>°C</span>"
            "</div>"
          "</div>"
        "</div>"

        // Humi / Offset
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Humi / Offset</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='dataHumi' value='" + config.dataHumi + "' placeholder='humidity'>"
          "</div>"
          "<div class='col-4'>"
              "<div class='input-group'>"
                "<input type='number' step='0.1' class='form-control' name='offsetHumi' value='" + String(config.offsetHumi, 1) + "' placeholder='0,0'>"
                "<span class='input-group-text'>%</span>"
              "</div>"
          "</div>"
        "</div>"

        // Press / Offset
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Press / Offset</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='dataPress' value='" + config.dataPress + "' placeholder='pressure'>"
          "</div>"
          "<div class='col-4'>"
              "<div class='input-group'>"
                "<input type='number' step='0.1' class='form-control' name='offsetPress' value='" + String(config.offsetPress, 1) + "' placeholder='0,0'>"
                "<span class='input-group-text'>hPa</span>"
              "</div>"
          "</div>"
        "</div>"

        // Light
        "<div class='row mb-3'>"
          "<div class='col-4 d-flex align-items-center'>"
            "<label class='col-form-label me-2'>Light</label>"
             "<div class='form-check form-switch m-0'>"
              "<input class='form-check-input' type='checkbox' name='activeLight' " + String(config.activeLight ? "checked" : "") + ">"
            "</div>"
          "</div>"
          "<div class='col-8'>"
            "<input type='text' class='form-control' name='dataLight' value='" + config.dataLight + "' placeholder='light'>"
          "</div>"
        "</div>"

        // RSSI 
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>RSSI</label>"
          "<div class='col-8'>"
            "<input type='text' class='form-control' name='dataRssi' value='" + config.dataRssi + "' placeholder='rssi'>"
          "</div>"
        "</div>"

      "</section>";


  // ===== SERVER =====
  html +=
    "<section>"
      "<h5><i class='bi bi-hdd-stack-fill'></i> SERVER</h5>"

      "<div class='d-flex justify-content-between align-items-center mb-3'>"

        // Server 0
        "<div class='d-flex align-items-center'>"
          "<p class='mb-0 d-none d-sm-block'>Server <i class='bi bi-info-lg'></i></p>"
          "<p class='mb-0 d-block d-sm-none'>S <i class='bi bi-info-lg'></i></p>"
          "<div class='form-check form-switch ms-2 mb-0'>"
            "<input class='form-check-input' type='checkbox' id='serverActive0' name='serverActive0' "
            + String(config.serverActive0 ? "checked" : "") 
            + " onclick='document.getElementById(\"ser0Fields\").style.display=this.checked?\"block\":\"none\";'>"
            "<label class='form-check-label' for='serverActive0'></label>"
          "</div>"
        "</div>"

        // Server 1
        "<div class='d-flex align-items-center'>"
          "<p class='mb-0 d-none d-sm-block'>Server 1</p>"
          "<p class='mb-0 d-block d-sm-none'>S1</p>"
          "<div class='form-check form-switch ms-2 mb-0'>"
            "<input class='form-check-input' type='checkbox' id='serverActive1' name='serverActive1' "
            + String(config.serverActive1 ? "checked" : "") 
            + " onclick='document.getElementById(\"ser1Fields\").style.display=this.checked?\"block\":\"none\";'>"
            "<label class='form-check-label' for='serverActive1'></label>"
          "</div>"
        "</div>"

        // Server 2
        "<div class='d-flex align-items-center'>"
          "<p class='mb-0 d-none d-sm-block'>Server 2</p>"
          "<p class='mb-0 d-block d-sm-none'>S2</p>"
          "<div class='form-check form-switch ms-2 mb-0'>"
            "<input class='form-check-input' type='checkbox' id='serverActive2' name='serverActive2' "
            + String(config.serverActive2 ? "checked" : "") 
            + " onclick='document.getElementById(\"ser2Fields\").style.display=this.checked?\"block\":\"none\";'>"
            "<label class='form-check-label' for='serverActive2'></label>"
          "</div>"
        "</div>"

        // Server 3
        "<div class='d-flex align-items-center'>"
          "<p class='mb-0 d-none d-sm-block'>Server 3</p>"
          "<p class='mb-0 d-block d-sm-none'>S3</p>"
          "<div class='form-check form-switch ms-2 mb-0'>"
            "<input class='form-check-input' type='checkbox' id='serverActive3' name='serverActive3' "
            + String(config.serverActive3 ? "checked" : "") 
            + " onclick='document.getElementById(\"ser3Fields\").style.display=this.checked?\"block\":\"none\";'>"
            "<label class='form-check-label' for='serverActive3'></label>"
          "</div>"
        "</div>"

      "</div>"

      // Server 0 (URL / Name)
      "<div id='ser0Fields' style='display:" + String(config.serverActive0 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server <i class='bi bi-info-lg'></i></label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverUrl0' value='" + config.serverUrl0 + "' placeholder='http://example.com/'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverName0' value='" + config.serverName0 + "' placeholder='wx-station'>"
          "</div>"
        "</div>" 
      "</div>"

      // Server 1 (URL / Name)
      "<div id='ser1Fields' style='display:" + String(config.serverActive1 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server 1</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverUrl1' value='" + config.serverUrl1 + "' placeholder='http://example.com/'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverName1' value='" + config.serverName1 + "' placeholder='wx-station'>"
          "</div>"
        "</div>" 
      "</div>"

      // Server 2 (URL / Name)
      "<div id='ser2Fields' style='display:" + String(config.serverActive2 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server 2</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverUrl2' value='" + config.serverUrl2 + "' placeholder='http://example.com/'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverName2' value='" + config.serverName2 + "' placeholder='wx-station'>"
          "</div>"
        "</div>" 
      "</div>"

      // Server 3 (URL / Name)
      "<div id='ser3Fields' style='display:" + String(config.serverActive3 ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server 3</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverUrl3' value='" + config.serverUrl3 + "' placeholder='http://example.com/'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='serverName3' value='" + config.serverName3 + "' placeholder='wx-station'>"
          "</div>"
        "</div>" 
      "</div>"

    "</section>";

  // ===== APRS =====
  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-broadcast'></i> APRS</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeAPRS' name='activeAPRS' "
          + String(config.activeAPRS ? "checked" : "") 
          + " onclick='document.getElementById(\"aprsFields\").style.display=this.checked?\"block\":\"none\";'>"
          "<label class='form-check-label' for='activeAPRS'></label>"
        "</div>"
      "</div>"

      "<div id='aprsFields' style='display:" + String(config.activeAPRS ? "block" : "none") + ";'>"

        // Host / Port
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Host / Port</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='aprsHost' value='" + config.aprsHost + "' placeholder='euro.aprs2.net'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='number' class='form-control' name='aprsPort' value='" + String(config.aprsPort) + "' placeholder='14580'>"
          "</div>"
        "</div>"

        // Call / Pass
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Call / Pass</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='aprsCall' value='" + config.aprsCall + "' placeholder='NOCALL-13'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='aprsPass' value='" + config.aprsPass + "' placeholder='12345'>"
          "</div>"
        "</div>"

        // Lat / Long
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Lat / Lon</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='aprsLat' value='" + config.aprsLat + "' placeholder='0000.00N'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='aprsLon' value='" + config.aprsLon + "' placeholder='00000.00E'>"
          "</div>"
        "</div>"

        // Comment
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Comment</label>"
          "<div class='col-8'>"
            "<input type='text' class='form-control' name='aprsComment' value='" + String(config.aprsComment) + "' placeholder='WX-Station https://www.ok1kky.cz'>"
          "</div>"
        "</div>"

      "</div>" 
    "</section>";

  // ===== MQTT =====
  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-cloud-fill'></i> MQTT</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeMQTT' name='activeMQTT' "
          + String(config.activeMQTT ? "checked" : "")
          + " onclick='document.getElementById(\"mqttFields\").style.display=this.checked?\"block\":\"none\";'>"
          "<label class='form-check-label' for='activeMQTT'></label>"
        "</div>"
      "</div>"

      "<div id='mqttFields' style='display:" + String(config.activeMQTT ? "block" : "none") + ";'>"
        // MQTT Server / Port
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server / Port</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='mqttServer' value='" + config.mqttServer + "' placeholder='example.com'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='number' class='form-control' name='mqttPort' value='" + String(config.mqttPort) + "' placeholder='1883'>"
          "</div>"
        "</div>"

        // MQTT Pub Topic
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Pub topic</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='mqttTopicPub1' value='" + config.mqttTopicPub1 + "' placeholder='Pub topic 1'>"
          "</div>"
           "<div class='col-4'>"
            "<input type='text' class='form-control' name='mqttTopicPub2' value='" + config.mqttTopicPub2 + "' placeholder='Pub topic 2'>"
          "</div>"
        "</div>"

        // MQTT Sub Topics 
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Sub topic</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='mqttTopicSub1' value='" + config.mqttTopicSub1 + "' placeholder='Sub topic 2'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='mqttTopicSub2' value='" + config.mqttTopicSub2 + "' placeholder='Sub topic 2'>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

  // ===== SYSLOG =====
  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-file-earmark-text-fill'></i> SYSLOG</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='activeSYSLOG' name='activeSYSLOG' "
          + String(config.activeSYSLOG ? "checked" : "")
          + " onclick='document.getElementById(\"syslogFields\").style.display=this.checked?\"block\":\"none\";'>"
          "<label class='form-check-label' for='activeSYSLOG'></label>"
        "</div>"
      "</div>"
      
      "<div id='syslogFields' style='display:" + String(config.activeSYSLOG ? "block" : "none") + ";'>"
        "<div class='row mb-3'>"
          "<label class='col-4 col-form-label'>Server / Port</label>"
          "<div class='col-4'>"
            "<input type='text' class='form-control' name='syslogServer' value='" + config.syslogServer + "' placeholder='example.com'>"
          "</div>"
          "<div class='col-4'>"
            "<input type='number' class='form-control' name='syslogPort' value='" + String(config.syslogPort) + "' placeholder='514'>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

  // ===== INTERVAL =====
  html +=
    "<section>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-clock-fill'></i> INTERVAL</h5>"
      "</div>"
      
      "<div class='row mb-3'>"
        "<label class='col-4 col-form-label'>Reboot / Server</label>"
          "<div class='col-4'>"
            "<select class='form-control' name='restartMode'>"
              "<option value='0'" + String(config.restartMode == 0 ? " selected" : "") + ">Disable</option>"
              "<option value='1'" + String(config.restartMode == 1 ? " selected" : "") + ">6 hours</option>"
              "<option value='2'" + String(config.restartMode == 2 ? " selected" : "") + ">12 hours</option>"
              "<option value='3'" + String(config.restartMode == 3 ? " selected" : "") + ">24 hours</option>"
              "<option value='4'" + String(config.restartMode == 4 ? " selected" : "") + ">48 hours</option>"
            "</select>"
          "</div>"
        "<div class='col-4'>"
          "<div class='input-group'>"
              "<input type='number' class='form-control' name='intervalHttp' value='" + String(config.intervalHttp / 60000) + "' placeholder='5' " 
          + (!(config.serverActive1 || config.serverActive2 || config.serverActive3) ? "disabled" : "") + ">"
              "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
      "</div>"

      "<div class='row mb-3'>"
        "<label class='col-4 col-form-label'>APRS / MQTT</label>"
        "<div class='col-4'>"
          "<div class='input-group'>"
            "<input type='number' class='form-control' name='intervalAprs' value='" + String(config.intervalAprs / 60000) + "' placeholder='10' " + (!config.activeAPRS ? "disabled" : "") + ">"
            "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
        "<div class='col-4'>"
          "<div class='input-group'>"
            "<input type='number' class='form-control' name='intervalMqtt' value='" + String(config.intervalMqtt / 60000) + "' placeholder='1' " + (!config.activeMQTT ? "disabled" : "") + ">"
            "<span class='input-group-text'>min</span>"
          "</div>"
        "</div>"
      "</div>"
    "</section>";

  // ===== DEBUG =====
  html +=
    "<section class='last'>"
      "<div class='d-flex align-items-center justify-content-between mb-3'>"
        "<h5><i class='bi bi-bug-fill'></i> DEBUG</h5>"
        "<div class='form-check form-switch mb-0'>"
          "<input class='form-check-input' type='checkbox' id='debugMode' name='debugMode' "
          + String(config.debugMode ? "checked" : "") + ">"
          "<label class='form-check-label' for='debugMode'></label>"
        "</div>"
      "</div>"
    "</section>"
    "</form></div>";

  html +=
    "<footer class='bg-light text-center text-muted py-2 mt-3 border-top small'>"
      "Made with ❤️ by <a href='https://www.ok1kky.cz' target='_blank'>OK1KKY</a> | "
      "WX-Station " + String(programVers) +
    "</footer>";

  html +=
    "<script src='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js'></script>"
    "</body></html>";

  server.send(200, "text/html", html);
}

// ====== Handle save config ======
void handleSave() {
  config.debugMode  = server.hasArg("debugMode");
  config.activeAPRS = server.hasArg("activeAPRS");
  config.activeMQTT = server.hasArg("activeMQTT");
  config.activeSYSLOG = server.hasArg("activeSYSLOG");

  // ===== STATION =====
  if (server.hasArg("stationName"))  config.stationName  = server.arg("stationName");
  if (server.hasArg("altitude"))  config.altitude  = server.arg("altitude").toFloat();

  // ===== DATA =====
  config.activeLight = server.hasArg("activeLight");

  if (server.hasArg("dataTemp"))  config.dataTemp  = server.arg("dataTemp");
  if (server.hasArg("dataHumi"))  config.dataHumi  = server.arg("dataHumi");
  if (server.hasArg("dataPress")) config.dataPress = server.arg("dataPress");
  if (server.hasArg("dataLight")) config.dataLight = server.arg("dataLight");
  if (server.hasArg("dataRssi"))  config.dataRssi  = server.arg("dataRssi");

  if (server.hasArg("offsetTemp"))  config.offsetTemp  = server.arg("offsetTemp").toFloat();
  if (server.hasArg("offsetHumi"))  config.offsetHumi  = server.arg("offsetHumi").toFloat();
  if (server.hasArg("offsetPress")) config.offsetPress = server.arg("offsetPress").toFloat();

  // ===== SERVER =====
  config.serverActive0  = server.hasArg("serverActive0");
  if (server.hasArg("serverUrl0"))   config.serverUrl0   = server.arg("serverUrl0");
  if (server.hasArg("serverName0"))  config.serverName0  = server.arg("serverName0");
  config.serverActive1  = server.hasArg("serverActive1");
  if (server.hasArg("serverUrl1"))   config.serverUrl1   = server.arg("serverUrl1");
  if (server.hasArg("serverName1"))  config.serverName1  = server.arg("serverName1");
  config.serverActive2  = server.hasArg("serverActive2");
  if (server.hasArg("serverUrl2"))   config.serverUrl2   = server.arg("serverUrl2");
  if (server.hasArg("serverName2"))  config.serverName2  = server.arg("serverName2");
  config.serverActive3  = server.hasArg("serverActive3");
  if (server.hasArg("serverUrl3"))   config.serverUrl3   = server.arg("serverUrl3");
  if (server.hasArg("serverName3"))  config.serverName3  = server.arg("serverName3");

  // ===== APRS =====
  if (server.hasArg("aprsHost"))    config.aprsHost    = server.arg("aprsHost");
  if (server.hasArg("aprsPort"))    config.aprsPort    = server.arg("aprsPort").toInt();
  if (server.hasArg("aprsCall"))    config.aprsCall    = server.arg("aprsCall");
  if (server.hasArg("aprsPass"))    config.aprsPass    = server.arg("aprsPass");
  if (server.hasArg("aprsLat"))     config.aprsLat     = server.arg("aprsLat");
  if (server.hasArg("aprsLon"))     config.aprsLon     = server.arg("aprsLon");
  if (server.hasArg("aprsComment")) strncpy(config.aprsComment, server.arg("aprsComment").c_str(), sizeof(config.aprsComment) - 1);

  // ===== MQTT =====
  if (server.hasArg("mqttServer"))     config.mqttServer    = server.arg("mqttServer");
  if (server.hasArg("mqttPort"))       config.mqttPort      = server.arg("mqttPort").toInt();
  if (server.hasArg("mqttTopicPub1"))  config.mqttTopicPub1 = server.arg("mqttTopicPub1");
  if (server.hasArg("mqttTopicPub2"))  config.mqttTopicPub2 = server.arg("mqttTopicPub2");
  if (server.hasArg("mqttTopicSub1"))  config.mqttTopicSub1 = server.arg("mqttTopicSub1");
  if (server.hasArg("mqttTopicSub2"))  config.mqttTopicSub2 = server.arg("mqttTopicSub2");

  // ===== SYSLOG =====
  if (server.hasArg("syslogServer")) config.syslogServer = server.arg("syslogServer");
  if (server.hasArg("syslogPort"))   config.syslogPort   = server.arg("syslogPort").toInt();

  // ===== INTERVAL =====
  if (server.hasArg("intervalHttp"))   config.intervalHttp   = server.arg("intervalHttp").toInt() * 60000;
  if (server.hasArg("intervalAprs"))   config.intervalAprs   = server.arg("intervalAprs").toInt() * 60000;
  if (server.hasArg("intervalMqtt"))   config.intervalMqtt   = server.arg("intervalMqtt").toInt() * 60000;
  if (server.hasArg("restartMode"))    config.restartMode    = server.arg("restartMode").toInt();

  // ensure null-termination for aprsComment
  config.aprsComment[sizeof(config.aprsComment) - 1] = '\0';

  // save config to file
  saveConfig();

  // Redirect back to root
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

// ====== Setup web ======
void setupWeb() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  // ====== Download config.json ======
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

  // ====== Restore config.json ======
  server.on(
    "/restore",
    HTTP_POST,
    []() {
      server.sendHeader("Location", "/", true);
      server.send(303, "text/plain", "");
    },
    handleUpload  
  );

  // ====== WiFi Manager ======
  server.on("/wifi", HTTP_GET, []() {
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "");
    delay(500);
    startCaptivePortal();
  });

  // ====== Factory reset ======
  server.on("/factory", HTTP_GET, []() {
      if (LittleFS.exists("/config.json")) {
          LittleFS.remove("/config.json"); 
      }
      loadConfig();
      server.sendHeader("Location", "/", true);
      server.send(303, "text/plain", "");
  });

  // ====== Reboot ESP ======
  server.on("/reboot", HTTP_GET, []() {
    server.send(200, "text/plain", "Rebooting...");
    delay(500);
    ESP.restart();
  });
  
  // ====== GET config.json ======
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