#include "config.h"

Config config; 
const char* configFile = "/config.json";

bool loadConfig() {
  if (!LittleFS.begin(true)) {
    Serial.println("SYST | LittleFS mount failed!");
    return false;
  }

  // default
  if (!LittleFS.exists(configFile)) {
    Serial.println("SYST | Config file not found, using defaults.");
    
    // Active config default
    config.debugMode     = false; 
    config.activeAPRS    = false; 
    config.activeMQTT    = false; 
    config.activeSYSLOG  = false; 

    // Station config default
    config.stationName = "wx-station";
    config.altitude    = 230.0;

    // Data active
    config.activeLight = false; 

    // Data config defaults
    config.dataTemp    = "temperature";
    config.dataHumi    = "humidity";
    config.dataPress   = "pressure";
    config.dataLight   = "light";
    config.dataRssi    = "rssi";

    // Offset config defaults
    config.offsetTemp  = 0.0;
    config.offsetHumi  = 0.0;
    config.offsetPress = 0.0;

    // SERVER config defaults
    config.serverActive0 = false;
    config.serverUrl0    = "http://example.com/";
    config.serverName0   = "";
    config.serverActive1 = false;
    config.serverUrl1    = "http://example.com/";
    config.serverName1   = "";
    config.serverActive2 = false;
    config.serverUrl2    = "http://example.com/";
    config.serverName2   = "";
    config.serverActive3 = false;
    config.serverUrl3    = "http://example.com/";
    config.serverName3   = "";

    // APRS config defaults
    config.aprsHost    = "euro.aprs2.net";
    config.aprsPort    = 14580;
    config.aprsCall    = "NOCALL-13";
    config.aprsPass    = "12345";
    config.aprsLat     = "0000.00N";
    config.aprsLon     = "00000.00E";
    strcpy(config.aprsComment, "WX-Station https://www.ok1kky.cz");

    // MQTT config defaults
    config.mqttServer     = "example.com";
    config.mqttPort       = 1883;
    config.mqttTopicPub1  = "";
    config.mqttTopicSub1  = "";
    config.mqttTopicSub2  = "";

    // SYSLOG config defaults
    config.syslogServer  = "example.com";
    config.syslogPort    = 514;

    // Interval config
    config.intervalHttp  = 300000;
    config.intervalAprs  = 600000;
    config.intervalMqtt  = 100000;  
    config.restartMode  = 2;   

    return false;
  };


  File file = LittleFS.open(configFile, "r");
  if (!file) {
    Serial.println("SYST | Failed to open config file.");
    return false;
  }

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("SYST | Failed to parse config file.");
    return false;
  }

  // Active config
  config.debugMode   = doc["debugMode"]       | false;
  config.activeAPRS  = doc["activeAPRS"]      | false;
  config.activeMQTT  = doc["activeMQTT"]      | false;
  config.activeSYSLOG  = doc["activeSYSLOG"]  | false;

  // Station config
  config.stationName = doc["stationName"]    | "wx-station";
  config.altitude = doc["altitude"]          | 230.0;

  // Data active
  config.activeLight = doc["activeLight"]   | false;

  // Data config
  config.dataTemp    = doc["dataTemp"]    | "temperature";
  config.dataHumi    = doc["dataHumi"]    | "humidity";
  config.dataPress   = doc["dataPress"]   | "pressure";
  config.dataLight   = doc["dataLight"]   | "light";
  config.dataRssi    = doc["dataRssi"]    | "rssi";

  // Offset config
  config.offsetTemp  = doc["offsetTemp"]  | 0.0;
  config.offsetHumi  = doc["offsetHumi"]  | 0.0;
  config.offsetPress = doc["offsetPress"] | 0.0;

  // SERVER config 
  config.serverActive0   = doc["serverActive0"]  | false;
  config.serverUrl0      = doc["serverUrl0"]     | "http://example.com/";
  config.serverName0     = doc["serverName0"]    | "";
  config.serverActive1   = doc["serverActive1"]  | false;
  config.serverUrl1      = doc["serverUrl1"]     | "http://example.com/";
  config.serverName1     = doc["serverName1"]    | "";
  config.serverActive2   = doc["serverActive2"]  | false;
  config.serverUrl2      = doc["serverUrl2"]     | "http://example.com/";
  config.serverName2     = doc["serverName2"]    | "";
  config.serverActive3   = doc["serverActive3"]  | false;
  config.serverUrl3      = doc["serverUrl3"]     | "http://example.com/";
  config.serverName3     = doc["serverName3"]    | "";

  // APRS config
  config.aprsHost    = doc["aprsHost"]    | "euro.aprs2.net";
  config.aprsPort    = doc["aprsPort"]    | 14580;
  config.aprsCall    = doc["aprsCall"]    | "NOCALL-13";
  config.aprsPass    = doc["aprsPass"]    | "12345";
  config.aprsLat     = doc["aprsLat"]     | "0000.00N";
  config.aprsLon     = doc["aprsLon"]     | "00000.00E";
  const char* comment = doc["aprsComment"] | "WX-Station https://www.ok1kky.cz";
  strcpy(config.aprsComment, comment);

  // MQTT config
  config.mqttServer     = doc["mqttServer"]    | "example.com";
  config.mqttPort       = doc["mqttPort"]      | 1883;
  config.mqttTopicPub1  = doc["mqttTopicPub1"] | "";
  config.mqttTopicSub1  = doc["mqttTopicSub1"] | "";
  config.mqttTopicSub2  = doc["mqttTopicSub2"] | "";

  // SYSLOG config
  config.syslogServer = doc["syslogServer"] | "example.com";
  config.syslogPort   = doc["syslogPort"]   | 514;

  // Interval config
  config.intervalHttp   = doc["intervalHttp"]  | 300000;
  config.intervalAprs   = doc["intervalAprs"]  | 600000;
  config.intervalMqtt   = doc["intervalMqtt"]  | 100000;
  config.restartMode    = doc["restartMode"]   | 2;

  return true;
}

