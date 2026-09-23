#!/usr/bin/env bash
# Usage: tools/hiwonder/build-div.sh [--install]; builds a complete image, never flashes.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
CLI=${ARDUINO_CLI:-arduino-cli}
if [[ ${1:-} == --install ]]; then
  "$CLI" core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  "$CLI" core install esp32:esp32@2.0.10 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  "$CLI" lib install 'PCF8574 library@2.3.7' 'XPT2046_Touchscreen@1.4.0' \
    'NimBLE-Arduino@1.4.3' 'RF24@1.4.11' 'rc-switch@2.6.4' \
    'arduinoFFT@1.6.2' 'ArduinoJson@6.21.5' 'IRremoteESP8266@2.8.6' \
    'Adafruit PN532@1.3.4' 'Adafruit BusIO@1.17.2'
fi
python3 - <<'PY'
from pathlib import Path
from zipfile import ZipFile
root = Path.cwd()
dest = root / '.libraries'
dest.mkdir(exist_ok=True)
for archive in ('TFT_eSPI-master.zip', 'SmartRC-CC1101-Driver-Lib-master.zip'):
    with ZipFile(root / 'Libraries' / archive) as z:
        for name in z.namelist():
            if not (dest / name).resolve().is_relative_to(dest.resolve()):
                raise SystemExit('Unsafe archive path')
        z.extractall(dest)
PY
mkdir -p .build artifacts
cp tools/hiwonder/partitions.csv ESP32-DIV/partitions.csv
FLAGS="-DBOARD_HIWONDER_ESP32_S3=1 -include $ROOT/tools/hiwonder/hiwonder_tft.h"
"$CLI" compile --fqbn 'esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,FlashMode=qio,PSRAM=opi,PartitionScheme=huge_app' \
  --libraries "$ROOT/.libraries" --build-path "$ROOT/.build" \
  --build-property "compiler.cpp.extra_flags=$FLAGS" \
  --build-property "compiler.c.extra_flags=$FLAGS" \
  --export-binaries ESP32-DIV
python3 tools/hiwonder/merge-div.py
