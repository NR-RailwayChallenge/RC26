# RC24 Handheld Indications Pico Firmware

## About
This repository contains the firmware running on the Pi Pico which handles the LEDs and the LCD.

> [!IMPORTANT]  
> Follow the pre-setup instructions *before* cloning this repo!

## Pre-setup instructions

Due to MAX_PATH limitations and PlatformIO wanting to do a recursive clone, we need to setup a local copy of 
the `arduino-pico` repository on our machines and manually do a shallow `git clone`. 

    1. Go to the root of your C drive, and create a directory named `dev`.
    2. Open powershell, and paste each of the below lines one-by-one.
    3. Once the commands below have been run, you may close the powershell window and proceed to clone this repo as usual.

```
cd c:\dev
git clone https://github.com/earlephilhower/arduino-pico 
cd .\arduino-pico\
git submodule update --init
cd pico-sdk
git submodule update --init
```