bool saveConfig() {
  StaticJsonDocument<2048> doc; 
  doc["debugMode"]      = config.debugMode;
  doc["activeAPRS"]     = config.activeAPRS;
  doc["activeMQTT"]     = config.activeMQTT;
  doc["activeSYSLOG"]   = config.activeSYSLOG;

  // Station config
  doc["stationName"]    = config.stationName;
  doc["altitude"]       = config.altitude;
  
  // Data active  
  doc["activeLight"] = config.activeLight;

  // Data config
  doc["dataTemp"]    = config.dataTemp;
  doc["dataHumi"]    = config.dataHumi;
  doc["dataPress"]   = config.dataPress;
  doc["dataLight"]   = config.dataLight;
  doc["dataRssi"]    = config.dataRssi;

  // Offset config
  doc["offsetTemp"]  = config.offsetTemp;
  doc["offsetHumi"]  = config.offsetHumi;
  doc["offsetPress"] = config.offsetPress;

  // SERVER config
  doc["serverActive0"]    = config.serverActive0;
  doc["serverUrl0"]       = config.serverUrl0;
  doc["serverName0"]      = config.serverName0;
  doc["serverActive1"]    = config.serverActive1;
  doc["serverUrl1"]       = config.serverUrl1;
  doc["serverName1"]      = config.serverName1;
  doc["serverActive2"]    = config.serverActive2;
  doc["serverUrl2"]       = config.serverUrl2;
  doc["serverName2"]      = config.serverName2;
  doc["serverActive3"]    = config.serverActive3;
  doc["serverUrl3"]       = config.serverUrl3;
  doc["serverName3"]      = config.serverName3;

  // APRS config
  doc["aprsHost"]    = config.aprsHost;
  doc["aprsPort"]    = config.aprsPort;
  doc["aprsCall"]    = config.aprsCall;
  doc["aprsPass"]    = config.aprsPass;
  doc["aprsLat"]     = config.aprsLat;
  doc["aprsLon"]     = config.aprsLon;
  doc["aprsComment"] = config.aprsComment;

  // MQTT config
  doc["mqttServer"]     = config.mqttServer;
  doc["mqttPort"]       = config.mqttPort;
  doc["mqttTopicPub1"]  = config.mqttTopicPub1;
  doc["mqttTopicSub1"]  = config.mqttTopicSub1;
  doc["mqttTopicSub2"]  = config.mqttTopicSub2;

  // SYSLOG config
  doc["syslogServer"] = config.syslogServer;
  doc["syslogPort"]   = config.syslogPort;

  // Interval config
  doc["intervalHttp"]   = config.intervalHttp;
  doc["intervalAprs"]   = config.intervalAprs;
  doc["intervalMqtt"]   = config.intervalMqtt;
  doc["restartMode"]    = config.restartMode;

  File file = LittleFS.open(configFile, "w");
  if (!file) {
    Serial.println("SYST | Failed to open config file for writing.");
    return false;
  }

  serializeJsonPretty(doc, file);
  file.close();
  return true;
}
