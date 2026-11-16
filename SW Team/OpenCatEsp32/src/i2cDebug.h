// I2C debugging helpers.
// Switch on with `#define I2C_DEBUG` in OpenCatEsp32.ino, before `#include "src/OpenCat.h"`.
// Everything is printed to the USB serial monitor (115200 baud), prefixed with "[I2C]" so it is easy to filter.
//
// What it tells you:
// - the idle level of SDA and SCL before Wire starts (both must read 1; SDA = 0 means a device holds the bus),
// - the result code of every address that does not simply NACK during the boot scan,
// - each step of the Grove Vision AI V2 set-up and its return code,
// - failed AI.invoke() calls while running, summarised once per second so the log stays readable,
// - every 2 s, whether SDA or SCL is stuck low.
#pragma once

#ifdef I2C_DEBUG

// The ESP32 Arduino core's default Wire pins, used by Wire.begin() without arguments.
#define I2C_DEBUG_SDA SDA
#define I2C_DEBUG_SCL SCL
#define I2C_DEBUG_MONITOR_MS 2000

// Wire.endTransmission() result codes in the ESP32 Arduino core.
const char *i2cErrorName(uint8_t code) {
  switch (code) {
    case 0: return "ok";
    case 1: return "data too long for buffer";
    case 2: return "NACK on address (no device)";
    case 3: return "NACK on data";
    case 4: return "other error (bus busy or arbitration lost)";
    case 5: return "timeout (a device may be holding SCL or SDA low)";
    default: return "unknown code";
  }
}

void i2cPrintLineLevels(const char *when) {
  int sda = digitalRead(I2C_DEBUG_SDA);
  int scl = digitalRead(I2C_DEBUG_SCL);
  Serial.printf("[I2C] %s: SDA(GPIO%d)=%d SCL(GPIO%d)=%d", when, I2C_DEBUG_SDA, sda, I2C_DEBUG_SCL, scl);
  if (sda && scl)
    Serial.println("  idle, OK");
  else if (!sda && scl)
    Serial.println("  SDA held low: a device is stuck mid-transfer, or SDA is shorted to GND. Power-cycle everything; if it stays, unplug devices one by one.");
  else if (sda && !scl)
    Serial.println("  SCL held low: a device is clock-stretching or SCL is shorted to GND.");
  else
    Serial.println("  both low: no pull-up power, or both lines shorted/held.");
}

// Run before Wire.begin(): with the pins as plain inputs, the reading shows only what the pull-ups and devices do.
void i2cCheckBeforeBegin() {
  pinMode(I2C_DEBUG_SDA, INPUT);
  pinMode(I2C_DEBUG_SCL, INPUT);
  delay(2);
  i2cPrintLineLevels("before Wire.begin");
}

// Sample several times so a normal transfer in progress is not mistaken for a stuck line.
void i2cMonitorTick() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < I2C_DEBUG_MONITOR_MS)
    return;
  lastCheck = millis();
  byte sdaLow = 0, sclLow = 0;
  const byte samples = 20;
  for (byte i = 0; i < samples; i++) {
    sdaLow += !digitalRead(I2C_DEBUG_SDA);
    sclLow += !digitalRead(I2C_DEBUG_SCL);
    delayMicroseconds(100);
  }
  if (sdaLow == samples || sclLow == samples)
    i2cPrintLineLevels("STUCK during run");
}

// Counts AI.invoke() failures and prints a summary at most once per second.
void i2cLogInvoke(int result) {
  static unsigned long windowStart = 0;
  static unsigned int okCount = 0, failCount = 0;
  static int lastError = 0;
  if (result == 0)
    okCount++;
  else {
    failCount++;
    lastError = result;
  }
  if (millis() - windowStart >= 1000) {
    if (failCount)
      Serial.printf("[I2C] AI.invoke last 1 s: %u ok, %u failed (last error %d)\n", okCount, failCount, lastError);
    okCount = failCount = 0;
    windowStart = millis();
  }
}

#define I2C_DBG(...) Serial.printf(__VA_ARGS__)

#else
#define I2C_DBG(...)
#endif
