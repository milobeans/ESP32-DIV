#ifndef SUBCONFIG_H
#define SUBCONFIG_H

#include <EEPROM.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "BoardButtons.h"
#include <RCSwitch.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <cstddef>
#include <stdio.h>
#include <string>
#include "arduinoFFT.h"
#include "shared.h"
#include "utils.h"

extern TFT_eSPI tft;
extern BoardButtonExpander pcf;

namespace replayat {
  void ReplayAttackSetup();
  void ReplayAttackLoop();
}

namespace SavedProfile {
  void saveSetup();
  void saveLoop();
}

namespace subjammer {
  void subjammerSetup();
  void subjammerLoop();
}

namespace SubBrute {
  void subBruteSetup();
  void subBruteLoop();
}

namespace jammingdetector {
  void Setup();
  void Loop();
}

#endif
