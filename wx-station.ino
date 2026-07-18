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
#include <ESPmDNS.h>
#include <BH1750.h>
#include <time.h>
#include <Wire.h>
#include "config.h"
#include "heartbeat.h"
#include "rain.h"
#include "web.h"

const char* programName = "WX-Station";
const char* programVers = "v1.0.6";
const char* localHostname = "wx";

WiFiUDP udp;
WiFiManager wm;
WiFiClient client; 
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ====== Global variables ======
bool mqttNoWiFiReported = false;
bool bmeOK = false;
bool lightOK = false;
bool setupCompleted = false;
bool accessPointModeActive = false;
bool fatalErrorActive = false;
bool runtimeSensorFaultActive = false;
uint8_t bmeReadErrorCount = 0;
uint8_t lightReadErrorCount = 0;

unsigned long lastSensorRead = 0;
unsigned long lastHttpSend = 0;
unsigned long lastAprsSend = 0;
unsigned long lastMQTTSend = 0;
unsigned long lastRestart = 0;
unsigned long restartIntervalMs = 0;
unsigned long intervalSensor = 30000;
unsigned long lastMQTTReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 10000; 
const uint8_t maxConsecutiveBmeReadErrors = 5;
const uint8_t maxConsecutiveLightReadErrors = 5;
const unsigned long sensorRecoveryIntervalMs = 5000;
const unsigned long ntpResyncIntervalMs = 6UL * 60UL * 60UL * 1000UL;
const uint8_t bmeI2cAddress = 0x76;
const uint8_t bh1750PrimaryAddress = 0x23;
const uint8_t bh1750SecondaryAddress = 0x5C;
float temperature, humidity, pressure, seaLevelPressure;
float lightLux, lightWm2;
int rssi;

Adafruit_BME280 bme;
BH1750 lightSensor;  
uint8_t activeBh1750Address = bh1750PrimaryAddress;
unsigned long lastNtpSyncAttempt = 0;
bool clockSynchronized = false;
String debugLogBuffer;
const size_t maxDebugLogBufferLength = 12000;

const char* ntpServerPrimary = "pool.ntp.org";
const char* ntpServerSecondary = "time.google.com";
const char* ntpServerTertiary = "time.cloudflare.com";

void debugPrint(const String& msg, bool newline);
void debugPrint(const char* msg, bool newline);
void logToSyslog(const char* message);
void appendDebugLog(const String& msg, bool newline);
bool initBME280(uint8_t attempts = 5, bool waitBetweenAttempts = true);
bool initBH1750(uint8_t attempts = 5, bool waitBetweenAttempts = true);
bool isI2CDeviceResponsive(uint8_t address);
bool isBME280Responsive();
bool isBH1750Responsive();
void tryRecoverSensors();
bool synchronizeClock(bool waitForSync = true);
void startMDNSService();

void refreshHeartbeatState() {
  if (fatalErrorActive || runtimeSensorFaultActive) {
    Heartbeat::setState(Heartbeat::State::Error);
  } else if (accessPointModeActive) {
    Heartbeat::setState(Heartbeat::State::AccessPoint);
  } else if (setupCompleted) {
    Heartbeat::setState(Heartbeat::State::Normal);
  } else {
    Heartbeat::setState(Heartbeat::State::Booting);
  }
}

void setAccessPointMode(bool active) {
  accessPointModeActive = active;
  refreshHeartbeatState();
}

void setFatalError(const char* message) {
  if (!fatalErrorActive) {
    debugPrint(message, true);
    logToSyslog(message);
  }

  fatalErrorActive = true;
  refreshHeartbeatState();
}

void setRuntimeSensorFault(const char* message) {
  if (!runtimeSensorFaultActive) {
    debugPrint(message, true);
    logToSyslog(message);
  }

  runtimeSensorFaultActive = true;
  bmeOK = false;
  lightOK = false;
  refreshHeartbeatState();
}

