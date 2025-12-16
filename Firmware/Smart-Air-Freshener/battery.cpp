#include "config.h"
#include "battery.h"
#include "leds.h"
#include "state.h"

uint16_t vbat = 9999;

uint16_t readBatteryVoltage() {
  digitalWrite(PIN_BATTERY_EN, HIGH);
  delayMicroseconds(5);

  uint16_t adc = analogRead(PIN_BATTERY_POINT);
  float vout_mv = adc * VCC_ARDUINO / 1023;
  uint16_t vbat_mv = (uint16_t)(vout_mv * BATTERY_DIVIDER_RATIO);

  digitalWrite(PIN_BATTERY_EN, LOW);
  return vbat_mv;
}

void updateBattery(bool isLightOn) {
  static uint32_t lastBatteryCheck = 0;
  static bool isBatteryLow = false;

  if (isLightOn && millis() - lastBatteryCheck >= BATTERY_CHECK_INTERVAL_MS) {
    vbat = readBatteryVoltage();

    if (vbat <= BATTERY_LOW_MV) {
      isBatteryLow = true;
    } else {
      isBatteryLow = false;
    }

    lastBatteryCheck = millis();
  }

  if (isLightOn) {
    if (isBatteryLow) {
      updateBatteryLed();
    } else {
      if (digitalRead(PIN_ADD_LED) == HIGH) {
        digitalWrite(PIN_ADD_LED, LOW);
      }
    }
  } else {
    digitalWrite(PIN_ADD_LED, LOW);
  }
}

bool isBatLow() {
  return false;  // для дебага
  // return vbat <= BATTERY_BLOCKED_MV;
}