#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h> 
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <BH1750.h>
#include "config.h"
#include "web.h"

const char* programName = "WX-Station";
const char* programVers = "v1.0.1";

WiFiUDP udp;
WiFiManager wm;
WiFiClient client; 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ====== Global variables ======
bool mqttNoWiFiReported = false;
bool bmeOK = false;
bool lightOK = false;

unsigned long lastSensorRead = 0;
unsigned long lastHttpSend = 0;
unsigned long lastAprsSend = 0;
unsigned long lastMQTTSend = 0;
unsigned long lastRestart = 0;
unsigned long restartIntervalMs = 0;
unsigned long intervalSensor = 30000;
unsigned long lastMQTTReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 10000; 
float temperature, humidity, pressure, seaLevelPressure;
float lightLux, lightWm2;
int rssi;

Adafruit_BME280 bme;
BH1750 lightSensor;  

// ====== Functions ======
void debugPrint(const String& msg, bool newline = false) {
  if (!config.debugMode) return;

  if (newline) Serial.println(msg);
  else Serial.print(msg);
}

void debugPrint(const char* msg, bool newline = false) {
  if (!config.debugMode) return;

  if (newline) Serial.println(msg);
  else Serial.print(msg);
}

void debugPrintln() {
  if (config.debugMode) Serial.println();
}

void logToSyslog(const char* message) {
  if (!config.activeSYSLOG) return;

  String syslogMessage = "<134>";
  
  syslogMessage += config.stationName;
  syslogMessage += ": ";
  syslogMessage += message;

  // Send via UDP
  udp.beginPacket(config.syslogServer.c_str(), config.syslogPort); 
  udp.write((const uint8_t*)syslogMessage.c_str(), syslogMessage.length());
  udp.endPacket();
}

void welcomeMessage() {
  String line = String(programName) + " " + programVers + " | Local IP: " + WiFi.localIP().toString();
  int len = line.length();

  debugPrintln();
  logToSyslog(" ");

  String lineBuffer = "";
  for (int i = 0; i < len; i++) {
      lineBuffer += "#";
  }

  debugPrint(lineBuffer);
  logToSyslog(lineBuffer.c_str()); 
  debugPrintln();

  debugPrint(line, true);
  logToSyslog(line.c_str());

  lineBuffer = "";
  for (int i = 0; i < len; i++) {
      lineBuffer += "#";
  }

  debugPrint(lineBuffer);
  logToSyslog(lineBuffer.c_str()); 
  debugPrint("", true);
  debugPrintln();
  logToSyslog(" ");
}

void restartInterval() {
  switch (config.restartMode) {
    case 1:  restartIntervalMs = 6UL  * 60UL * 60UL * 1000UL; break;
    case 2:  restartIntervalMs = 12UL * 60UL * 60UL * 1000UL; break;
    case 3:  restartIntervalMs = 24UL * 60UL * 60UL * 1000UL; break;
    case 4:  restartIntervalMs = 48UL * 60UL * 60UL * 1000UL; break;
    case 0:  restartIntervalMs = 0; break;
  }
}

void startCaptivePortal() {
  debugPrint("Web server turned off", true);
  logToSyslog("Web server turned off");
  server.stop();  

  wm.setConnectRetries(3);        
  wm.setConfigPortalTimeout(600);   
  wm.setTitle("WX Station");
  wm.startConfigPortal("WX-StationAP");  

  server.begin();
  debugPrint("Web server turned on", true);
  logToSyslog("Web server turned on");
}

void reconnectWiFi() {
  static bool reconnecting = false;
  static unsigned long reconnectStart = 0;
  static int failedAttempts = 0; 
  const unsigned long reconnectInterval = 10000;

  if (WiFi.status() != WL_CONNECTED) {
    if (!reconnecting) {
      debugPrint("WiFi | Lost connection, starting reconnect...", true);
      logToSyslog("WiFi | Lost connection, starting reconnect...");
      reconnecting = true;
      reconnectStart = millis();
      failedAttempts = 0; 
    } else {
      if (millis() - reconnectStart >= reconnectInterval) {
        debugPrint("WiFi | Attempting reconnect...", true);
        logToSyslog("WiFi | Attempting reconnect...");
        WiFi.reconnect();
        reconnectStart = millis();
        failedAttempts++;

        if (failedAttempts >= 9) {   
          debugPrint("REST | Reconnect failed too many times -> Restarting...", true);
          logToSyslog("REST | Reconnect failed too many times -> Restarting...");
          ESP.restart();
        }
      }
    }
  } else {
    if (reconnecting) {
      debugPrint("WiFi | Reconnected!", true);
      logToSyslog("WiFi | Reconnected!");
      reconnecting = false;
      failedAttempts = 0;
    }
  }
}