void clearRuntimeSensorFault() {
  if (!runtimeSensorFaultActive) {
    return;
  }

  runtimeSensorFaultActive = false;
  bmeReadErrorCount = 0;
  lightReadErrorCount = 0;
  debugPrint("SENS | Sensor communication restored, resuming station.", true);
  logToSyslog("SENS | Sensor communication restored, resuming station.");
  refreshHeartbeatState();
}

void onConfigPortalStarted(WiFiManager* wifiManager) {
  (void)wifiManager;
  setAccessPointMode(true);
}

void appendDebugLog(const String& msg, bool newline) {
  if (!config.debugMode) return;

  debugLogBuffer += msg;
  if (newline || debugLogBuffer.length() == 0 || debugLogBuffer[debugLogBuffer.length() - 1] != '\n') {
    debugLogBuffer += "\n";
  }

  if (debugLogBuffer.length() > maxDebugLogBufferLength) {
    debugLogBuffer.remove(0, debugLogBuffer.length() - maxDebugLogBufferLength);
  }
}

String getDebugLogBuffer() {
  return debugLogBuffer;
}

void clearDebugLogBuffer() {
  debugLogBuffer = "";
}

// ====== Functions ======
void debugPrint(const String& msg, bool newline = false) {
  if (!config.debugMode) return;

  appendDebugLog(msg, true);
  if (newline) Serial.println(msg);
  else Serial.print(msg);
}

void debugPrint(const char* msg, bool newline = false) {
  if (!config.debugMode) return;

  appendDebugLog(String(msg), true);
  if (newline) Serial.println(msg);
  else Serial.print(msg);
}

void debugPrintln() {
  if (config.debugMode) {
    appendDebugLog("", true);
    Serial.println();
  }
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
  setAccessPointMode(true);
  debugPrint("Web server turned off", true);
  logToSyslog("Web server turned off");
  server.stop();  

  wm.setConnectRetries(3);        
  wm.setConfigPortalTimeout(600);   
  wm.setTitle("WX Station");
  wm.startConfigPortal("WX-StationAP");  
  setAccessPointMode(false);
  startMDNSService();

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
          RainGauge::flush();
          ESP.restart();
        }
      }
    }
  } else {
    if (reconnecting) {
      debugPrint("WiFi | Reconnected!", true);
      logToSyslog("WiFi | Reconnected!");
      startMDNSService();
      reconnecting = false;
      failedAttempts = 0;
    }
  }
}

void startMDNSService() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  MDNS.end();

  if (MDNS.begin(localHostname)) {
    MDNS.addService("http", "tcp", 80);
  } else {
    debugPrint("mDNS | Failed to start", true);
    logToSyslog("mDNS | Failed to start");
  }
}

bool initBME280(uint8_t attempts, bool waitBetweenAttempts) {
  for (uint8_t i = 0; i < attempts; i++) {
    if (bme.begin(bmeI2cAddress) && isBME280Responsive()) {
      bmeOK = true;
      bmeReadErrorCount = 0;
      return true;
    }

    if (i + 1 < attempts) {
      debugPrint("SENS | BME280 not found, retrying...", true);
      if (waitBetweenAttempts) {
        delay(1000);
      }
    }
  }

  bmeOK = false;
  return false;
}

bool isI2CDeviceResponsive(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool isBME280Responsive() {
  if (!isI2CDeviceResponsive(bmeI2cAddress)) {
    return false;
  }

  uint8_t sensorId = bme.sensorID();
  return sensorId == 0x60;
}

bool isBH1750Responsive() {
  return isI2CDeviceResponsive(activeBh1750Address);
}

bool initBH1750(uint8_t attempts, bool waitBetweenAttempts) {
  if (!config.activeLight) {
    lightOK = false;
    lightReadErrorCount = 0;
    return true;
  }

  for (uint8_t i = 0; i < attempts; i++) {
    if (isI2CDeviceResponsive(bh1750PrimaryAddress) &&
        lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, bh1750PrimaryAddress)) {
      activeBh1750Address = bh1750PrimaryAddress;
      lightOK = true;
      lightReadErrorCount = 0;
      return true;
    }

    if (isI2CDeviceResponsive(bh1750SecondaryAddress) &&
        lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, bh1750SecondaryAddress)) {
      activeBh1750Address = bh1750SecondaryAddress;
      lightOK = true;
      lightReadErrorCount = 0;
      return true;
    }

    if (i + 1 < attempts) {
      debugPrint("SENS | BH1750 not found, retrying...", true);
      if (waitBetweenAttempts) {
        delay(1000);
      }
    }
  }

  lightOK = false;
  return false;
}

