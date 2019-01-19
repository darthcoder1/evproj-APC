#!/bin/bash
echo "Building firmware for STM32"

# build the project
~/.cargo/bin/cargo build --target=thumbv7em-none-eabihf

echo "done" 