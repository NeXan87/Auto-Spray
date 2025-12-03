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
bool wasSpray = false;
bool buttonPressed = false;
bool readyWasCanceledInThisSession = false;

inline bool isLightOn() {
  static uint32_t lastRead = 0;
  static bool cachedValue = false;
  uint32_t now = millis();
  if (now - lastRead >= LIGHT_READ_INTERVAL_MS) {
    cachedValue = (digitalRead(PIN_LIGHT) == LOW);
    lastRead = now;
  }
  return cachedValue;
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
  // КНОПКА: запуск ТОЛЬКО если не в блокировке и не в распылении
  // --------------------------
  if (button.fell()) {
    wasSpray = false;

    if (currentState == STATE_BLOCKED) {
      buttonPressed = true;
      buttonPressStartTime = now;
    } else if (currentState == STATE_READY && isAutoModeEnabled()) {
      buttonPressed = true;
      readyWasCanceledInThisSession = true;
      currentState = STATE_LIGHT_WAIT;
      tLightOn = now;
    } else {
      buttonPressed = false;
      currentState = STATE_SPRAY;
    }
  }
  if (button.rose()) {
    buttonPressed = false;
  }

  // --------------------------
  // ПИСК ПРИ СБРОСЕ БЛОКИРОВКИ
  // --------------------------
  if (currentState == STATE_RESET_BEEP) {
    if (now - tBeepStart >= RESET_BEEP_DURATION_MS) {
      noTone(PIN_BUZZER);
      currentState = STATE_IDLE;
    }
    return;
  }

  // --------------------------
  // РАСПЫЛЕНИЕ (единственное место, где запускается мотор)
  // --------------------------
  if (currentState == STATE_SPRAY) {
    updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);

    tBlockStart = millis();
    runSpray();

    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    currentState = STATE_BLOCKED;
    return;
  }

  // --- Смена режима ---
  if (isAuto && !wasAutoMode && isLight) {
    tLightOn = now;
  }
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
      if (now - tBlink >= LED_BLINK_MS) {
        tBlink = now;
        static bool blink = false;
        blink = !blink;
        updateLed(blink ? LED_RED_ON : LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
      }
    } else {
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    // Автоматический выход из блокировки
    if (now - tBlockStart >= BLOCK_MS) {
      currentState = STATE_IDLE;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }
    // Сброс удержанием
    else if (buttonPressed && (now - buttonPressStartTime >= BLOCK_RESET_HOLD_MS)) {
      currentState = STATE_RESET_BEEP;
      tBeepStart = now;
      tone(PIN_BUZZER, 1000);
      buttonPressed = false;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    return;
  }

  // --------------------------
  // СВЕТ ВЫКЛЮЧЕН
  // --------------------------
  if (!isLight) {
    tBlink = now;
    wasSpray = false;
    readyWasCanceledInThisSession = false;

    if (AUTO_SPRAY_ON_LIGHT_OFF && currentState == STATE_READY) {
      currentState = STATE_SPRAY;
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
      if (now - tLightOn < LIGHT_READY_MS) {
        if (now - tBlink >= LED_BLINK_MS) {
          tBlink = now;
          static bool blink = false;
          blink = !blink;
          updateLed(LED_RED_OFF, LED_GREEN_OFF, blink ? LED_BLUE_ON : LED_BLUE_OFF);
        }
      } else {
        if (!readyWasCanceledInThisSession && !wasSpray && isAuto) {
          currentState = STATE_READY;
          updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
          tone(PIN_BUZZER, 1000);
          delay(READY_BEEP_MS);
          noTone(PIN_BUZZER);

          if (!AUTO_SPRAY_ON_LIGHT_OFF) {
            currentState = STATE_SPRAY;
            wasSpray = true;
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