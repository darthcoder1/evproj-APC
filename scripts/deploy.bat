
echo @off

echo "Deploying to STM32"

set FLASH_TOOL=D:/Development/Tools/stlink-1.3.0-win64/bin/st-flash.exe
set OUTPUT_BIN=target/thumbv7em-none-eabihf/debug/APC

arm-none-eabi-objcopy -O ihex %OUTPUT_BIN% %OUTPUT_BIN%.hex
%FLASH_TOOL% --format ihex write %OUTPUT_BIN%.hex