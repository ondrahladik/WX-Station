#pragma once
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

constexpr uint8_t GPIO_TRIGGER_COUNT = 3;
constexpr int8_t GPIO_TRIGGER_PIN_DISABLED = -1;

enum GPIOTriggerMetric : uint8_t {
  GPIO_TRIGGER_METRIC_TEMPERATURE = 0,
  GPIO_TRIGGER_METRIC_HUMIDITY = 1,
  GPIO_TRIGGER_METRIC_PRESSURE = 2,
  GPIO_TRIGGER_METRIC_LIGHT = 3,
  GPIO_TRIGGER_METRIC_RAIN_1H = 4,
  GPIO_TRIGGER_METRIC_RAIN_24H = 5,
  GPIO_TRIGGER_METRIC_RSSI = 6,
  GPIO_TRIGGER_METRIC_COUNT
};

struct GPIOTriggerConfig {
  bool enabled;
  float triggerOnValue;
  float triggerOffValue;
  uint8_t value;
  int8_t gpioPin;
};

// ===== Config structure =====
struct Config {
  // Active config
  bool debugMode;
  bool activeHeartbeat;
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
  bool activeRain;
  
  // Offset config
  float offsetTemp;
  float offsetHumi;
  float offsetPress;

  // Rain config
  float rainTipMm;

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

  // GPIO trigger config
  GPIOTriggerConfig gpioTriggers[GPIO_TRIGGER_COUNT];

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
