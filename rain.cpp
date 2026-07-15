#include "rain.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>

namespace RainGauge {

namespace {

constexpr const char* kStateFile = "/rain_state.bin";
constexpr unsigned long kPersistIntervalMs = 10000UL;
constexpr unsigned long kAcceptedPulseGapMs = 250UL;
constexpr unsigned long kMinPulseLowMs = 15UL;
constexpr unsigned long kMaxPulseLowMs = 1000UL;
constexpr uint32_t kRain1hWindowSec = 60UL * 60UL;
constexpr uint32_t kRain24hWindowSec = 24UL * 60UL * 60UL;
constexpr size_t kMaxStoredTips = 1024;
constexpr uint32_t kStateVersion = 6;
constexpr time_t kValidEpochThreshold = 1700000000;

volatile uint32_t pendingTips = 0;
volatile bool pulseLowActive = false;
volatile unsigned long pulseLowStartedAtMs = 0;
volatile unsigned long lastAcceptedPulseAtMs = 0;

bool enabled = false;
float tipMm = 0.2794f;
uint32_t totalTips = 0;
uint32_t rain1hTips = 0;
uint32_t rain24hTips = 0;
bool stateLoaded = false;
bool stateDirty = false;
unsigned long lastPersistAtMs = 0;

uint32_t tipTimestampsSec[kMaxStoredTips] = {0};
size_t tipHead = 0;
size_t tipCount = 0;

struct PersistedState {
  uint32_t version;
  uint32_t totalTips;
  uint16_t tipCount;
  uint32_t tipTimestampsSec[kMaxStoredTips];
};

PersistedState persistedState;

bool getCurrentSeconds(uint32_t& nowSec) {
  time_t now = time(nullptr);
  if (now < kValidEpochThreshold) {
    return false;
  }

  nowSec = static_cast<uint32_t>(now);
  return true;
}

size_t physicalIndex(size_t logicalOffset) {
  return (tipHead + logicalOffset) % kMaxStoredTips;
}

void clearTipBuffer() {
  memset(tipTimestampsSec, 0, sizeof(tipTimestampsSec));
  tipHead = 0;
  tipCount = 0;
}

void pruneOldTips24h(uint32_t nowSec) {
  bool removedAnyTip = false;

  while (tipCount > 0) {
    uint32_t oldestTipSec = tipTimestampsSec[tipHead];
    uint32_t ageSec = nowSec - oldestTipSec;
    if (ageSec < kRain24hWindowSec) {
      break;
    }

    tipTimestampsSec[tipHead] = 0;
    tipHead = (tipHead + 1) % kMaxStoredTips;
    tipCount--;
    removedAnyTip = true;
  }

  if (removedAnyTip) {
    stateDirty = true;
  }

  rain24hTips = static_cast<uint32_t>(tipCount);
  if (rain1hTips > rain24hTips) {
    rain1hTips = rain24hTips;
  }
}

void pruneOldTips1h(uint32_t nowSec) {
  while (rain1hTips > 0) {
    size_t oldest1hOffset = tipCount - rain1hTips;
    uint32_t oldest1hTipSec = tipTimestampsSec[physicalIndex(oldest1hOffset)];
    uint32_t ageSec = nowSec - oldest1hTipSec;
    if (ageSec < kRain1hWindowSec) {
      break;
    }

    rain1hTips--;
  }
}

void appendTip(uint32_t timestampSec) {
  if (tipCount < kMaxStoredTips) {
    tipTimestampsSec[physicalIndex(tipCount)] = timestampSec;
    tipCount++;
    return;
  }

  tipTimestampsSec[tipHead] = timestampSec;
  tipHead = (tipHead + 1) % kMaxStoredTips;
  stateDirty = true;
}

void IRAM_ATTR handleTipSignalChange() {
  unsigned long now = millis();
  bool isLow = (digitalRead(kRainGaugePin) == LOW);

  if (isLow) {
    if (!pulseLowActive) {
      pulseLowActive = true;
      pulseLowStartedAtMs = now;
    }
    return;
  }

  if (!pulseLowActive) {
    return;
  }

  unsigned long lowDurationMs = now - pulseLowStartedAtMs;
  pulseLowActive = false;

  if (lowDurationMs < kMinPulseLowMs || lowDurationMs > kMaxPulseLowMs) {
    return;
  }

  if (now - lastAcceptedPulseAtMs < kAcceptedPulseGapMs) {
    return;
  }

  lastAcceptedPulseAtMs = now;
  pendingTips++;
}

void detachGaugeInterrupt() {
  detachInterrupt(digitalPinToInterrupt(kRainGaugePin));
}

void attachGaugeInterrupt() {
  pinMode(kRainGaugePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kRainGaugePin), handleTipSignalChange, CHANGE);
}

bool loadState() {
  totalTips = 0;
  rain1hTips = 0;
  rain24hTips = 0;
  clearTipBuffer();
  stateLoaded = false;

  if (!LittleFS.exists(kStateFile)) {
    stateLoaded = true;
    return true;
  }

  File file = LittleFS.open(kStateFile, "r");
  if (!file) {
    return false;
  }

  size_t bytesRead = file.read(reinterpret_cast<uint8_t*>(&persistedState), sizeof(persistedState));
  file.close();

  if (bytesRead != sizeof(persistedState) || persistedState.version != kStateVersion) {
    stateLoaded = true;
    return true;
  }

  totalTips = persistedState.totalTips;

  uint32_t nowSec = 0;
  if (!getCurrentSeconds(nowSec)) {
    return false;
  }

  size_t restoredTipCount = persistedState.tipCount;
  if (restoredTipCount > kMaxStoredTips) {
    restoredTipCount = kMaxStoredTips;
  }

  for (size_t i = 0; i < restoredTipCount; i++) {
    uint32_t tipSec = persistedState.tipTimestampsSec[i];
    if (tipSec == 0 || tipSec > nowSec) {
      continue;
    }

    uint32_t ageSec = nowSec - tipSec;
    if (ageSec >= kRain24hWindowSec) {
      continue;
    }

    appendTip(tipSec);
  }

  rain24hTips = static_cast<uint32_t>(tipCount);
  rain1hTips = rain24hTips;
  pruneOldTips1h(nowSec);
  stateDirty = false;
  stateLoaded = true;
  return true;
}

bool saveState() {
  uint32_t nowSec = 0;
  if (!getCurrentSeconds(nowSec)) {
    return false;
  }

  memset(&persistedState, 0, sizeof(persistedState));
  persistedState.version = kStateVersion;
  persistedState.totalTips = totalTips;
  persistedState.tipCount = static_cast<uint16_t>(tipCount);

  for (size_t i = 0; i < tipCount; i++) {
    size_t index = physicalIndex(i);
    persistedState.tipTimestampsSec[i] = tipTimestampsSec[index];
  }

  File file = LittleFS.open(kStateFile, "w");
  if (!file) {
    return false;
  }

  size_t bytesWritten = file.write(reinterpret_cast<const uint8_t*>(&persistedState), sizeof(persistedState));
  file.close();
  if (bytesWritten != sizeof(persistedState)) {
    return false;
  }

  stateDirty = false;
  lastPersistAtMs = millis();
  return true;
}

void consumePendingTips(uint32_t nowSec) {
  uint32_t capturedTips = 0;

  noInterrupts();
  capturedTips = pendingTips;
  pendingTips = 0;
  interrupts();

  if (capturedTips == 0) {
    return;
  }

  for (uint32_t i = 0; i < capturedTips; i++) {
    appendTip(nowSec);
  }

  totalTips += capturedTips;
  rain24hTips += capturedTips;
  rain1hTips += capturedTips;
  stateDirty = true;
}

}  // namespace

