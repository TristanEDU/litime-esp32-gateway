#!/bin/zsh

arduino-cli compile \
  --fqbn 'esp32:esp32:esp32:FlashMode=dio,FlashSize=4M,PartitionScheme=no_fs,UploadSpeed=115200' \
  firmware
