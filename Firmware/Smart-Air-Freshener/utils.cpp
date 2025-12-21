#include "config.h"
#include "utils.h"
#include <GyverPower.h>

bool checkSprayMode(SprayMode currentMode) {
  static SprayMode lastSprayMode = MODE_MANUAL;

  if (currentMode != lastSprayMode) {
    lastSprayMode = currentMode;
    return true;
  }
  return false;
}

SprayMode getCurrentMode() {
  bool d7 = (digitalRead(PIN_SW_SPRAY_1) == LOW);
  bool d8 = (digitalRead(PIN_SW_SPRAY_2) == LOW);

  if (d7 && d8) return MODE_3;  // 3 пшика
  if (d8) return MODE_2;        // 2 пшика
  if (d7) return MODE_1;        // 1 пшик
  return MODE_MANUAL;           // ручной режим
}

void disableOutputPins() {
  digitalWrite(PIN_ADD_LED, LOW);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_LED_BUILTIN, LOW);
  digitalWrite(PIN_KEY_BAT_EN, LOW);
}

void enableInputPullups() {
  pinMode(PIN_SW_SPRAY_1, INPUT_PULLUP);
  pinMode(PIN_SW_SPRAY_2, INPUT_PULLUP);
  pinMode(PIN_SW_MODE, INPUT_PULLUP);
  pinMode(PIN_SW_GLOBAL_EN, INPUT_PULLUP);
}

void disableInputPullups() {
  pinMode(PIN_SW_SPRAY_1, INPUT);
  pinMode(PIN_SW_SPRAY_2, INPUT);
  pinMode(PIN_SW_MODE, INPUT);
  pinMode(PIN_SW_GLOBAL_EN, INPUT);
}

void disableHardware() {
  power.hardwareDisable(PWR_UART0 | PWR_TIMER1 | PWR_I2C | PWR_SPI | PWR_USB);
}

void enableADC() {
  power.hardwareEnable(PWR_ADC);
}

void disableADC() {
  power.hardwareDisable(PWR_ADC);
}

void beepStart() {
  power.hardwareEnable(PWR_TIMER2);
  tone(PIN_BUZZER, FREQ_SQUEAKER);
}

void beepStop() {
  noTone(PIN_BUZZER);
  power.hardwareDisable(PWR_TIMER2);
}