#include "config.h"
#include "state.h"
#include "leds.h"
#include "spray.h"
#include <Arduino.h>

State currentState = STATE_IDLE;
Bounce button;

uint32_t tLightOn = 0;
uint32_t tBlockStart = 0;
uint32_t tBlink = 0;
uint32_t buttonPressStartTime = 0;
uint32_t tBeepStart = 0;

bool wasAutoMode = false;
bool buttonPressed = false;

inline bool isLightOn() {
  return digitalRead(PIN_LIGHT) == LOW;
}

inline bool isAutoModeEnabled() {
  return digitalRead(PIN_MODE) == LOW;
}

void initStateMachine() {
  currentState = STATE_IDLE;
  button.attach(PIN_BUTTON, INPUT_PULLUP);
  button.interval(50);
}

void updateStateMachine() {
  uint32_t now = millis();
  bool isLight = isLightOn();
  bool isAuto = isAutoModeEnabled();

  button.update();

  // --------------------------
  // ПИСК ПРИ СБРОСЕ БЛОКИРОВКИ
  // --------------------------
  if (currentState == STATE_RESET_BEEP) {
    if (now - tBeepStart >= RESET_BEEP_DURATION_MS) {
      digitalWrite(PIN_BUZZER, LOW);  // выключаем писк
      currentState = STATE_IDLE;      // возвращаемся в обычный режим
    }
    return;  // не обрабатываем другие события во время писка
  }

  if (button.fell()) {
    buttonPressed = true;
    buttonPressStartTime = now;
  }
  if (button.rose()) {
    buttonPressed = false;
  }

  // --- Смена режима ---
  if (isAuto && !wasAutoMode && isLight) tLightOn = now;
  if (!isAuto && wasAutoMode && currentState == STATE_READY) {
    currentState = STATE_LIGHT_WAIT;
    tLightOn = now;
  }
  wasAutoMode = isAuto;

  // --------------------------
  // БЛОКИРОВКА
  // --------------------------
  if (currentState == STATE_BLOCKED) {
    if (isLight) {
      if (now - tBlink >= TIME_LED_BLINK_MS) {
        tBlink = now;
        static bool blink = false;
        blink = !blink;
        updateLed(blink ? LED_RED_ON : LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
      }
    } else {
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    if (now - tBlockStart >= TIME_BLOCK_MS) {
      currentState = STATE_IDLE;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    else if (buttonPressed && (now - buttonPressStartTime >= BLOCK_RESET_HOLD_MS)) {
      currentState = STATE_RESET_BEEP;
      tBeepStart = now;
      digitalWrite(PIN_BUZZER, HIGH);  // включаем писк
      buttonPressed = false;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);  // гасим LED
    }

    return;
  }

  // --------------------------
  // КНОПКА
  // --------------------------
  if (button.fell()) {
    if (currentState != STATE_SPRAY) {
      currentState = STATE_SPRAY;
      startSpray();
    }
  }

  // --------------------------
  // РАСПЫЛЕНИЕ
  // --------------------------
  if (currentState == STATE_SPRAY) {
    updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);
    if (runSpray()) {
      tBlockStart = now;
      currentState = STATE_BLOCKED;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }
    return;
  }

  // --------------------------
  // СВЕТ ВЫКЛЮЧЕН
  // --------------------------
  if (!isLight) {
    tBlink = now;
    if (AUTO_SPRAY_ON_LIGHT_OFF && currentState == STATE_READY) {
      currentState = STATE_SPRAY;
      startSpray();
    } else {
      currentState = STATE_IDLE;
    }
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    return;
  }

  // --------------------------
  // СВЕТ ВКЛЮЧЕН
  // --------------------------
  switch (currentState) {
    case STATE_IDLE:
      tLightOn = now;
      tBlink = now;
      currentState = STATE_LIGHT_WAIT;
      break;

    case STATE_LIGHT_WAIT:
      if (now - tLightOn < TIME_LIGHT_READY_MS) {
        if (now - tBlink >= TIME_LED_BLINK_MS) {
          tBlink = now;
          static bool blink = false;
          blink = !blink;
          updateLed(LED_RED_OFF, LED_GREEN_OFF, blink ? LED_BLUE_ON : LED_BLUE_OFF);
        }
      } else {
        if (isAuto) {
          currentState = STATE_READY;
          updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
          digitalWrite(PIN_BUZZER, HIGH);
          delay(TIME_READY_BEEP_MS);
          digitalWrite(PIN_BUZZER, LOW);

          if (!AUTO_SPRAY_ON_LIGHT_OFF) {
            currentState = STATE_SPRAY;
            startSpray();
          }
        } else {
          tLightOn = now;
        }
      }
      break;

    case STATE_READY:
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
      break;
  }
}
