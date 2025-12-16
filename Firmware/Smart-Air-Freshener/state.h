#pragma once

#include "config.h"
#include "utils.h"
#include <Bounce2.h>

void initStateMachine();
void updateStateMachine(SprayMode currentMode, bool isLightOn);
bool hasLightOn();
void resetState();

extern State currentState;
extern Bounce button;
