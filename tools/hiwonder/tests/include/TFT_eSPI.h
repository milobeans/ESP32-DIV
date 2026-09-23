#pragma once
#include <cstdint>
#include <vector>
class TFT_eSPI {
 public:
  std::vector<uint8_t> commands, data;
  int rotation = -1;
  bool inverted = false;
  void writecommand(uint8_t command) { commands.push_back(command); }
  void writedata(uint8_t value) { data.push_back(value); }
  void setRotation(int value) { rotation = value; }
  void invertDisplay(bool value) { inverted = value; }
};
