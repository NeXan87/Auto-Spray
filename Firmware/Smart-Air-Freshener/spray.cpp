#include "config.h"
#include "spray.h"
#include "utils.h"

bool isSpraying = false;

SprayMode pulseCount = 0;
static SprayStep sprayStep = FORWARD;
static uint8_t sprayPulse = 0;
static uint32_t sprayTimer = 0;

void resetSpray() {
  isSpraying = false;
  sprayStep = FORWARD;
  sprayPulse = 0;
  sprayTimer = 0;
}

bool runSpray() {
  uint32_t now = millis();

  if (!isSpraying) {
    isSpraying = true;
    pulseCount = getCurrentMode();
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
          resetSpray();
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