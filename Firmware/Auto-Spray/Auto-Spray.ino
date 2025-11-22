/********************************************************************
 * Smart Air Freshener – Li-ion Battery Version (3.3V)
 * Platform: Arduino Pro Mini (ATmega328P 8MHz)
 * Author: ChatGPT (functional split version)
 ********************************************************************/

#include "config.h"
#include "leds.h"
#include "spray.h"
#include "state.h"

// -----------------------------------------------------------
// SETUP
// -----------------------------------------------------------
void setup() {
  // Инициализация пинов
  pinMode(PIN_LIGHT, INPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_MODE, INPUT_PULLUP);

  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Стартовая последовательность: 1 секунда писка + цикл цветов
  digitalWrite(PIN_BUZZER, HIGH);  // Включаем звук
  updateLed(LED_RED_ON, LED_GREEN_OFF, LED_BLUE_OFF);
  delay(200);  // Красный
  updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);
  delay(200);  // Зелёный
  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
  delay(200);                                           // Синий
  digitalWrite(PIN_BUZZER, LOW);                        // Выключаем звук
  updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);  // Гасим LED

  // Инициализация основной логики
  initStateMachine();
}

// -----------------------------------------------------------
// LOOP
// -----------------------------------------------------------
void loop() {
  updateStateMachine();
}
