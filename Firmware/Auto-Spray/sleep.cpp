#include "config.h"

#if ENABLE_SLEEP_MODE

#include "sleep.h"
#include "state.h"
#include "leds.h"
#include <GyverPower.h>

void initSleepMode() {
  // power.hardwareDisable(PWR_UART0 | PWR_TIMER0 | PWR_TIMER1 | PWR_TIMER2);
  power.setSleepMode(POWERDOWN_SLEEP);
}

void wakeUp() {
  // power.wakeUp();
}

void sleepWDT(int time) {
  power.sleep(time);
}

void maybeSleep(bool isLightOn) {
  // Спать можно ТОЛЬКО если:
  // - свет ВЫКЛЮЧЕН
  // - не пшикает
  // - нет блокировки
  bool isIdle = (currentState == STATE_IDLE);

  if (!isLightOn && isIdle) {
    attachInterrupt(
      digitalPinToInterrupt(PIN_BUTTON), wakeUp, FALLING);  // D2 — кнопка
    attachInterrupt(
      digitalPinToInterrupt(PIN_LIGHT), wakeUp, CHANGE);  // D3 — датчик света

    disableOutputPins();
    updateLed(LED_RED_OFF, LED_GREEN_OFF, LED_BLUE_OFF);
    power.sleep(SLEEP_FOREVER);

    detachInterrupt(digitalPinToInterrupt(PIN_BUTTON));
    detachInterrupt(digitalPinToInterrupt(PIN_LIGHT));
  }
}

#endif