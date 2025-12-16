#include "config.h"
#include "utils.h"

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
  digitalWrite(PIN_BATTERY_EN, HIGH);
}