# The Not-A-Better-Anduril... Anduril Fork

###### Definitely not a better version of Anduril, i promise.



# About
Anduril is great and all, but lacks some basic features and seemingly obvious adjustments.
So yea, thats what this fork is for. Enjoy!

But before you proceed, I'd like to mention that this fork is targeted towards HankLights and HankLights only. I tried my best to maintain backwards compatibility with other Anduril based flashlights, but can't guarantee anything given that my collection only consists of HankLights for now.

Oh and... I'm not responsible for you're newly flashed light exploding with a shock wave more powerful than that of Fatman. I'm a certified dumb-ass who just so happens to be a stubborn bastard who doesn't take no for an answer - hence this monstrosity of an Anduril fork.

Cheers!


# Flashing existing hex files

Before we can start flashing fancy Anduril forks onto our lights, we first need to identify a few things.

1. Head on over to https://ivanthinking.net/thoughts/anduril-1-or-2-how-to-check-what-version-of-anduril/ and follow the instructions in order to check if your light's running Anduril 1 or 2.

2. Identify which hex file is needed for the given light using https://ivanthinking.net/thoughts/emisar-d4v2-anduril2-hex-file-selection/

3. Download the hex file from the [Releases section](https://github.com/Kevy42/Not-A-Better-Anduril/releases)

Now that you're all set, we can finally start flashing!  
This can be done the the really easy, half easy, or very hard way (using Windows). Personally recommend Linux for obvious reasons, and firing up a VM nowadays only takes like an hour.

### Linux

1. Install required packages  
        `sudo apt install avrdude avrdude-doc -y`

2. Connect the flashing kit to your computer and light.

3. Test the connection to the chip.  
        `sudo avrdude -c usbasp -p t1634 -n`
    
3. Flash the firmware.  
        `sudo avrdude -c usbasp -p t1634 -u -Uflash:w:[HEX FILE NAME]`  

Note (for steps 3 and 4): replace `t1634` with the controller type you're light uses.

### Mac
Follow https://oweban.org/files/Anduril%20-%20macOS%20guide.pdf


### Windows
Follow: https://oweban.org/files/Anduril%20-%20Windows%2010%20guide.pdf




# Compiling your own hex's

### Linux

1. Install required packages.  
        `sudo apt install flex byacc bison gcc libusb-dev libc6-dev`  
        `sudo apt install gcc-avr avr-libc binutils-avr`  
        `sudo apt install avrdude avrdude-doc`

2. Compile hex files.  
Navigate to `"/Anduril 2/ToyKeeper/spaghetti-monster/anduril/"` and run `./build-all.sh`  
Note: Not all builds will compile (there will be 9 which won't).

3. Connect the flashing kit to you're computer and light.

4. Test the connection to the chip.  
        `sudo avrdude -c usbasp -p t1634 -n`
    
5. Flash the firmware.  
        `sudo avrdude -c usbasp -p t1634 -u -Uflash:w:[HEX FILE NAME]`  
    

Note (for steps 4 and 5): replace `t1634` with the controller type you're light uses.

### Mac & Windows
No idea, never managed to get it working. I'm open to advice.


# Additional resources
  - ToyKeeper's website: https://toykeeper.net/

  - ToyKeeper's repo: https://launchpad.net/~toykeeper

  - Anduril 2 repo: https://bazaar.launchpad.net/~toykeeper/flashlight-firmware/anduril2/files

  - Anduril 2 model identification table: https://toykeeper.net/torches/fsm/anduril2/MODELS

  - Anduril 2 stock/default hex files: https://toykeeper.net/torches/fsm/anduril2/




# Features/Changes

### Decoupled button and aux leds.
From off, use 6C to cycle through the button led brightness levels (off, low & high) independent of the aux led's!

https://user-images.githubusercontent.com/31995172/193652963-db614239-e889-4efc-ba50-3b120cf36dba.mp4



### Set default (post factory reset) off and lockout button leds to high and low respectively.  
### Set default (post factory reset) off and lockout aux leds to high and low disco respectively.

https://user-images.githubusercontent.com/31995172/193653058-fbf26cb5-453b-4db2-9d9d-63066e91f1fd.mp4



### Set default (post factory reset) mode to advanced.
Thank god, that was so annoying.

https://user-images.githubusercontent.com/31995172/193653410-a9cb9897-eca3-4203-8e57-bf788e5660fc.mp4



### Set default autolock time to 4h.



### Disabled the blink when going in and out of off/lockout mode.
Annoying when you're in the pitch black.

https://user-images.githubusercontent.com/31995172/193654385-885cdeb7-4e6e-4ea5-b6e7-2d4c991a7079.mp4



### Changed momentary activation to 5H from off mode.
No more getting stuck in momentary!

https://user-images.githubusercontent.com/31995172/193653856-d5ea5408-968a-4677-974d-67c4a7e5a081.mp4



### Set rainbow update interval to 35'ish seconds.

https://user-images.githubusercontent.com/31995172/193653945-3bec3424-7218-4d69-8cc4-3ea4d3dc2cc4.mp4



### Set default (post factory reset) thermal regulation limit to 50 degrees C.



### Enabled blink at ramp floor.

https://user-images.githubusercontent.com/31995172/193653745-9e18adad-2944-47c0-b4ff-4341d8464320.mp4


### Disabled bikeflasher strobe mode.
Does anyone use this? if so.. sorry not sorry.

https://user-images.githubusercontent.com/31995172/193654521-e873b99c-7e53-42b7-9ecc-957ad7b70277.mp4



### Set configuration buzz inactivity duration to 5 sec.

https://user-images.githubusercontent.com/31995172/193654571-243af892-2b22-414e-88c1-e81c707fd120.mp4



### Disabled factory resetting from simple UI
Insurance :)

https://user-images.githubusercontent.com/31995172/193654878-e496c997-3c6b-47ce-8c6b-6c0a62ba0c3e.mp4



### Changed information blinkout (e.g voltage, firmware version.. etc) to use button and or aux leds.

### Slowed down information blinkout speed and increased ontime for 0's.
Its finally readable!

https://user-images.githubusercontent.com/31995172/193655022-5be7fa3b-92c7-43a5-a228-a30834742e71.mp4



### Added morse code easter egg (10C from SOS blinky & utility mode)
Disabled for now in order to make everything compile.


### Added 3C for going back to the previous strobe/blinky & utility modes

https://user-images.githubusercontent.com/31995172/193655186-7cf9270e-3f62-4763-8c60-05c1e6160828.mp4



### Enabled sunset timer in simple UI

https://user-images.githubusercontent.com/31995172/193654727-41498538-d636-4efa-9fc9-6be989de6906.mp4



### Increased sunset blink to 10 minutes.



### Lowered ramping mode button led low/high threshold.
Its much closer to moonlight levels now.

https://user-images.githubusercontent.com/31995172/193655420-5a5e102a-5cd8-4683-8aad-0611a856cfd6.mp4



## Possible future features/changes
    - calm candle mode
    - Add termal throttling indication
    - ramp up to brightness when turned on
    - turn aux on in on mode
    - Brighter party strobe
    - check code (dont forget to NOT add indicator led support)
    - reenable blink for lockout/off?
