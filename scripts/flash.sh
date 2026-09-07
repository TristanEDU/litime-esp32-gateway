#!/usr/bin/env bash
set -euo pipefail

port="${1:-/dev/cu.usbserial-0001}"
profile="${2:-release}"
./scripts/build.sh "$profile"
./scripts/build-fs.sh build/littlefs.bin

arduino-cli upload \
  --input-dir "build/$profile" \
  -p "$port" \
  --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=custom,UploadSpeed=115200'

esptool_path=$(arduino-cli compile --show-properties --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=custom,UploadSpeed=115200' firmware 2>/dev/null | sed -n 's/^runtime.tools.esptool_py.path=//p' | head -1)
esptool="$esptool_path/esptool"
if [[ -z "$esptool_path" || ! -x "$esptool" ]]; then
  echo 'esptool was not found in the installed ESP32 Arduino core.' >&2
  exit 1
fi
"$esptool" --chip esp32 --port "$port" --baud 460800 write-flash 0x3F0000 build/littlefs.bin
