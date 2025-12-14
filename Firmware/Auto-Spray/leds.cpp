#include "config.h"
#include "leds.h"

static uint8_t blinkCount = 0;
static uint8_t currentBlink = 0;
static uint32_t lastBlinkTime = 0;
static bool ledOn = false;
static bool blinking = false;

void updateSprayMode(SprayMode currentMode) {
  if (checkSprayMode(currentMode)) {
    startBlinkConfirm(currentMode);
  }
  blinkSprayConfirm();
}

void startBlinkConfirm(SprayMode count) {
  if (count == 0 || count > 3) {
    blinking = false;
    digitalWrite(PIN_ADD_LED, LOW);
    return;
  }
  blinkCount = count;
  currentBlink = 0;
  ledOn = true;
  blinking = true;
  lastBlinkTime = millis();
  digitalWrite(PIN_ADD_LED, HIGH);
}

void blinkSprayConfirm() {
  if (!blinking) return;

  uint32_t now = millis();

  if (ledOn) {
    if (now - lastBlinkTime >= BLINK_ON_CONFIRM_MODE_MS) {
      digitalWrite(PIN_ADD_LED, LOW);
      ledOn = false;
      lastBlinkTime = now;
    }
  } else {
    if (now - lastBlinkTime >= BLINK_OFF_CONFIRM_MODE_MS) {
      currentBlink++;
      if (currentBlink >= blinkCount) {
        blinking = false;
        digitalWrite(PIN_ADD_LED, LOW);
        return;
      }
      digitalWrite(PIN_ADD_LED, HIGH);
      ledOn = true;
      lastBlinkTime = now;
    }
  }
}

void updateLed(LedColor red, LedColor green, LedColor blue) {
#if LED_COMMON_ANODE
  // Общий анод: LOW = включить, HIGH = выключить
  digitalWrite(PIN_LED_R, (red == LED_RED_ON) ? LOW : HIGH);
  digitalWrite(PIN_LED_G, (green == LED_GREEN_ON) ? LOW : HIGH);
  digitalWrite(PIN_LED_B, (blue == LED_BLUE_ON) ? LOW : HIGH);
#else
  // Общий катод: HIGH = включить, LOW = выключить
  digitalWrite(PIN_LED_R, (red == LED_RED_ON) ? HIGH : LOW);
  digitalWrite(PIN_LED_G, (green == LED_GREEN_ON) ? HIGH : LOW);
  digitalWrite(PIN_LED_B, (blue == LED_BLUE_ON) ? HIGH : LOW);
#endif
}

#if ACTIVITY_LED_ENABLED

void updateActivityLed() {
  static uint32_t lastToggle = 0;
  static bool ledState = false;
  uint32_t now = millis();

  if (ledState) {
    if (now - lastToggle >= ACTIVITY_LED_ON_MS) {
      digitalWrite(PIN_LED_BUILTIN, LOW);
      ledState = false;
      lastToggle = now;
    }
  } else {
    if (now - lastToggle >= ACTIVITY_LED_OFF_MS) {
      digitalWrite(PIN_LED_BUILTIN, HIGH);
      ledState = true;
      lastToggle = now;
    }
  }
}

void updateBatteryLed() {
  static uint32_t lastToggle = 0;
  static bool ledState = false;
  uint32_t now = millis();

  if (ledState) {
    if (now - lastToggle >= BATTERY_LED_ON_MS) {
      digitalWrite(PIN_ADD_LED, LOW);
      ledState = false;
      lastToggle = now;
    }
  } else {
    if (now - lastToggle >= BATTERY_LED_OFF_MS) {
      digitalWrite(PIN_ADD_LED, HIGH);
      ledState = true;
      lastToggle = now;
    }
  }
}

#endif