#include "HiwonderBoard.h"
#if defined(BOARD_HIWONDER_ESP32_S3)
#include <Wire.h>
#include <TFT_eSPI.h>
#include "shared.h"

extern TFT_eSPI tft;

namespace HiwonderBoard {
namespace {
constexpr uint8_t kExpanderAddress = 0x20;
constexpr uint8_t kTouchAddress = 0x38;
constexpr uint8_t kKeysMask = 0xF0;
constexpr uint8_t kBacklightMask = 0x40;
bool expanderReady = false;
bool expanderAttempted = false;
bool touchReady = false;
bool touchAttempted = false;
bool backlightOn = false;

bool readRegisters(TwoWire& bus, uint8_t address, uint8_t reg,
                   uint8_t* data, size_t count) {
  bus.beginTransmission(address);
  bus.write(reg);
  if (bus.endTransmission(false) != 0) return false;
  if (bus.requestFrom(address, count, true) != count) return false;
  for (size_t i = 0; i < count; ++i) data[i] = bus.read();
  return true;
}

bool updateExpander(uint8_t reg, uint8_t mask, uint8_t bits) {
  uint8_t current;
  if (!readRegisters(Wire1, kExpanderAddress, reg, &current, 1)) return false;
  const uint8_t updated = (current & ~mask) | (bits & mask);
  if (updated == current) return true;
  Wire1.beginTransmission(kExpanderAddress);
  Wire1.write(reg);
  Wire1.write(updated);
  return Wire1.endTransmission() == 0;
}
}

bool begin() {
  if (expanderAttempted) return expanderReady;
  expanderAttempted = true;
  // Never initialize the vendor's whole-board configuration: unrelated speaker,
  // camera and sensor bits must retain their state. GPIO40 IRQ is not needed.
  if (Wire1.begin(HIWONDER_EXPANDER_SDA, HIWONDER_EXPANDER_SCL, 400000)) {
    Wire1.setTimeOut(25);
    expanderReady = updateExpander(6, kKeysMask, kKeysMask) &&
                    updateExpander(4, kKeysMask, 0) &&
                    updateExpander(3, kBacklightMask, 0) &&
                    updateExpander(7, kBacklightMask, 0);
  }
  Serial.printf("[Hiwonder] XL9555 0x20 on 38/48: %s\n",
                expanderReady ? "ready" : "unavailable");
  return expanderReady;
}

bool buttonsReady() { return expanderReady; }

bool buttonPressed(uint8_t bit) {
  if (!expanderReady || bit < 4 || bit > 7) return false;
  // All navigation calls in a UI pass share one sample. Debounce press and
  // release transitions while treating a failed I2C read as released.
  static uint32_t lastPoll = 0;
  static uint32_t candidateSince = 0;
  static uint8_t candidate = 0xFF;
  static uint8_t stable = 0xFF;
  const uint32_t now = millis();
  if (now - lastPoll >= 10) {
    lastPoll = now;
    uint8_t inputs = 0xFF;
    if (!readRegisters(Wire1, kExpanderAddress, 0, &inputs, 1)) {
      stable = candidate = 0xFF;
      candidateSince = now;
    } else {
      // Sensor IRQ inputs share port 0; only KEY1..KEY4 affect debounce.
      inputs |= uint8_t(~kKeysMask);
      if (inputs != candidate) { candidate = inputs; candidateSince = now; }
      if (now - candidateSince >= 20) stable = candidate;
    }
  }
  return (stable & (1U << bit)) == 0;
}

bool setBacklight(bool on) {
  if (!expanderReady) return false;
  if (on == backlightOn) return true;
  if (!updateExpander(3, kBacklightMask, on ? kBacklightMask : 0)) return false;
  backlightOn = on;
  return true;
}

void configurePanel() {
  // Vendor ST7789 power/gamma initialization, after TFT_eSPI software reset.
  // The BSP supplies no valid LCD reset GPIO; do not drive a guessed pin.
  struct Command { uint8_t command, length, data[14]; };
  static const Command commands[] = {
    {0xB2, 5, {0x0C, 0x0C, 0x00, 0x33, 0x33}},
    {0xB7, 1, {0x35}}, {0xBB, 1, {0x19}}, {0xC0, 1, {0x2C}},
    {0xC2, 1, {0x01}}, {0xC3, 1, {0x12}}, {0xC4, 1, {0x20}},
    {0xC6, 1, {0x0F}}, {0xD0, 2, {0xA4, 0xA1}},
    {0xE0, 14, {0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23}},
    {0xE1, 14, {0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23}}
  };
  tft.writecommand(0x28);
  for (const auto& command : commands) {
    tft.writecommand(command.command);
    for (uint8_t i = 0; i < command.length; ++i) tft.writedata(command.data[i]);
  }
  tft.setRotation(TFT_ROTATION);
  tft.invertDisplay(true);
  tft.writecommand(0x29);
  delay(120);
  Serial.println("[Hiwonder] ST7789 portrait 240x320 rotation 2 initialized");
}

bool beginTouch() {
  if (touchAttempted) return touchReady;
  touchAttempted = true;
  if (Wire.begin(HIWONDER_TOUCH_SDA, HIWONDER_TOUCH_SCL, 400000)) {
    Wire.setTimeOut(25);
    // Touch reset is not specified by the vendor; allow its power-on reset.
    delay(120);
    uint8_t id = 0;
    if (readRegisters(Wire, kTouchAddress, 0xA8, &id, 1)) {
      touchReady = id == 0x11 || id == 0x64;
      Serial.printf("[Hiwonder] FT6336 0x38 on 4/5: ID 0x%02X, %s\n",
                    id, touchReady ? "ready" : "unrecognized");
    }
  }
  if (!touchReady) Serial.println("[Hiwonder] Touch unavailable; use KEY1..KEY4");
  return touchReady;
}

bool readTouch(int16_t& x, int16_t& y) {
  if (!touchReady) return false;
  uint8_t data[5] = {};
  if (!readRegisters(Wire, kTouchAddress, 0x02, data, sizeof(data))) return false;
  const uint8_t contacts = data[0] & 0x0F;
  if (contacts < 1 || contacts > 2 || (data[1] >> 6) == 1) return false;
  const uint16_t rawX = (uint16_t(data[1] & 0x0F) << 8) | data[2];
  const uint16_t rawY = (uint16_t(data[3] & 0x0F) << 8) | data[4];
  if (rawX >= 240 || rawY >= 320) return false;
  // FT6336 reports absolute panel pixels. This is not XPT2046 ADC calibration.
  switch (TFT_ROTATION & 3) {
    case 0: x = rawX; y = rawY; break;
    case 1: x = rawY; y = 239 - rawX; break;
    case 2: x = 239 - rawX; y = 319 - rawY; break;
    default: x = 319 - rawY; y = rawX; break;
  }
  return true;
}
}
#endif
