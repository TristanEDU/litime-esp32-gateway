#!/bin/zsh

arduino-cli monitor \
  -p /dev/cu.usbserial-0001 \
  --config baudrate=115200