void begin(bool enabledValue, float tipMmValue) {
  loadState();
  lastPersistAtMs = millis();
  onConfigurationChanged(enabledValue, tipMmValue);
}

void update() {
  if (!stateLoaded && !loadState()) {
    return;
  }

  uint32_t nowSec = 0;
  if (!getCurrentSeconds(nowSec)) {
    return;
  }

  consumePendingTips(nowSec);
  pruneOldTips24h(nowSec);
  pruneOldTips1h(nowSec);

  if (stateDirty && (millis() - lastPersistAtMs >= kPersistIntervalMs)) {
    saveState();
  }
}

void onConfigurationChanged(bool enabledValue, float tipMmValue) {
  enabled = enabledValue;
  tipMm = (tipMmValue > 0.0f) ? tipMmValue : 0.2794f;

  detachGaugeInterrupt();
  if (enabled) {
    attachGaugeInterrupt();
  }
}

void flush() {
  if (stateDirty) {
    saveState();
  }
}

void reset() {
  detachGaugeInterrupt();
  noInterrupts();
  pendingTips = 0;
  pulseLowActive = false;
  pulseLowStartedAtMs = 0;
  lastAcceptedPulseAtMs = 0;
  interrupts();

  totalTips = 0;
  rain1hTips = 0;
  rain24hTips = 0;
  stateLoaded = true;
  stateDirty = false;
  lastPersistAtMs = millis();
  clearTipBuffer();

  if (LittleFS.exists(kStateFile)) {
    LittleFS.remove(kStateFile);
  }

  if (enabled) {
    attachGaugeInterrupt();
  }
}

bool isEnabled() {
  return enabled;
}

uint8_t getPin() {
  return kRainGaugePin;
}

uint32_t getTotalTips() {
  return totalTips;
}

float getRainLastHourMm() {
  return static_cast<float>(rain1hTips) * tipMm;
}

float getRainLast24HoursMm() {
  return static_cast<float>(rain24hTips) * tipMm;
}

} 
