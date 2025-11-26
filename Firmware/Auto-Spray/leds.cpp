#include "config.h"
#include "leds.h"
#include <Arduino.h>

void updateLed(LedColor red, LedColor green, LedColor blue) {
#if LED_COMMON_ANODE
  // Общий анод: LOW = включить, HIGH = выключить
  digitalWrite(PIN_LED_R, (red == LED_RED_ON) ? LOW : HIGH);
  digitalWrite(PIN_LED_G, (green == LED_GREEN_ON) ? LOW : HIGH);
  digitalWrite(PIN_LED_B, (blue == LED_BLUE_ON) ? LOW : HIGH);
#else
  // Общий катод: HIGH = включить, LOW = выключить
  digitalWrite(PIN_LED_R, (red == LED_RED_ON) ? HIGH : LOW);
  digitalWrite(PIN_LED_G, (green == LED_GREEN_ON) ? HIGH : LOW);
  digitalWrite(PIN_LED_B, (blue == LED_BLUE_ON) ? HIGH : LOW);
#endif
}