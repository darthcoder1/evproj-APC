#!/bin/bash

./build.sh

echo "Deploying to STM32"

STLINK_BIN_DIR=~/Development/Tools/stlink-1.3.0-win64/bin
FLASH_TOOL=$STLINK_BIN_DIR/st-flash.exe
OUTPUT_BIN=target/thumbv7em-none-eabihf/debug/APC

OUTPUT_HEX=$OUTPUT_BIN.hex

arm-none-eabi-objcopy -O ihex $OUTPUT_BIN $OUTPUT_BIN.hex
$FLASH_TOOL --format ihex write $OUTPUT_BIN.hex