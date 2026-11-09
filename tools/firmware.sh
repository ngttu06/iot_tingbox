#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLI=(arduino-cli)
if [[ -f "$ROOT/.tools/arduino-cli.yaml" ]]; then CLI+=(--config-file "$ROOT/.tools/arduino-cli.yaml"); fi
FQBN='esp32:esp32:esp32:PartitionScheme=huge_app'
EXTRA=()
# Local validation environment: same official core/tool archives, manually extracted
# to avoid downloading toolchains for unrelated ESP32 chip families.
if [[ -d "$ROOT/.tools/manual-toolchain/esp32-libs" && -d "$ROOT/.tools/arduino-user/hardware/espressif/esp32" ]]; then
  FQBN='espressif:esp32:esp32:PartitionScheme=huge_app'
  EXTRA+=(--build-property "runtime.tools.esp-x32.path=$ROOT/.tools/manual-toolchain/xtensa-esp-elf")
  EXTRA+=(--build-property "runtime.tools.esp32-libs.path=$ROOT/.tools/manual-toolchain/esp32-libs")
  EXTRA+=(--build-property "runtime.tools.esptool_py.path=$ROOT/.tools/manual-toolchain/esptool-macos-amd64")
fi
if [[ "${1:-build}" == 'setup' ]]; then
  "${CLI[@]}" core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  "${CLI[@]}" core install esp32:esp32@3.3.7 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
  "${CLI[@]}" lib install 'ArduinoJson@7.4.2' 'TFT_eSPI@2.5.43' 'ESP8266Audio@2.4.1'
  exit 0
fi
python3 "$ROOT/tools/audio/embed.py"
"${CLI[@]}" compile --fqbn "$FQBN" --build-path "$ROOT/firmware/build" \
  "${EXTRA[@]}" \
  --build-property "build.extra_flags=-DESP32 -DCORE_DEBUG_LEVEL=0 -DARDUINO_RUNNING_CORE=1 -DARDUINO_EVENT_RUNNING_CORE=0 -DUSER_SETUP_LOADED -include $ROOT/firmware/tft/User_Setup.h" \
  "$ROOT/firmware/SmartPayESP32"
if [[ "${1:-build}" == 'upload' ]]; then
  : "${2:?Provide serial port as second argument}"
  "${CLI[@]}" upload --fqbn "$FQBN" --port "$2" --input-dir "$ROOT/firmware/build" "$ROOT/firmware/SmartPayESP32"
fi