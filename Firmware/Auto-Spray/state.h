#pragma once

#include "config.h"
#include <Bounce2.h>

void initStateMachine();
void updateStateMachine();

extern State currentState;
extern Bounce button;
