#pragma once

#include <Arduino.h>

namespace Heartbeat {

constexpr uint8_t HEARTBEAT_LED_PIN = 2;
constexpr bool HEARTBEAT_LED_ACTIVE_HIGH = true;

enum class State : uint8_t {
  Booting,
  Normal,
  AccessPoint,
  Error
};

void begin();
void setEnabled(bool enabled);
bool isEnabled();
void setState(State state);
State getState();
void update();

}  