void readSensorData() {
  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  if (isnan(temp) || isnan(hum) || isnan(pres)) {
    debugPrint("Sensor read error!", true);
    logToSyslog("Sensor read error!");
    return;
  }

  float seaLevel = pres / pow(1.0 - (config.altitude / 44330.0), 5.255);

  temperature = temp + config.offsetTemp;
  humidity    = hum + config.offsetHumi;
  pressure    = pres + config.offsetPress;
  seaLevelPressure = seaLevel + config.offsetPress; 
  rssi = WiFi.RSSI();
}

void readLightSensor() {
    float lux = lightSensor.readLightLevel();

    if (lux < 0 || isnan(lux)) {
        debugPrint("Light sensor read error!", true);
        logToSyslog("Light sensor read error!");
        return;
    }

    lightLux = lux;
    lightWm2 = lux * 0.0079;
}

void sendInfoToDB() {
  if (!config.serverActive0) return;

  String localIP = WiFi.localIP().toString();
  String publicIP = "unknown";

  HTTPClient http;
  http.begin("http://api.ipify.org");
  int httpCode = http.GET();
  if (httpCode == 200) {
    publicIP = http.getString();
    publicIP.trim();
  }
  http.end();

  String url = config.serverUrl0;
  if (config.serverName0.length() > 0) { 
      url += "?station=";
      url += config.serverName0;
      url += "&version=";
      url += programVers;
  } else {
      url += "?version=";
      url += programVers;
  }
  url += "&loc-ip=" + localIP;
  url += "&pub-ip=" + publicIP;

  http.begin(url);
  int response = http.GET();
  if (response > 0) {
    String msg = "INFO | SENT OK | HTTP " + String(response) + " | URL: " + url;
    debugPrint(msg, true);
    logToSyslog(msg.c_str());
  } else {
    String msg = "INFO | SENT KO | HTTP " + String(response) + " | URL: " + url;
    debugPrint(msg, true);
    logToSyslog(msg.c_str());
  }
  http.end();
}

