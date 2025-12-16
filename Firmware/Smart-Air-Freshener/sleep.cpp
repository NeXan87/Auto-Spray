#include "config.h"

#if ENABLE_SLEEP_MODE

#include "sleep.h"
#include "state.h"
#include "leds.h"
#include <GyverPower.h>

void initSleepMode() {
  power.hardwareDisable(PWR_I2C | PWR_SPI | PWR_TIMER2 | PWR_USB);
  power.setSleepMode(POWERDOWN_SLEEP);
}

void sleepWDT() {
  power.sleep(SLEEP_1024MS);
}

void maybeSleep(bool isLightOn) {
  // Спать можно ТОЛЬКО если:
  // - свет ВЫКЛЮЧЕН
  // - не пшикает
  // - нет блокировки
  bool isIdle = (currentState == STATE_IDLE);

  if (!isLightOn && isIdle) {
    attachInterrupt(
      digitalPinToInterrupt(PIN_BUTTON), []() {}, FALLING);
    attachInterrupt(
      digitalPinToInterrupt(PIN_LIGHT), []() {}, CHANGE);

    disableOutputPins();
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    power.sleep(SLEEP_FOREVER);

    detachInterrupt(digitalPinToInterrupt(PIN_BUTTON));
    detachInterrupt(digitalPinToInterrupt(PIN_LIGHT));
  }
}

#endif