void tryRecoverSensors() {
  static unsigned long lastRecoveryAttempt = 0;
  unsigned long now = millis();

  if (now - lastRecoveryAttempt < sensorRecoveryIntervalMs) {
    return;
  }

  lastRecoveryAttempt = now;

  debugPrint("SENS | Sensor recovery in progress...", true);
  logToSyslog("SENS | Sensor recovery in progress...");

  bool bmeRecovered = initBME280(1, false);
  bool lightRecovered = initBH1750(1, false);

  if (bmeRecovered && lightRecovered) {
    clearRuntimeSensorFault();
    readSensorData();

    if (config.activeLight) {
      readLightSensor();
    }
  }
}

bool synchronizeClock(bool waitForSync) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  time_t now = time(nullptr);
  if (now >= 1700000000) {
    clockSynchronized = true;
    return true;
  }

  unsigned long currentAttemptMs = millis();
  if (!waitForSync && (currentAttemptMs - lastNtpSyncAttempt < ntpResyncIntervalMs)) {
    return clockSynchronized;
  }

  lastNtpSyncAttempt = currentAttemptMs;
  configTime(0, 0, ntpServerPrimary, ntpServerSecondary, ntpServerTertiary);

  if (!waitForSync) {
    return false;
  }

  debugPrint("TIME | Waiting for NTP sync...", true);
  logToSyslog("TIME | Waiting for NTP sync...");

  for (uint8_t attempt = 0; attempt < 20; attempt++) {
    delay(500);
    now = time(nullptr);
    if (now >= 1700000000) {
      clockSynchronized = true;
      debugPrint("TIME | NTP synchronized.", true);
      logToSyslog("TIME | NTP synchronized.");
      return true;
    }
  }

  clockSynchronized = false;
  debugPrint("TIME | NTP sync timeout, continuing without confirmed time.", true);
  logToSyslog("TIME | NTP sync timeout, continuing without confirmed time.");
  return false;
}

void readSensorData() {
  if (!bmeOK) {
    setRuntimeSensorFault("SENS | BME280 unavailable.");
    return;
  }

  if (!isBME280Responsive()) {
    setRuntimeSensorFault("SENS | BME280 communication lost.");
    return;
  }

  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  if (isnan(temp) || isnan(hum) || isnan(pres)) {
    debugPrint("SENS | BME280 read error!", true);
    logToSyslog("SENS | BME280 read error!");

    if (bmeReadErrorCount < 255) {
      bmeReadErrorCount++;
    }

    if (bmeReadErrorCount >= maxConsecutiveBmeReadErrors) {
      setRuntimeSensorFault("SENS | BME280 read failed repeatedly.");
    }
    return;
  }

  bmeReadErrorCount = 0;

  float seaLevel = pres / pow(1.0 - (config.altitude / 44330.0), 5.255);

  temperature = temp + config.offsetTemp;
  humidity    = hum + config.offsetHumi;
  pressure    = pres + config.offsetPress;
  seaLevelPressure = seaLevel + config.offsetPress; 
  rssi = WiFi.RSSI();
}

