// Tests actual board source against bounded I2C fixtures. This does not emulate
// ESP32 timing, radio, visible pixels or physical I2C. Run with run.sh.
#include <cassert>
#include <cstring>
#include <iostream>
#include "Wire.h"
#include "TFT_eSPI.h"
#include "HiwonderBoard.h"
uint32_t fakeMillis = 100;
SerialMock Serial;
TwoWire Wire(0x38), Wire1(0x20);
TFT_eSPI tft;

static void touch(uint16_t x, uint16_t y, uint8_t contacts = 1, uint8_t event = 2) {
  Wire.registers[2] = contacts;
  Wire.registers[3] = (event << 6) | (x >> 8);
  Wire.registers[4] = x & 255;
  Wire.registers[5] = y >> 8;
  Wire.registers[6] = y & 255;
}

static void successfulBoard() {
  Wire1.registers.fill(0xA5);
  Wire1.registers[0] = 0xFF;
  Wire1.registers[3] = 0xD5;
  Wire1.registers[4] = 0xAB;
  Wire1.registers[6] = 0x05;
  Wire1.registers[7] = 0xFF;
  const auto original = Wire1.registers;
  assert(HiwonderBoard::begin());
  assert(HiwonderBoard::buttonsReady());
  assert(Wire1.sda == 38 && Wire1.scl == 48 && Wire1.frequency == 400000 && Wire1.timeout == 25);
  assert(Wire.beginCalls == 0);
  assert(Wire1.registers[6] == 0xF5 && Wire1.registers[4] == 0x0B);
  assert(Wire1.registers[3] == 0x95 && Wire1.registers[7] == 0xBF);
  for (size_t i = 0; i < original.size(); ++i) {
    const uint8_t allowed = (i == 4 || i == 6) ? 0xF0 : (i == 3 || i == 7) ? 0x40 : 0;
    assert(((original[i] ^ Wire1.registers[i]) & ~allowed) == 0);
  }
  const size_t initWrites = Wire1.writes.size();
  assert(HiwonderBoard::begin() && Wire1.beginCalls == 1 && Wire1.writes.size() == initWrites);
  assert(HiwonderBoard::setBacklight(true) && Wire1.registers[3] == 0xD5);
  const size_t enabledWrites = Wire1.writes.size();
  assert(HiwonderBoard::setBacklight(true) && Wire1.writes.size() == enabledWrites);
  Wire1.failWrites = true;
  assert(!HiwonderBoard::setBacklight(false) && Wire1.registers[3] == 0xD5);
  Wire1.failWrites = false;
  assert(HiwonderBoard::setBacklight(false) && Wire1.registers[3] == 0x95);
  for (const auto& w : Wire1.writes) {
    const uint8_t allowed = (w.reg == 4 || w.reg == 6) ? 0xF0 : (w.reg == 3 || w.reg == 7) ? 0x40 : 0;
    assert(((w.before ^ w.after) & ~allowed) == 0 && allowed != 0);
  }

  // Key debounce must survive unrelated sensor input changes on the same port.
  fakeMillis = 1000;
  Wire1.registers[0] = 0xEF;
  assert(!HiwonderBoard::buttonPressed(4));
  fakeMillis += 10; Wire1.registers[0] = 0xED;
  assert(!HiwonderBoard::buttonPressed(4));
  fakeMillis += 10; Wire1.registers[0] = 0xEC;
  assert(HiwonderBoard::buttonPressed(4));
  assert(!HiwonderBoard::buttonPressed(5));
  const int reads = Wire1.readCalls;
  assert(!HiwonderBoard::buttonPressed(3) && !HiwonderBoard::buttonPressed(8));
  assert(Wire1.readCalls == reads);
  Wire1.registers[0] = 0xFF;
  fakeMillis += 10; assert(HiwonderBoard::buttonPressed(4));
  fakeMillis += 20; assert(!HiwonderBoard::buttonPressed(4));
  Wire1.registers[0] = 0xBF;
  fakeMillis += 10; assert(!HiwonderBoard::buttonPressed(6));
  fakeMillis += 20; assert(HiwonderBoard::buttonPressed(6));
  Wire1.nack = true; fakeMillis += 10;
  assert(!HiwonderBoard::buttonPressed(6));
  Wire1.nack = false; Wire1.shortRead = true; fakeMillis += 10;
  assert(!HiwonderBoard::buttonPressed(6));
  Wire1.shortRead = false;

  Wire.registers[0xA8] = 0x64;
  assert(HiwonderBoard::beginTouch());
  assert(Wire.sda == 4 && Wire.scl == 5 && Wire.frequency == 400000 && Wire.timeout == 25);
  assert(HiwonderBoard::beginTouch() && Wire.beginCalls == 1);
  int16_t x = -1, y = -1;
  struct Corner { uint16_t rawX, rawY; int16_t x, y; };
  const Corner corners[] = {{0,0,239,319}, {239,0,0,319}, {0,319,239,0}, {239,319,0,0}};
  for (const auto& p : corners) {
    touch(p.rawX, p.rawY);
    assert(HiwonderBoard::readTouch(x, y) && x == p.x && y == p.y);
  }
  touch(120, 160, 2); assert(HiwonderBoard::readTouch(x, y) && x == 119 && y == 159);
  touch(0, 0, 0); assert(!HiwonderBoard::readTouch(x, y));
  touch(0, 0, 3); assert(!HiwonderBoard::readTouch(x, y));
  touch(10, 10, 1, 1); assert(!HiwonderBoard::readTouch(x, y));
  touch(240, 0); assert(!HiwonderBoard::readTouch(x, y));
  touch(0, 320); assert(!HiwonderBoard::readTouch(x, y));
  touch(4095, 4095); assert(!HiwonderBoard::readTouch(x, y));
  touch(10, 10); Wire.shortRead = true; assert(!HiwonderBoard::readTouch(x, y));
  Wire.shortRead = false; Wire.nack = true; assert(!HiwonderBoard::readTouch(x, y));
  assert(Wire.writes.empty());

  HiwonderBoard::configurePanel();
  assert(tft.commands.front() == 0x28 && tft.commands.back() == 0x29);
  assert(tft.rotation == 2 && tft.inverted);
}

