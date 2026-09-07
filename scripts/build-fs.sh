#!/usr/bin/env bash
set -euo pipefail

output_file="${1:-build/littlefs.bin}"
mkdir -p "$(dirname "$output_file")"
mklittlefs_path=$(arduino-cli compile --show-properties --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=custom,UploadSpeed=115200' firmware 2>/dev/null | sed -n 's/^runtime.tools.mklittlefs.path=//p' | head -1)
mklittlefs_tool="$mklittlefs_path/mklittlefs"
if [[ -z "$mklittlefs_path" || ! -x "$mklittlefs_tool" ]]; then
  echo 'mklittlefs was not found in the installed ESP32 Arduino core.' >&2
  exit 1
fi
# Must match firmware/partitions.csv: 0x10000 bytes at offset 0x3F0000.
"$mklittlefs_tool" -c firmware/data -p 256 -b 4096 -s 0x10000 "$output_file"
echo "Built $output_file"
