/********************************************************************
 * Smart Air Freshener
 * Platform: Arduino Pro Mini (ATmega328P 8MHz, 3.3V)
 ********************************************************************/

#include "config.h"
#include "state.h"
#include "leds.h"
#include "spray.h"
#include "utils.h"
#include "battery.h"

#if USE_OPT3001
#include "opt3001.h"
#endif

#if ENABLE_SLEEP_MODE
#include "sleep.h"
#include <GyverPower.h>
#endif

void runStartupSequence() {
  tone(PIN_BUZZER, FREQ_SQUEAKER);  // Включаем писк

  digitalWrite(PIN_ADD_LED, HIGH);
  digitalWrite(PIN_LED_BUILTIN, HIGH);
  updateLed(LED_RED_ON, LED_GREEN_OFF, LED_BLUE_OFF);
  delay(STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);
  delay(STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
  delay(STARTUP_DELAY_MS);

  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
  digitalWrite(PIN_ADD_LED, LOW);
  digitalWrite(PIN_LED_BUILTIN, LOW);

  noTone(PIN_BUZZER);  // Выключаем писк
}

// -----------------------------------------------------------
// SETUP
// -----------------------------------------------------------
void setup() {
  // Serial.begin(9600);
#if USE_OPT3001
  pinMode(PIN_LIGHT, INPUT_PULLUP);
  initOpt3001();
#else
  pinMode(PIN_LIGHT, INPUT);
#endif

  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_ADD_LED, OUTPUT);
  pinMode(PIN_BATTERY_EN, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  pinMode(PIN_SW_MODE, INPUT_PULLUP);
  pinMode(PIN_SW_SPRAY_1, INPUT_PULLUP);
  pinMode(PIN_SW_SPRAY_2, INPUT_PULLUP);
  pinMode(PIN_SW_GLOBAL_EN, INPUT_PULLUP);

  disableOutputPins();
  runStartupSequence();
  initStateMachine();
  digitalWrite(PIN_LED_BUILTIN, LOW);

#if ENABLE_SLEEP_MODE
  initSleepMode();
#endif
}

// -----------------------------------------------------------
// LOOP
// -----------------------------------------------------------
void loop() {
  bool isEnabled = digitalRead(PIN_SW_GLOBAL_EN) == LOW;

  if (!isEnabled) {
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    disableOutputPins();
    resetState();

#if ENABLE_SLEEP_MODE
    sleepWDT();
#endif
    return;
  }

  SprayMode currentMode = getCurrentMode();
  bool isLightOn = hasLightOn();

  updateBattery(isLightOn);
  updateStateMachine(currentMode, isLightOn);
  updateSprayMode(currentMode);

#if ENABLE_SLEEP_MODE
  maybeSleep(isLightOn);
#endif

#if ACTIVITY_LED_ENABLED
  updateActivityLed();
#endif
}
