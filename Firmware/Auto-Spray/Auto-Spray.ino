/********************************************************************
 * Smart Air Freshener
 * Platform: Arduino Pro Mini (ATmega328P 8MHz, 3.3V)
 ********************************************************************/

#include "config.h"
#include "leds.h"
#include "spray.h"
#include "state.h"

#if ENABLE_SLEEP_MODE
#include "sleep.h"
#endif

void runStartupSequence() {
  digitalWrite(PIN_BUZZER, HIGH);  // Включаем писк

  updateLed(LED_RED_ON, LED_GREEN_OFF, LED_BLUE_OFF);
  delay(TIME_STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);
  delay(TIME_STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
  delay(TIME_STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
  delay(TIME_STARTUP_DELAY_MS);

  digitalWrite(PIN_BUZZER, LOW);  // Выключаем писк
}

// -----------------------------------------------------------
// SETUP
// -----------------------------------------------------------
void setup() {
  pinMode(PIN_LIGHT, INPUT);
  pinMode(PIN_MODE, INPUT_PULLUP);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);

  runStartupSequence();
  initStateMachine();

#if ENABLE_SLEEP_MODE
  initSleepMode();
#endif
}

// -----------------------------------------------------------
// LOOP
// -----------------------------------------------------------
void loop() {
  updateStateMachine();

#if ENABLE_SLEEP_MODE
  bool lightOn = (digitalRead(PIN_LIGHT) == HIGH);
  bool isBlocked = (currentState == STATE_BLOCKED);
  maybeSleep(lightOn, isBlocked);
#endif
}
