#pragma once
#include <Arduino.h>
#include "BoardConfig.h"

#if defined(BOARD_HIWONDER_ESP32_S3)
namespace HiwonderBoard {
bool begin();
bool beginTouch();
bool buttonsReady();
bool buttonPressed(uint8_t bit);
bool setBacklight(bool on);
void configurePanel();
bool readTouch(int16_t& x, int16_t& y);
}
#endif
