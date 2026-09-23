#pragma once
#include "BoardConfig.h"

#if defined(BOARD_HIWONDER_ESP32_S3)
#include <Arduino.h>
#include "HiwonderBoard.h"

// Compatibility with the existing button calls, without sending PCF8574
// transactions to the XL9555 that happens to share the same I2C address.
class BoardButtonExpander {
public:
  explicit BoardButtonExpander(uint8_t) {}
  bool begin(uint8_t = 0x20) { return HiwonderBoard::begin(); }
  void pinMode(uint8_t, uint8_t) {}  // Directions are configured once with masks.
  bool digitalRead(uint8_t pin) { return !HiwonderBoard::buttonPressed(pin); }
};
#else
#include <PCF8574.h>
using BoardButtonExpander = PCF8574;
#endif