void sendDataToDB() {
  // Server 1
  if (config.serverActive1) {
      String url = config.serverUrl1 + "?";
      
      bool firstParam = true;

      if (config.serverName1.length() > 0) {
          url += "station=" + config.serverName1;
          firstParam = false;
      }

      url += (firstParam ? "" : "&") + String(config.dataTemp) + "=" + String(temperature, 2);
      firstParam = false;
      url += "&" + String(config.dataHumi) + "=" + String(humidity, 2);
      url += "&" + String(config.dataPress) + "=" + String(seaLevelPressure, 2);
      if (config.activeLight) {
        url += "&" + String(config.dataLight) + "=" + String(lightWm2, 2);
      }
      url += "&" + String(config.dataRssi) + "=" + String(rssi);

      HTTPClient http;
      http.begin(url);
      int httpResponseCode = http.GET();

      String msg;

      if (httpResponseCode > 0) {
          msg = "SVR1 | SENT OK | HTTP " + String(httpResponseCode) + " | URL: " + url;
      } else {
          msg = "SVR1 | SENT KO | HTTP " + String(httpResponseCode) + " | URL: " + url;
      }

      debugPrint(msg, true);
      logToSyslog(msg.c_str());

      http.end();
  }

  // Server 2
  if (config.serverActive2) {
      String url = config.serverUrl2 + "?";
      bool firstParam = true;

      if (config.serverName2.length() > 0) {
          url += "station=" + config.serverName2;
          firstParam = false;
      }

      url += (firstParam ? "" : "&") + String(config.dataTemp) + "=" + String(temperature, 2);
      firstParam = false;
      url += "&" + String(config.dataHumi) + "=" + String(humidity, 2);
      url += "&" + String(config.dataPress) + "=" + String(seaLevelPressure, 2);
      if (config.activeLight) {
        url += "&" + String(config.dataLight) + "=" + String(lightWm2, 2);
      }
      url += "&" + String(config.dataRssi) + "=" + String(rssi);

      HTTPClient http;
      http.begin(url);
      int httpResponseCode = http.GET();

      String msg;
      if (httpResponseCode > 0) {
          msg = "SVR2 | SENT OK | HTTP " + String(httpResponseCode) + " | URL: " + url;
      } else {
          msg = "SVR2 | SENT KO | HTTP " + String(httpResponseCode) + " | URL: " + url;
      }

      debugPrint(msg, true);
      logToSyslog(msg.c_str());

      http.end();
  }

  // Server 3
  if (config.serverActive3) {
      String url = config.serverUrl3 + "?";
      bool firstParam = true;

      if (config.serverName3.length() > 0) {
          url += "station=" + config.serverName3;
          firstParam = false;
      }

      url += (firstParam ? "" : "&") + String(config.dataTemp) + "=" + String(temperature, 2);
      firstParam = false;
      url += "&" + String(config.dataHumi) + "=" + String(humidity, 2);
      url += "&" + String(config.dataPress) + "=" + String(seaLevelPressure, 2);
      if (config.activeLight) {
        url += "&" + String(config.dataLight) + "=" + String(lightWm2, 2);
      }
      url += "&" + String(config.dataRssi) + "=" + String(rssi);

      HTTPClient http;
      http.begin(url);
      int httpResponseCode = http.GET();

      String msg;
      if (httpResponseCode > 0) {
          msg = "SVR3 | SENT OK | HTTP " + String(httpResponseCode) + " | URL: " + url;
      } else {
          msg = "SVR3 | SENT KO | HTTP " + String(httpResponseCode) + " | URL: " + url;
      }

      debugPrint(msg, true);
      logToSyslog(msg.c_str());

      http.end();
  }
}

void sendDataToAPRS() {
  if (!config.activeAPRS) return;

  WiFiClient client;
  debugPrint(String("APRS | Connecting to ") + config.aprsHost + ":" + String(config.aprsPort));
  String msg = String("APRS | Connecting to ") + config.aprsHost + ":" + String(config.aprsPort);

  if (client.connect(config.aprsHost.c_str(), (uint16_t)config.aprsPort)) {
    debugPrint(" -> Connected", true);
    logToSyslog((msg + " -> Connected").c_str());

    // Login to APRS-IS
    char login[80];
    sprintf(login, "user %s pass %s vers WX_ESP32 0.1 filter m/1", config.aprsCall, config.aprsPass);
    client.println(login);

    // Temperature for APRS must be in °F
    float temperatureF = (temperature * 1.8) + 32;

    char sentence[180];

    const char* format =
        "%s>APRS,TCPIP*:@%02d%02d%02dz%s/%s_.../...t%03dh%02db%05d%s%s";

    char lightPart[10] = "";
    if (config.activeLight) {
        snprintf(lightPart, sizeof(lightPart), "L%03d", (int)lightWm2);
    }

    snprintf(sentence, sizeof(sentence), format,
            config.aprsCall,
            0, 0, 0,
            config.aprsLat,
            config.aprsLon,
            (int)temperatureF,
            (int)humidity,
            (int)(seaLevelPressure * 10),
            lightPart,
            config.aprsComment
    );

    // Sending
    client.println(sentence);
    debugPrint(String("APRS | SENT OK | ") + sentence, true);
    logToSyslog((String("APRS | SENT OK | ") + sentence).c_str());

    client.stop();
  } else {
    debugPrint("APRS | SENT KO", true);
    logToSyslog("APRS | SENT KO");
  }
}

