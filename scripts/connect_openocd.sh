#!/bin/bash

# OPENOCD setup

OPENOCD_DIR=~/Development/Tools/openocd_win
$OPENOCD_DIR/bin/openocd.exe -s $OPENOCD_DIR/share/scripts -f interface/stlink-v2.cfg -f target/stm32f1x.cfg