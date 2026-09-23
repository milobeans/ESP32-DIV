#pragma once
#include <cstddef>
#include <cstdint>
extern uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
inline void delay(uint32_t duration) { fakeMillis += duration; }
struct SerialMock {
  template<typename... Args> void printf(const char*, Args...) {}
  void println(const char*) {}
};
extern SerialMock Serial;
