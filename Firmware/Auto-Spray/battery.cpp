#include "config.h"
#include "battery.h"
#include "leds.h"

uint16_t readBatteryVoltage() {
  digitalWrite(PIN_BATTERY_EN, HIGH);
  delayMicroseconds(5);

  uint16_t adc = analogRead(PIN_BATTERY_POINT);

  digitalWrite(PIN_BATTERY_EN, LOW);

  float vout_mv = (adc * VCC_ARDUINO) / 1023;
  uint16_t vbat_mv = (uint16_t)(vout_mv * BATTERY_DIVIDER_RATIO);
  return vbat_mv;
}

void updateBattery(bool isLightOn) {
  static uint32_t lastBatteryCheck = 0;
  static bool isBatteryLow = false;

  if (isLightOn && millis() - lastBatteryCheck >= BATTERY_CHECK_INTERVAL_MS) {
    uint16_t vbat = readBatteryVoltage();

    if (vbat <= BATTERY_LOW_VOLTAGE_MV) {
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