void runningMQTT() {
  if (mqttClient.connected()) {
    mqttClient.loop(); 
    mqttNoWiFiReported = false; 
    return;
  }

  unsigned long now = millis();
  if (now - lastMQTTReconnectAttempt > mqttReconnectInterval) {
    lastMQTTReconnectAttempt = now;
    debugPrint("MQTT | Attempting reconnect...", true);
    logToSyslog("MQTT | Attempting reconnect...");
    if (mqttClient.connect(config.stationName.c_str())) {
      debugPrint("MQTT | Reconnected!", true);
      logToSyslog("MQTT | Reconnected!");
      if (config.mqttTopicSub1.length() > 0) {
        mqttClient.subscribe(config.mqttTopicSub1.c_str());
      }
      if (config.mqttTopicSub2.length() > 0) {
        mqttClient.subscribe(config.mqttTopicSub2.c_str());
      }
      mqttNoWiFiReported = false;
    } else {
      debugPrint("MQTT | Failed rc=" + String(mqttClient.state()), true);
      logToSyslog((String("MQTT | Failed rc=") + String(mqttClient.state())).c_str());
    }
  }
}

void publishToMQTT() {
  if (!config.activeMQTT) return;

  if (!mqttClient.connected()) {
    debugPrint("MQTT | Not connected, skipping publish", true);
    logToSyslog("MQTT | Not connected, skipping publish");
    return;
  }

  StaticJsonDocument<200> jsonDoc;
  jsonDoc[config.dataTemp]  = roundf(temperature * 100) / 100.0;
  jsonDoc[config.dataHumi]  = roundf(humidity * 100) / 100.0;
  jsonDoc[config.dataPress] = roundf(seaLevelPressure * 100) / 100.0;
  if (config.activeLight) {
      jsonDoc[config.dataLight] = roundf(lightWm2 * 100) / 100.0;
  }
  jsonDoc[config.dataRssi]  = rssi;

  char jsonBuffer[256];
  serializeJson(jsonDoc, jsonBuffer);

  if (config.mqttTopicPub1.length() > 0) {
    if (mqttClient.publish(config.mqttTopicPub1.c_str(), jsonBuffer)) {
      debugPrint("MQTT | SENT OK | " + String(jsonBuffer), true);
      logToSyslog((String("MQTT | SENT OK | ") + String(jsonBuffer)).c_str());
    } else {
      debugPrint("MQTT | SENT KO", true);
      logToSyslog("MQTT | SENT KO");
    }
  } else {
    debugPrint("MQTT | Publish topic is empty, skipping", true);
    logToSyslog("MQTT | Publish topic is empty, skipping");
  }
}

void subscribeMQTT(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();

  // ======= Command =======
  if (message.equalsIgnoreCase("reboot")) {
    debugPrint("MQTT | RECV OK | Command RESET -> Restarting ESP...", true);
    logToSyslog("MQTT | RECV OK | Command RESET -> Restarting ESP...");
    ESP.restart();
  } 
  else if (message.equalsIgnoreCase("start-ap")) {
    debugPrint("MQTT | RECV OK | Command START-AP -> Captive portal started...", true);
    logToSyslog("MQTT | RECV OK | Command START-AP -> Captive portal started...");
    startCaptivePortal();
  }
  else if (message.equalsIgnoreCase("info")) {
    debugPrint("MQTT | RECV OK | Command INFO -> Sending info...", true);
    logToSyslog("MQTT | RECV OK | Command INFO -> Sending info...");
    sendInfoToDB();
  }
  // ======= OTA Update =======
  else if (message.startsWith("update(") && message.endsWith(")")) {
    String url = message.substring(7, message.length() - 1);
    debugPrint("MQTT | RECV OK | Command UPDATE -> URL: " + url, true);
    logToSyslog((String("MQTT | RECV OK | Command UPDATE -> URL: ") + url).c_str());

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      http.begin(url);       
      http.setTimeout(30000); 

      t_httpUpdate_return ret = httpUpdate.update(http); 

      switch(ret) {
        case HTTP_UPDATE_FAILED:
          debugPrint(" OTA | UPDATE FAILED | " + String(httpUpdate.getLastError()) + ": " + httpUpdate.getLastErrorString(), true);
          logToSyslog((String(" OTA | UPDATE FAILED | ") + String(httpUpdate.getLastError()) + ": " + httpUpdate.getLastErrorString()).c_str());
          break;
        case HTTP_UPDATE_NO_UPDATES:
          debugPrint(" OTA | No updates available", true);
          logToSyslog(" OTA | No updates available");
          break;
        case HTTP_UPDATE_OK:
          debugPrint(" OTA | UPDATE OK | Restarting...", true);
          logToSyslog(" OTA | UPDATE OK | Restarting...");
          delay(1000);
          ESP.restart(); 
          break;
      }
      http.end(); 
    }
  }
}