int main(int argc, char** argv) {
  assert(argc == 2);
  const char* scenario = argv[1];
  if (std::strcmp(scenario, "success") == 0) successfulBoard();
  else if (std::strcmp(scenario, "expander-begin-failure") == 0) {
    Wire1.beginOk = false;
    assert(!HiwonderBoard::begin() && !HiwonderBoard::buttonsReady());
    assert(!HiwonderBoard::setBacklight(true) && !HiwonderBoard::buttonPressed(4));
    assert(Wire1.writes.empty() && Wire1.readCalls == 0);
  } else if (std::strcmp(scenario, "expander-nack") == 0) {
    Wire1.nack = true;
    assert(!HiwonderBoard::begin() && !HiwonderBoard::buttonsReady());
    assert(Wire1.writes.empty());
  } else if (std::strcmp(scenario, "touch-begin-failure") == 0) {
    Wire.beginOk = false;
    int16_t x, y;
    assert(!HiwonderBoard::beginTouch() && !HiwonderBoard::readTouch(x, y));
    assert(Wire.readCalls == 0 && Wire.writes.empty());
    assert(Wire.beginCalls == 3 && Wire.endCalls == 2);
  } else if (std::strcmp(scenario, "touch-unknown-id") == 0) {
    Wire.registers[0xA8] = 0x7F;
    int16_t x, y;
    assert(!HiwonderBoard::beginTouch() && !HiwonderBoard::readTouch(x, y));
    assert(Wire.beginCalls == 1);
  } else if (std::strcmp(scenario, "touch-short-id") == 0) {
    Wire.registers[0xA8] = 0x64;
    Wire.shortRead = true;
    assert(!HiwonderBoard::beginTouch());
    assert(Wire.beginCalls == 3 && Wire.endCalls == 2);
  } else if (std::strcmp(scenario, "touch-transient-nack") == 0) {
    Wire.registers[0xA8] = 0x64;
    Wire.probeFailures = 1;
    assert(HiwonderBoard::beginTouch());
    assert(Wire.beginCalls == 2 && Wire.endCalls == 1);
    assert(Wire.beginFrequencies[0] == 400000 && Wire.beginFrequencies[1] == 100000);
    assert(Wire1.beginCalls == 0 && Wire1.writes.empty());
    int16_t x, y;
    touch(0, 0);
    assert(HiwonderBoard::readTouch(x, y) && x == 239 && y == 319);
  } else if (std::strcmp(scenario, "touch-transient-short-read") == 0) {
    Wire.registers[0xA8] = 0x11;
    Wire.shortReadsRemaining = 1;
    assert(HiwonderBoard::beginTouch() && Wire.beginCalls == 2);
    assert(Wire1.beginCalls == 0 && Wire1.writes.empty());
  } else if (std::strcmp(scenario, "touch-persistent-nack") == 0) {
    Wire.nack = true;
    assert(!HiwonderBoard::beginTouch());
    assert(Wire.beginCalls == 3 && Wire.endCalls == 2 && Wire.readCalls == 0);
    assert(!HiwonderBoard::beginTouch() && Wire.beginCalls == 3);
  } else assert(false && "unknown fixture scenario");
  std::cout << "PASS " << scenario << '\n';
}
