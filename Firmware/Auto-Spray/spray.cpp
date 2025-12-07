#include "config.h"
#include "spray.h"

bool isSpraying = false;

enum SprayStep {
  FORWARD,
  PAUSE1,
  REVERSE,
  PAUSE2,
  REPEAT
};

static uint8_t sprayPulse = 0;
static SprayStep sprayStep = FORWARD;
static uint32_t sprayTimer = 0;
uint8_t pulseCount = 0;

uint8_t getSprayPulseCount() {
  if (digitalRead(PIN_SPRAY_2) == LOW) return 2;
  if (digitalRead(PIN_SPRAY_3) == LOW) return 3;
  if (digitalRead(PIN_SPRAY_4) == LOW) return 4;
  return 1;  // по умолчанию 1 пшик
}

bool runSpray() {
  uint32_t now = millis();

  if (!isSpraying) {
    isSpraying = true;
    pulseCount = getSprayPulseCount();
  }

  switch (sprayStep) {
    case FORWARD:
      sprayStep = PAUSE1;
      sprayTimer = now;
      digitalWrite(PIN_MOTOR_IN1, HIGH);
      digitalWrite(PIN_MOTOR_IN2, LOW);
      break;

    case PAUSE1:
      if (now - sprayTimer >= SPRAY_ON_MS) {
        sprayStep = REVERSE;
        sprayTimer = now;
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
      }
      break;

    case REVERSE:
      if (now - sprayTimer >= SPRAY_PAUSE1_MS) {
        sprayStep = PAUSE2;
        sprayTimer = now;
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
      }
      break;

    case PAUSE2:
      if (now - sprayTimer >= SPRAY_REVERSE_MS) {
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        if (sprayPulse < pulseCount - 1) {
          sprayStep = REPEAT;
          sprayTimer = now;
        } else {
          isSpraying = false;
          sprayStep = FORWARD;
          sprayPulse = 0;
          return true;
        }
      }
      break;

    case REPEAT:
      if (now - sprayTimer >= SPRAY_PAUSE2_MS) {
        sprayPulse++;
        sprayStep = FORWARD;
        sprayTimer = now;
      }
      break;
  }
  return false;
}