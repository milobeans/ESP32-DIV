#pragma once
#include "Arduino.h"
#include <array>
#include <vector>
#include <cassert>
struct RegisterWrite { uint8_t reg, before, after; };
class TwoWire {
 public:
  explicit TwoWire(uint8_t address) : expectedAddress(address) {}
  std::array<uint8_t, 256> registers{};
  std::vector<RegisterWrite> writes;
  uint8_t expectedAddress;
  bool beginOk = true, nack = false, failWrites = false, shortRead = false;
  int probeFailures = 0, shortReadsRemaining = 0, endCalls = 0;
  std::vector<uint32_t> beginFrequencies;
  int sda = -1, scl = -1, beginCalls = 0, readCalls = 0, timeout = 0;
  uint32_t frequency = 0;
  bool begin(int data, int clock, uint32_t hz) {
    ++beginCalls; sda = data; scl = clock; frequency = hz;
    beginFrequencies.push_back(hz); return beginOk;
  }
  bool end() { ++endCalls; return true; }
  void setTimeOut(int milliseconds) { timeout = milliseconds; }
  void beginTransmission(uint8_t address) { assert(address == expectedAddress); tx.clear(); }
  size_t write(uint8_t value) { tx.push_back(value); return 1; }
  uint8_t endTransmission(bool = true) {
    if (nack || (failWrites && tx.size() > 1)) return 2;
    if (tx.empty()) {
      if (probeFailures > 0) { --probeFailures; return 2; }
      return 0;
    }
    assert(tx.size() == 1 || tx.size() == 2);
    selected = tx[0];
    if (tx.size() == 2) {
      writes.push_back({selected, registers[selected], tx[1]});
      registers[selected] = tx[1];
    }
    return 0;
  }
  size_t requestFrom(uint8_t address, size_t count, bool) {
    assert(address == expectedAddress); ++readCalls;
    readPosition = selected;
    if (shortReadsRemaining > 0) { --shortReadsRemaining; return count ? count - 1 : 0; }
    return shortRead && count ? count - 1 : count;
  }
  int read() { return registers[readPosition++]; }
 private:
  std::vector<uint8_t> tx;
  uint8_t selected = 0, readPosition = 0;
};
extern TwoWire Wire, Wire1;