void readLightSensor() {
    if (!lightOK) {
        setRuntimeSensorFault("SENS | BH1750 unavailable.");
        return;
    }

    if (!isBH1750Responsive()) {
        setRuntimeSensorFault("SENS | BH1750 communication lost.");
        return;
    }

    float lux = lightSensor.readLightLevel();

    if (lux < 0 || isnan(lux)) {
        debugPrint("SENS | BH1750 read error!", true);
        logToSyslog("SENS | BH1750 read error!");

        if (lightReadErrorCount < 255) {
            lightReadErrorCount++;
        }

        if (lightReadErrorCount >= maxConsecutiveLightReadErrors) {
            setRuntimeSensorFault("SENS | BH1750 read failed repeatedly.");
        }
        return;
    }

    lightReadErrorCount = 0;
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
  String rainParam = "";
  if (config.activeRain) {
    rainParam = "&rain_1h=" + String(RainGauge::getRainLastHourMm(), 2);
    rainParam += "&rain_24h=" + String(RainGauge::getRainLast24HoursMm(), 2);
  }

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
      url += rainParam;
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
      url += rainParam;
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
      url += rainParam;
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
    char lightPart[10] = "";
    char rainPart[8] = "";
    char rain24Part[8] = "";

    if (config.activeLight) {
      snprintf(lightPart, sizeof(lightPart), "L%03d", (int)lightWm2);
    }

    if (config.activeRain) {
      float rainLastHourInches = RainGauge::getRainLastHourMm() / 25.4f;
      int rainHundredths = (int)roundf(rainLastHourInches * 100.0f);
      if (rainHundredths < 0) {
        rainHundredths = 0;
      } else if (rainHundredths > 999) {
        rainHundredths = 999;
      }
      snprintf(rainPart, sizeof(rainPart), "r%03d", rainHundredths);

      float rainLast24HoursInches = RainGauge::getRainLast24HoursMm() / 25.4f;
      int rain24Hundredths = (int)roundf(rainLast24HoursInches * 100.0f);
      if (rain24Hundredths < 0) {
        rain24Hundredths = 0;
      } else if (rain24Hundredths > 999) {
        rain24Hundredths = 999;
      }
      snprintf(rain24Part, sizeof(rain24Part), "p%03d", rain24Hundredths);
    }

    snprintf(sentence, sizeof(sentence),
             "%s>APRS,TCPIP*:@%02d%02d%02dz%s/%s_.../...t%03dh%02db%05d%s%s%s%s",
             config.aprsCall,
             0, 0, 0,
             config.aprsLat,
             config.aprsLon,
             (int)temperatureF,
             (int)humidity,
             (int)(seaLevelPressure * 10),
             lightPart,
             rainPart,
             rain24Part,
             config.aprsComment);

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

  StaticJsonDocument<256> jsonDoc;
  jsonDoc[config.dataTemp]  = roundf(temperature * 100) / 100.0;
  jsonDoc[config.dataHumi]  = roundf(humidity * 100) / 100.0;
  jsonDoc[config.dataPress] = roundf(seaLevelPressure * 100) / 100.0;
  if (config.activeLight) {
    jsonDoc[config.dataLight] = roundf(lightWm2 * 100) / 100.0;
  }
  if (config.activeRain) {
    jsonDoc["rain_1h"] = roundf(RainGauge::getRainLastHourMm() * 100) / 100.0;
    jsonDoc["rain_24h"] = roundf(RainGauge::getRainLast24HoursMm() * 100) / 100.0;
  }
  jsonDoc[config.dataRssi]  = rssi;

  char jsonBuffer[320];
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
    RainGauge::flush();
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
  // ======= Get config value =======
  else if (message.startsWith("get(") && message.endsWith(")")) {
    String key = message.substring(4, message.length() - 1);
    key.trim();
    
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
      debugPrint("MQTT | RECV KO | Command get(" + key + ") -> Cannot open config", true);
      logToSyslog(("MQTT | RECV KO | Command get(" + key + ") -> Cannot open config").c_str());
      return;
    }

    // Return entire config
    if (key == "config") {
      String configJson;
      while (file.available()) {
        configJson += (char)file.read();
      }
      file.close();
      
      mqttClient.publish(config.mqttTopicPub2.c_str(), configJson.c_str());
      debugPrint("MQTT | RECV OK | Command get(config) -> Full config sent", true);
      logToSyslog("MQTT | RECV OK | Command get(config) -> Full config sent");
      return;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
      debugPrint("MQTT | RECV KO | Command get(" + key + ") -> JSON parse error", true);
      logToSyslog(("MQTT | RECV KO | Command get(" + key + ") -> JSON parse error").c_str());
      return;
    }

    if (doc.containsKey(key)) {
      String value;
      JsonVariant val = doc[key];
      
      if (val.is<bool>()) {
        value = val.as<bool>() ? "true" : "false";
      } else if (val.is<int>()) {
        value = String(val.as<int>());
      } else if (val.is<float>()) {
        value = String(val.as<float>());
      } else {
        value = val.as<String>();
      }

      String response = key + "(" + value + ")";
      mqttClient.publish(config.mqttTopicPub2.c_str(), response.c_str());
      debugPrint("MQTT | RECV OK | Command get(" + key + ") -> " + response, true);
      logToSyslog(("MQTT | RECV OK | Command get(" + key + ") -> " + response).c_str());
    } else {
      debugPrint("MQTT | RECV KO | Command get(" + key + ") -> Unknown key", true);
      logToSyslog(("MQTT | RECV KO | Command get(" + key + ") -> Unknown key").c_str());
    }
  }
  // ======= Set full config JSON =======
  else if (message.startsWith("set(config=")) {
    int startIdx = 11; 
    int endIdx = message.lastIndexOf(')');
    
    if (endIdx <= startIdx) {
      debugPrint("MQTT | RECV KO | Command set(config) -> Invalid format", true);
      logToSyslog("MQTT | RECV KO | Command set(config) -> Invalid format");
      return;
    }
    
    String jsonStr = message.substring(startIdx, endIdx);
    jsonStr.trim();
    
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
      String errMsg = "MQTT | RECV KO | Command set(config) -> JSON parse error: " + String(error.c_str());
      debugPrint(errMsg, true);
      logToSyslog(errMsg.c_str());
      return;
    }
    
    File outFile = LittleFS.open("/config.json", "w");
    if (outFile) {
      serializeJsonPretty(doc, outFile);
      outFile.close();
      loadConfig();
      Heartbeat::setEnabled(config.activeHeartbeat);
      RainGauge::onConfigurationChanged(config.activeRain, config.rainTipMm);
      
      mqttClient.publish(config.mqttTopicPub2.c_str(), "set(config) OK");
      debugPrint("MQTT | RECV OK | set(config) -> Full config replaced", true);
      logToSyslog("MQTT | RECV OK | set(config) -> Full config replaced");
    } else {
      debugPrint("MQTT | RECV KO | Command set(config) -> Failed to save", true);
      logToSyslog("MQTT | RECV KO | Command set(config) -> Failed to save");
    }
  }
  // ======= Set config value =======
  else if (message.startsWith("set(") && message.endsWith(")")) {
    String content = message.substring(4, message.length() - 1);
    int eqPos = content.indexOf('=');
    
    if (eqPos == -1) {
      debugPrint("MQTT | RECV KO | Command set() -> Missing '='", true);
      logToSyslog("MQTT | RECV KO | Command set() -> Missing '='");
      return;
    }
    
    String key = content.substring(0, eqPos);
    String value = content.substring(eqPos + 1);
    key.trim();
    value.trim();
    
    File file = LittleFS.open("/config.json", "r");
    StaticJsonDocument<2048> doc;
    
    if (file) {
      deserializeJson(doc, file);
      file.close();
    }

    String valueParsed = value;
    valueParsed.replace(',', '.');
    bool hasDecimal = (valueParsed.indexOf('.') >= 0);
    
    // Fields that allow float values
    bool isFloatField = (key == "altitude" || key == "offsetTemp" || key == "offsetHumi" ||
                         key == "offsetPress" || key == "rainTipMm");
    
    if (doc.containsKey(key)) {
      JsonVariant existing = doc[key];
      if (existing.is<bool>()) {
        doc[key] = (value == "true" || value == "1");
      } else if (existing.is<float>() || existing.is<double>() || existing.is<int>()) {
        if (isFloatField) {
          doc[key] = valueParsed.toFloat();
        } else {
          doc[key] = valueParsed.toInt();
        }
      } else {
        doc[key] = value;
      }
    } else {
      // New key - auto-detect type from value
      if (value == "true" || value == "false") {
        doc[key] = (value == "true");
      } else if (isFloatField && hasDecimal) {
        doc[key] = valueParsed.toFloat();
      } else if (valueParsed.toInt() != 0 || valueParsed == "0") {
        doc[key] = valueParsed.toInt();
      } else {
        doc[key] = value;
      }
    }
    
    File outFile = LittleFS.open("/config.json", "w");
    if (outFile) {
      serializeJsonPretty(doc, outFile);
      outFile.close();
      loadConfig();
      Heartbeat::setEnabled(config.activeHeartbeat);
      RainGauge::onConfigurationChanged(config.activeRain, config.rainTipMm);
      
      String response = "set(" + key + "=" + value + ") OK";
      mqttClient.publish(config.mqttTopicPub2.c_str(), response.c_str());
      debugPrint("MQTT | RECV OK | " + response, true);
      logToSyslog(("MQTT | RECV OK | " + response).c_str());
    } else {
      debugPrint("MQTT | RECV KO | Command set(" + key + ") -> Failed to save", true);
      logToSyslog(("MQTT | RECV KO | Command set(" + key + ") -> Failed to save").c_str());
    }
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
          RainGauge::flush();
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
  loadConfig();
  Heartbeat::setEnabled(config.activeHeartbeat);
  Heartbeat::begin();
  Wire.begin();

  WiFi.setHostname("WX-Station");
  wm.setConnectRetries(3);
  wm.setConfigPortalTimeout(600);
  wm.setTitle("WX Station");
  wm.setAPCallback(onConfigPortalStarted);

  if (!wm.autoConnect("WX-StationAP")) {
    debugPrint("REST | Failed to connect, restarting...", true);
    logToSyslog("REST | Failed to connect, restarting...");
    RainGauge::flush();
    ESP.restart();
  }
  setAccessPointMode(false);
  startMDNSService();
  synchronizeClock(true);
  RainGauge::begin(config.activeRain, config.rainTipMm);

  if (!initBME280()) {
      setRuntimeSensorFault("SENS | BME280 initialization failed.");
  }

  // Init BH1750 
  if (!fatalErrorActive && !initBH1750()) {
      setRuntimeSensorFault("SENS | BH1750 initialization failed.");
  }

  setupWeb();

  // MQTT setup
  mqttClient.setBufferSize(2048);
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
      setFatalError("SYST | LittleFS mount failed, even after format!");
    }
  }

  if (fatalErrorActive) {
    return;
  }

  sendInfoToDB();
  restartInterval();
  readSensorData();
  publishToMQTT();
  sendDataToDB();
  sendDataToAPRS();

  setupCompleted = true;
  refreshHeartbeatState();
}

// ====== Loop ======
void loop() {
  Heartbeat::update();
  RainGauge::update();

  if (fatalErrorActive) {
    return;
  }

  if (runtimeSensorFaultActive) {
    reconnectWiFi();
    runningMQTT();
    server.handleClient();
    tryRecoverSensors();
    return;
  }

  unsigned long now = millis();

  // Periodic restart
  if (restartIntervalMs > 0 && (now - lastRestart >= restartIntervalMs)) {
    debugPrint("REST | Periodic restart...", true);
    logToSyslog("REST | Periodic restart...");
    delay(1000);
    RainGauge::flush();
    ESP.restart();
  }

  // Wi-Fi watchdog 
  reconnectWiFi(); 
  synchronizeClock(false);

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
