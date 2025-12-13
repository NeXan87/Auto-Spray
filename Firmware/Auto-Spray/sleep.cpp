#include "config.h"

#if ENABLE_SLEEP_MODE

#include "sleep.h"
#include <GyverPower.h>

void initSleepMode() {
  // power.hardwareDisable(PWR_UART0 | PWR_TIMER0 | PWR_TIMER1 | PWR_TIMER2);
  power.setSleepMode(POWERDOWN_SLEEP);
}

void wakeUp() {
  // power.wakeUp();
}

void sleepWDT() {
  power.sleep(SLEEP_1024MS);
}

void maybeSleep(bool lightOn, bool isNotSpray, bool isNotBlocked) {
  // Спать можно ТОЛЬКО если:
  // - свет ВЫКЛЮЧЕН
  // - не пшикает
  // - нет блокировки
  if (!lightOn && isNotSpray && isNotBlocked) {
    attachInterrupt(
      digitalPinToInterrupt(PIN_BUTTON), wakeUp, FALLING);  // D2 — кнопка
    attachInterrupt(
      digitalPinToInterrupt(PIN_LIGHT), wakeUp, CHANGE);  // D3 — датчик света

    power.sleep(SLEEP_FOREVER);

    detachInterrupt(digitalPinToInterrupt(PIN_BUTTON));
    detachInterrupt(digitalPinToInterrupt(PIN_LIGHT));
  }
}

#endif