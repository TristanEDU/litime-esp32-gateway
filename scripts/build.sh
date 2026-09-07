#!/usr/bin/env bash
set -euo pipefail

profile="${1:-release}"
case "$profile" in
  release) debug_flag=0; websocket_debug='' ;;
  development) debug_flag=1; websocket_debug='-DDEBUG_ESP_PORT=Serial' ;;
  *) echo "Usage: $0 [release|development]" >&2; exit 64 ;;
esac

output_dir="build/$profile"
mkdir -p "$output_dir"
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=custom,UploadSpeed=115200' \
  --libraries firmware/libraries \
  --build-property upload.maximum_size=2031616 \
  --build-property "compiler.cpp.extra_flags=-DGATEWAY_DEBUG=$debug_flag $websocket_debug" \
  --output-dir "$output_dir" \
  firmware