// ====== Setup ======
void setup() {
  Serial.begin(115200);

  WiFi.setHostname("WX-Station");
  wm.setConnectRetries(3);
  wm.setConfigPortalTimeout(600);
  wm.setTitle("WX Station");

  if (!wm.autoConnect("WX-StationAP")) {
    debugPrint("REST | Failed to connect, restarting...", true);
    logToSyslog("REST | Failed to connect, restarting...");
    ESP.restart();
  }

  loadConfig();

  // Init BME280
  for (int i = 0; i < 5; i++) {  
      if (bme.begin(0x76)) {
          bmeOK = true;
          break;
      }
      debugPrint("BME280 not found, retrying...", true);
      delay(1000);
  }

  if (!bmeOK) {
      debugPrint("BME280 unavailable, continuing without it!", true);
      logToSyslog("BME280 unavailable, continuing without it!");
  }

  // Init BH1750 
  if (config.activeLight) {
      for (int i = 0; i < 5; i++) {  
          if (lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
              lightOK = true;
              break;
          }
          debugPrint("BH1750 not found, retrying...", true);
          delay(1000);
      }

      if (!lightOK) {
          debugPrint("BH1750 unavailable, continuing without it!", true);
          logToSyslog("BH1750 unavailable, continuing without it!");
      }
  }

  setupWeb();

  // MQTT setup
  mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
  mqttClient.setCallback(subscribeMQTT);   

  // Subscribe for control
  if (mqttClient.connect(config.stationName.c_str())) {
      if (config.mqttTopicSub1.length() > 0) {
          mqttClient.subscribe(config.mqttTopicSub1.c_str());
      }
      if (config.mqttTopicSub2.length() > 0) {
          mqttClient.subscribe(config.mqttTopicSub2.c_str());
      }
  }

  welcomeMessage();

  if (!LittleFS.begin()) {
    debugPrint("SYST | LittleFS mount failed! Trying to format...", true);
    logToSyslog("SYST | LittleFS mount failed! Trying to format...");
    if (LittleFS.begin(true)) {
      debugPrint("SYST | LittleFS formatted and mounted successfully.", true);
      logToSyslog("SYST | LittleFS formatted and mounted successfully.");
    } else {
      debugPrint("SYST | LittleFS mount failed, even after format!", true);
      logToSyslog("SYST | LittleFS mount failed, even after format!");
    }
  }

  sendInfoToDB();
  restartInterval();
  readSensorData();
  publishToMQTT();
  sendDataToDB();
  sendDataToAPRS();
}

// ====== Loop ======
void loop() {
  unsigned long now = millis();

  // Periodic restart
  if (restartIntervalMs > 0 && (now - lastRestart >= restartIntervalMs)) {
    debugPrint("REST | Periodic restart...", true);
    logToSyslog("REST | Periodic restart...");
    delay(1000);
    ESP.restart();
  }

  // Wi-Fi watchdog 
  reconnectWiFi(); 

  // Read sensor
  if (now - lastSensorRead >= intervalSensor) {
      lastSensorRead = now;

      if (WiFi.status() == WL_CONNECTED) {
          readSensorData();   // BME280

          if (config.activeLight) {
              readLightSensor();  // BH1750
          }
      }
  }

  // HTTP 
  if (now - lastHttpSend >= config.intervalHttp) {
    lastHttpSend = now;
    if (WiFi.status() == WL_CONNECTED) {
      sendDataToDB();  
    }
  }

  // APRS
  if (now - lastAprsSend >= config.intervalAprs) {
    lastAprsSend = now;
    if (WiFi.status() == WL_CONNECTED) {
      sendDataToAPRS();
    }
  }

  // MQTT
  if (now - lastMQTTSend >= config.intervalMqtt) {
    lastMQTTSend = now;
    if (WiFi.status() == WL_CONNECTED) {
      publishToMQTT();
    }
  }

  runningMQTT(); 
  server.handleClient();
}
