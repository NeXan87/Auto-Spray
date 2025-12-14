#pragma once

#if ENABLE_SLEEP_MODE

// Проверяет условия и, если возможно, переводит МК в сон
void maybeSleep(bool isLightOn);
void sleepWDT();
void initSleepMode();

#endif