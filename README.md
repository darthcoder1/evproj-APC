# Auxilaries Power Controller (APU)

## Overview

This is an ARM Cortex M3 (STM32F103C8T6) controlling the power output to the auxilaries of a motorcycle. The
MCU reads input from the driver controls, processes these and switches the according auxilaries
on or off (Headlight, Brakelight, Turn signals, etc). The output is controlled via BTS432E2 
powerfets with on-die protection for all the things (ESD, shorts, etc) as well as a diagnosis pin.

The logic is implemented in rust

* Read inputs from the driver controls
* Determine the on/off state of the auxilaries
* Switch the power output to the auxilaries
* Read diagnosis from all output channels
* handle possible errors
* (send telemtric information to ???)

## Setup Requirements

* Install Rust
* Install nightly toolchain ("rustup toolchain install nightly")
* Set nightly toolchain as default ("rustup default nightly")
* Install arm target 'thumbv7em-none-eabihf' ("rustup add target thumbv7em-none-eabihf")
* Install arm compiler toolchain ("apt-get install gcc-arm-none-eabi")

Rust must be installed as well as the rust target .

## Deployment

Check the scripts

## Inputs

The input is handled by a BD3376EFV-CE2 (Multiple Input Switch Monitor). All external driver controls 
are connected via this chip. It is responsible for securing against hazards (ESD, shorts,etc) and dispatches
the state of the inputs to the ARM Cortex M3 via SPI.

                        Channel
* Killswitch            [Ch 00]         
* Ignition              [Ch 01]
* Sidestand             [Ch 02]
* Headlight             [Ch 03]
* Fullbeam              [Ch 04]
* Turn Signal Left      [Ch 05]
* Turn Signal Right     [Ch 06]
* Hazard                [Ch 07]
* Brake signal Front    [Ch 08]
* Brake signal Rear     [Ch 09]
* Horn                  [Ch 10]


The MISM is connected via the following pinout to the STM32. To read a specific input channel, the Selector bits 
must be setup to indicate which channel should be read from. The data pin then forwards the state of the selected
channel.

Input channels: 0-7
    PA11 - Selector 0
    PA12 - Selector 1
    PA15 - Selector 2
    PB04 - Data

Input channels: 8-15
    PA10 - Selector 0
    PA09 - Selector 1
    PA08 - Selector 2
    PB03 - Data



## Outputs

The ouput has 12 channel. The software takes care of the channel usage, so they are fully programmable. 
The outputs are switched by a BTS432E2 powerFET per channel.

* Headlight lower beam      [Ch 00]
* Headlight full beam       [Ch 01]
* Headlight parking         [Ch 02]
* Turn signal left front    [Ch 03]
* Turn signal left rear     [Ch 04]
* Turn signal right front   [Ch 05]
* Turn signal right rear    [Ch 06]
* Rearlight                 [Ch 07]
* Braking light             [Ch 08]
* Horn                      [Ch 09]
* Unused                    [Ch 10]
* Unused                    [Ch 11]


## States

When turned on, the APC is in the "locked" state. The driver needs to press the front and the back brake down 
for 1 sec to active the APC. The "active" state is then reading the drivers controls and switches the power
for the auxialiers on or off. 

"locked" ==> [ Break Signal Front + Back for 1 sec ] ==> "active"
? "active" ==> [ ... ] ==> "debug"



