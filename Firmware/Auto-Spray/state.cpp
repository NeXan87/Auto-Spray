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
  static uint32_t lastRead = 0;
  static bool cachedValue = false;
  uint32_t latestNow = millis();

  if (latestNow - lastRead >= LIGHT_READ_INTERVAL_MS) {
    cachedValue = (digitalRead(PIN_LIGHT) == LOW);
    lastRead = latestNow;
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

  // Обработка удержания кнопки (для сброса блокировки)
  if (button.fell() && currentState == STATE_BLOCKED) {
    buttonPressed = true;
    buttonPressStartTime = now;
  }
  if (button.rose()) {
    buttonPressed = false;
  }

  // --------------------------
  // ПИСК ПРИ СБРОСЕ БЛОКИРОВКИ
  // --------------------------
  if (currentState == STATE_RESET_BEEP) {
    if (now - tBeepStart >= RESET_BEEP_DURATION_MS) {
      digitalWrite(PIN_BUZZER, LOW);
      currentState = STATE_IDLE;
    }
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
      if (now - tBlink >= TIME_LED_BLINK_MS) {
        tBlink = now;
        static bool blink = false;
        blink = !blink;
        updateLed(blink ? LED_RED_ON : LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
      }
    } else {
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    // Автоматический выход из блокировки
    if (now - tBlockStart >= TIME_BLOCK_MS) {
      currentState = STATE_IDLE;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }
    // Сброс удержанием
    else if (buttonPressed && (now - buttonPressStartTime >= BLOCK_RESET_HOLD_MS)) {
      currentState = STATE_RESET_BEEP;
      tBeepStart = now;
      digitalWrite(PIN_BUZZER, HIGH);
      buttonPressed = false;
      updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    }

    return;
  }

  // --------------------------
  // РАСПЫЛЕНИЕ (единственное место, где запускается мотор)
  // --------------------------
  if (currentState == STATE_SPRAY) {
    digitalWrite(PIN_MOTOR_POWER_EN, HIGH);
    updateLed(LED_RED_OFF, LED_GREEN_ON, LED_BLUE_OFF);

    tBlockStart = millis();
    runSpray();
    
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    digitalWrite(PIN_MOTOR_POWER_EN, LOW);
    currentState = STATE_BLOCKED;
    return;
  }

  // --------------------------
  // КНОПКА: запуск ТОЛЬКО если не в блокировке и не в распылении
  // --------------------------
  if (button.fell()) {
    if (currentState != STATE_BLOCKED) {
      currentState = STATE_SPRAY;
      buttonPressed = false;
    }
  }

  // --------------------------
  // СВЕТ ВЫКЛЮЧЕН
  // --------------------------
  if (!isLight) {
    tBlink = now;
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
            currentState = STATE_SPRAY;  // ← только состояние!
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