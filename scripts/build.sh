#!/bin/zsh
set -euo pipefail

profile="${1:-release}"
case "$profile" in
  release) debug_flag=0 ;;
  development) debug_flag=1 ;;
  *) print -u2 "Usage: $0 [release|development]"; exit 64 ;;
esac

output_dir="build/$profile"
mkdir -p "$output_dir"
arduino-cli compile \
  --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=custom,UploadSpeed=115200' \
  --build-property upload.maximum_size=1966080 \
  --build-property "compiler.cpp.extra_flags=-DGATEWAY_DEBUG=$debug_flag" \
  --output-dir "$output_dir" \
  firmware
