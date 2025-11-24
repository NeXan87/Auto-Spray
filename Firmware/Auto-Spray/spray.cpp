#include "config.h"
#include "spray.h"

void startSpray() {
  // Гарантированная остановка перед стартом
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
}

bool runSpray() {
  for (uint8_t i = 0; i < SPRAY_PULSE_COUNT; i++) {
    // Включаем мотор вперёд
    digitalWrite(PIN_MOTOR_IN1, HIGH);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    
    delay(SPRAY_ON_MS);
    
    // Останавливаем
    digitalWrite(PIN_MOTOR_IN1, LOW);
    digitalWrite(PIN_MOTOR_IN2, LOW);
    
    // Пауза между пшиками (кроме последнего)
    if (i < SPRAY_PULSE_COUNT - 1) {
      delay(SPRAY_OFF_MS);
    }
  }
  return true;
}
