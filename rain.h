#pragma once

#include <Arduino.h>

namespace RainGauge {

constexpr uint8_t kRainGaugePin = 27;

void begin(bool enabled, float tipMm);
void update();
void onConfigurationChanged(bool enabled, float tipMm);
void flush();
void reset();

bool isEnabled();
uint8_t getPin();
uint32_t getTotalTips();
float getRainLastHourMm();
float getRainLast24HoursMm();

}  
