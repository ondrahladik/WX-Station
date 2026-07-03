#include "heartbeat.h"

namespace Heartbeat {

namespace {

struct PatternStep {
  bool ledOn;
  unsigned long durationMs;
};

constexpr PatternStep NORMAL_PATTERN[] = {
  {true, 120},
  {false, 260},
  {true, 120},
  {false, 2200},
};

constexpr PatternStep ERROR_PATTERN[] = {
  {true, 120},
  {false, 120},
};

State currentState = State::Booting;
unsigned long lastStepChangeMs = 0;
size_t currentStep = 0;
bool initialized = false;
bool heartbeatEnabled = false;

void writeLed(bool ledOn) {
  digitalWrite(
    HEARTBEAT_LED_PIN,
    ledOn == HEARTBEAT_LED_ACTIVE_HIGH ? HIGH : LOW
  );
}

template <size_t N>
void applyPatternStep(const PatternStep (&pattern)[N], size_t stepIndex, unsigned long now) {
  writeLed(pattern[stepIndex].ledOn);
  currentStep = stepIndex;
  lastStepChangeMs = now;
}

template <size_t N>
void updatePattern(const PatternStep (&pattern)[N], unsigned long now) {
  while (now - lastStepChangeMs >= pattern[currentStep].durationMs) {
    size_t nextStep = (currentStep + 1) % N;
    applyPatternStep(pattern, nextStep, now);
  }
}

void applyState(State state, unsigned long now) {
  if (!heartbeatEnabled) {
    writeLed(false);
    lastStepChangeMs = now;
    currentStep = 0;
    return;
  }

  switch (state) {
    case State::Booting:
      writeLed(false);
      lastStepChangeMs = now;
      currentStep = 0;
      break;
    case State::AccessPoint:
      writeLed(true);
      lastStepChangeMs = now;
      currentStep = 0;
      break;
    case State::Normal:
      applyPatternStep(NORMAL_PATTERN, 0, now);
      break;
    case State::Error:
      applyPatternStep(ERROR_PATTERN, 0, now);
      break;
  }
}

} 

void begin() {
  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  initialized = true;
  applyState(currentState, millis());
}

void setEnabled(bool enabled) {
  heartbeatEnabled = enabled;

  if (!initialized) {
    return;
  }

  applyState(currentState, millis());
}

bool isEnabled() {
  return heartbeatEnabled;
}

void setState(State state) {
  if (!initialized || currentState == state) {
    currentState = state;
    return;
  }

  currentState = state;
  applyState(currentState, millis());
}

State getState() {
  return currentState;
}

void update() {
  if (!initialized || !heartbeatEnabled) {
    return;
  }

  unsigned long now = millis();

  switch (currentState) {
    case State::Normal:
      updatePattern(NORMAL_PATTERN, now);
      break;
    case State::Error:
      updatePattern(ERROR_PATTERN, now);
      break;
    case State::Booting:
    case State::AccessPoint:
      break;
  }
}

}  
