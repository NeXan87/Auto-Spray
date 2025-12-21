#pragma once

#include <Arduino.h>

enum SprayMode {
  MODE_MANUAL,  // положение 1 — всё выключено
  MODE_1,    // положение 2 — 1 пшик
  MODE_2,    // положение 3 — 2 пшика
  MODE_3     // положение 4 — 3 пшика
};

bool checkSprayMode(SprayMode mode);
SprayMode getCurrentMode();
void disableOutputPins();
void disableOutputPins();
void enableInputPullups();
void disableInputPullups();
void disableHardware();
void enableADC();
void disableADC();
void beepStart();
void beepStop();