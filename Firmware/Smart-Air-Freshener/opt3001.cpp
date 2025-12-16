#include "opt3001.h"
#include "config.h"

#if USE_OPT3001

void initOpt3001() {
  Wire.begin();

  // 1. Конфигурация: Continuous mode, auto-range, LATCH = 1
  Wire.beginTransmission(OPT3001_I2C_ADDRESS);
  Wire.write(0x01);        // Configuration register
  Wire.write(0b11000101);  // C[15:12]=1100 (continuous), T=000 (800ms), R=1 (auto)
                           // LATCH = 1 (прерывание фиксируется до чтения)
  Wire.write(0x00);
  Wire.endTransmission();

  // 2. Установка порогов
  uint16_t low_thresh = (OPT3001_LIGHT_THRESHOLD > OPT3001_HYSTERESIS) ? (OPT3001_LIGHT_THRESHOLD - OPT3001_HYSTERESIS) : 0;

  // High Limit Register
  Wire.beginTransmission(OPT3001_I2C_ADDRESS);
  Wire.write(0x02);
  Wire.write(OPT3001_LIGHT_THRESHOLD >> 8);
  Wire.write(OPT3001_LIGHT_THRESHOLD & 0xFF);
  Wire.endTransmission();

  // Low Limit Register
  Wire.beginTransmission(OPT3001_I2C_ADDRESS);
  Wire.write(0x03);
  Wire.write(low_thresh >> 8);
  Wire.write(low_thresh & 0xFF);
  Wire.endTransmission();

  // 3. Сброс флага прерывания (на всякий случай)
  Wire.requestFrom((uint8_t)OPT3001_I2C_ADDRESS, (uint8_t)2);
  if (Wire.available() >= 2) {
    (void)Wire.read();
    (void)Wire.read();  // сброс
  }
}

// Опционально: функция для сброса флага прерывания после пробуждения
void clearOpt3001Interrupt() {
  Wire.requestFrom((uint8_t)OPT3001_I2C_ADDRESS, (uint8_t)2);
  if (Wire.available() >= 2) {
    (void)Wire.read();
    (void)Wire.read();
  }
}

#endif