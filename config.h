#pragma once
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

// ===== Config structure =====
struct Config {
  // Active config
  bool debugMode;
  bool activeAPRS;
  bool activeMQTT;
  bool activeSYSLOG;

  // Station config
  String stationName;
  float altitude;

  // Data config
  String dataTemp;
  String dataHumi;
  String dataPress;
  String dataLight;
  String dataRssi;

  // Data active
  bool activeLight;
  
  // Offset config
  float offsetTemp;
  float offsetHumi;
  float offsetPress;

  // Server config
  bool serverActive0;
  String serverUrl0;
  String serverName0;
  bool serverActive1;
  String serverUrl1;
  String serverName1;
  bool serverActive2;
  String serverUrl2;
  String serverName2;
  bool serverActive3;
  String serverUrl3;
  String serverName3;

  // APRS config
  String aprsHost;
  int aprsPort;
  String aprsCall;
  String aprsPass;
  String aprsLat;
  String aprsLon;
  char aprsComment[64]; 

  // MQTT config
  String mqttServer;  
  int mqttPort;     

  // MQTT topics
  String mqttTopicPub1;
  String mqttTopicPub2;
  String mqttTopicSub1;
  String mqttTopicSub2;

  // Syslog config
  String syslogServer;
  int syslogPort;

  // Interval config
  int intervalHttp; 
  int intervalAprs;   
  int intervalMqtt;   
  int restartMode;
};

extern Config config;

bool loadConfig();
bool saveConfig();
