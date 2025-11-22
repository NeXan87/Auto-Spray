#include "config.h"

#if ENABLE_SLEEP_MODE

#include "sleep.h"
#include <avr/sleep.h>
#include <avr/power.h>
#include <Arduino.h>

void initSleepMode() {
  // Настройка прерываний: оба пина пробуждают из POWER_DOWN
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), [](){}, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_LIGHT),  [](){}, CHANGE);
}

void maybeSleep(bool lightOn, bool isBlocked) {
  // Спать можно ТОЛЬКО если:
  // - свет ВЫКЛЮЧЕН
  // - нет блокировки
  if (!lightOn && !isBlocked) {
    // Отключаем АЦП для минимизации потребления
    ADCSRA &= ~(1 << ADEN);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();        // МК засыпает здесь
    sleep_disable();    // После пробуждения — продолжаем

    // Включаем АЦП обратно (на случай, если другие модули его используют)
    ADCSRA |= (1 << ADEN);
  }
}

#endif