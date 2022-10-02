# Not-A-Better-Anduril... Anduril Fork

###### Definetly not a better version of anduril, i promise.



# About
Anduril is great and all.. but lacks some basic features and seemingly obvious adjustments.
So yea, thats what this fork is all about. enjoy!



# Flashing existing hex files

We can do this the really easy, half easy, or very hard way (cough cough using Shitdows cough cough)


### Linux

1. Install required packages  
        sudo apt-get install avrdude avrdude-doc -y

2. Connect the flashing kit to you're computer and light.

3. Test the connection to the chip.  
        sudo avrdude -c usbasp -p t1634 -n
    
3. Flash the firmware.  
        sudo avrdude -c usbasp -p t1634 -u -Uflash:w:[HEX FILE NAME]  
Note: Replace [HEX FILE NAME] with the name of the hex file you'd like to flash.

Note (for step 3 and 4): replace "t1634" with the controller type you're light uses.

### Mac
https://oweban.org/files/Anduril%20-%20macOS%20guide.pdf


### Windows
https://oweban.org/files/Anduril%20-%20Windows%2010%20guide.pdf




# Compiling you're own hex's

### Linux

1. Install required packages.  
        sudo apt-get install flex byacc bison gcc libusb-dev libc6-dev  
        sudo apt-get install gcc-avr avr-libc binutils-avr  
        sudo apt-get install avrdude avrdude-doc

2. compile hex files.  
Navigate to "/Anduril 2/ToyKeeper/spaghetti-monster/anduril/" and run
        ./build-all.sh.
Note: Not all builds will compile, and there will be 9 which wont. Any advice on how to get these to compile as well would be much appreciated.

3. Connect the flashing kit to you're computer and light.

4. Test the connection to the chip.  
        sudo avrdude -c usbasp -p t1634 -n
    
5. Flash the firmware.  
        sudo avrdude -c usbasp -p t1634 -u -Uflash:w:[HEX FILE NAME]  
Note: Replace [HEX FILE NAME] with the name of the hex file you'd like to flash.
    

Note (for step 4 and 5): replace "t1634" with the controller type you're light uses.

### Mac & Windows
No idea, never managed to get it working. I'm open to advice.





# Features/Changes

### Decoupled button and aux leds.
From off, use 6C to cycle through the button led brightness levels (off, low & high) independant of the aux led's!

### Set default (post factory reset) off and lockout switch leds to high and low respectively.


### Set default (post factory reset) off and lockout aux leds to high and low disco respectively.


### Set default (post factory reset) mode to advanced.
Thank god, that was annoying.

### Set default autolock time to 12h.


### Disabled the blink when going in and out of off/lockout mode.
Annoying when you're in the pitch black.


### Changed momentary activation to 5H from off mode.
No more getting stuck in momentary!


### Set rainbow update interval to 35'ish seconds.


### Set default (post factory reset) thermal regulation limit to 50 degrees C.


### Enabled blink at ramp floor.


### Disabled bikeflasher strobe mode.
Does anyone use this? if so.. sorry not sorry.


### Set configuration buzz inactivity duration to 5 sec.


### Disabled factory resetting from simple UI
Insurance :)


### Changed information blink (e.g voltage, firmware version.. etc) readout to use button and or aux leds.


### Added morse code easter egg (10C from SOS blinky & utility mode)
Disabled for now in order to make everything compile.


### Added 3C for going back to the previous strobe/blinky & utility modes


### Slowed down information blinkout speed and increased ontime for 0's.
Its finally readable!


### Enabled sunset timer in simple UI


### Lowered ramping mode button led low/high threshold.
Its much closer to moonlight levels now.


### Increased sunset blink to 10 minutes.

## Possible future features/changes
    - calm candle mode
    - Add termal throttling indication
    - ramp up to brightness when turned on
    - turn aux on in on mode
    - Brighter party strobe
    - check code (dont forget to NOT add indicator led support)
    - reenable blink for lockout/off?