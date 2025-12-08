#include "config.h"
#include "state.h"
#include "leds.h"
#include "spray.h"
#include <Arduino.h>

State currentState = STATE_IDLE;
Bounce button;

uint32_t tLightOn = 0;
uint32_t tBlockStart = 0;
uint32_t tBlink = 0;  // таймер для мигания
uint32_t buttonPressStartTime = 0;
uint32_t tBeepStart = 0;

bool isAutoMode = false;
bool isSpray = false;
bool isButtonPressed = false;
bool isReadyCancelInSession = false;
bool isButtonSpray = false;

// Глобальные для мигания
static uint32_t lastBlinkToggle = 0;
static bool isBlinkState = false;  // false = выкл, true = вкл

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

void initStateMachine() {
  currentState = STATE_IDLE;
  button.attach(PIN_BUTTON, INPUT_PULLUP);
  button.interval(50);
}

// Обновлённая функция мигания
void updateBlinkLed(LedColor red, LedColor green, LedColor blue) {
  uint32_t now = millis();
  uint32_t elapsed = now - lastBlinkToggle;

  if (!isBlinkState && elapsed >= LED_BLINK_OFF_CONFIRM_MODE_MS) {
    isBlinkState = true;
    lastBlinkToggle = now;
    updateLed(red, green, blue);
  } else if (isBlinkState && elapsed >= LED_BLINK_ON_CONFIRM_MODE_MS) {
    isBlinkState = false;
    lastBlinkToggle = now;
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
  }
}

void resetState() {
  tLightOn = 0;
  tBlockStart = 0;
  tBeepStart = 0;
  lastBlinkToggle = 0;
  buttonPressStartTime = 0;
  isAutoMode = false;
  isSpray = false;
  isButtonPressed = false;
  isBlinkState = false;
  isReadyCancelInSession = false;
}

void updateStateMachine() {
  static SprayMode lastMode = MODE_MANUAL;
  static bool isLastSprayOnLightOn = false;

  SprayMode currentMode = getCurrentMode();
  uint32_t now = millis();
  bool isLight = isLightOn();
  bool isAuto = getCurrentMode() != MODE_MANUAL;
  bool isSprayOnLightOn = digitalRead(PIN_MODE) == LOW;  // при срабатывании таймера пшик после выключения света или сразу
  bool isUpdateUI = isAutoMode != isAuto || lastMode != currentMode || isLastSprayOnLightOn != isSprayOnLightOn;
  bool isStateReady = currentState == STATE_READY;
  bool isStateBeep = currentState == STATE_RESET_BEEP;
  bool isStateSpray = currentState == STATE_SPRAY;
  bool isStateBlocked = currentState == STATE_BLOCKED;

  if (isUpdateUI && !isStateSpray) {
    resetState();
    currentState = STATE_IDLE;
    lastMode = currentMode;
    isLastSprayOnLightOn = isSprayOnLightOn;
    isAutoMode = isAuto;
  }

  if (!isStateSpray) {
    button.update();
  }

  // --------------------------
  // КНОПКА
  // --------------------------
  if (button.fell()) {
    isSpray = false;

    if (isStateBlocked) {
      isButtonPressed = true;
      buttonPressStartTime = now;
    } else if (isStateReady && isAuto) {
      // Отмена готовности в автоматическом режиме
      isReadyCancelInSession = true;
      currentState = STATE_LIGHT_WAIT;
      tLightOn = now;
      isBlinkState = false;  // сброс мигания
    } else {
      // Обычный пшик (ручной режим или другие состояния)
      currentState = STATE_SPRAY;
      isButtonPressed = false;
      isButtonSpray = true;
    }
  }
  if (button.rose()) {
    isButtonPressed = false;
  }

  // --------------------------
  // СБРОС ПИСКОМ
  // --------------------------
  if (isStateBeep) {
    if (now - tBeepStart >= RESET_BEEP_DURATION_MS) {
      noTone(PIN_BUZZER);
      currentState = STATE_IDLE;
    }
    return;
  }

  // --------------------------
  // РАСПЫЛЕНИЕ
  // --------------------------
  if (isStateSpray) {
    updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);

    if (runSpray()) {
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);

      if (isButtonSpray) {
        isButtonSpray = false;
        resetState();
        currentState = STATE_IDLE;
        return;
      }

      tBlockStart = millis();
      currentState = STATE_BLOCKED;
    }
    return;
  }

  // --------------------------
  // СМЕНА РЕЖИМА
  // --------------------------
  if (!isStateSpray && isAuto && !isAutoMode && isLight) {
    tLightOn = now;
  }
  if (!isAuto && isAutoMode && isStateReady) {
    currentState = STATE_LIGHT_WAIT;
    tLightOn = now;
    isBlinkState = false;
  }
  isAutoMode = isAuto;

  // --------------------------
  // БЛОКИРОВКА
  // --------------------------
  if (isStateBlocked) {
    if (isLight) {
      // Мигание красного с новыми таймингами
      updateBlinkLed(LED_RED_ON, LED_GREEN_OFF, LED_BLUE_OFF);
    } else {
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    if (now - tBlockStart >= BLOCK_MS) {
      currentState = STATE_IDLE;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    } else if (isButtonPressed && (now - buttonPressStartTime >= BLOCK_RESET_HOLD_MS)) {
      currentState = STATE_RESET_BEEP;
      tBeepStart = now;
      tone(PIN_BUZZER, FREQ_SQUEAKER);
      isButtonPressed = false;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }
    return;
  }

  // --------------------------
  // СВЕТ ВЫКЛЮЧЕН
  // --------------------------
  if (!isLight) {
    tBlink = now;
    isSpray = false;
    isReadyCancelInSession = false;
    isBlinkState = false;

    if (currentState != STATE_SPRAY) {
      if (isSprayOnLightOn && isStateReady) {
        currentState = STATE_SPRAY;
      } else {
        currentState = STATE_IDLE;
      }
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
      isBlinkState = false;
      currentState = STATE_LIGHT_WAIT;
      break;

    case STATE_LIGHT_WAIT:
      if (now - tLightOn < LIGHT_READY_MS) {
        // Мигание синего с новыми таймингами
        updateBlinkLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
      } else {
        if (!isReadyCancelInSession && !isSpray && isAuto) {
          currentState = STATE_READY;
          updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_ON);
          tone(PIN_BUZZER, FREQ_SQUEAKER);
          delay(READY_BEEP_MS);
          noTone(PIN_BUZZER);
          if (!isSprayOnLightOn) {
            currentState = STATE_SPRAY;
            isSpray = true;
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