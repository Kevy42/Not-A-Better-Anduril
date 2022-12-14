//-------------------------------------------------------------------------------------
// eswBrOutCfg.c - based on STAR_momentary.c v1.6 from JonnyC as of Feb 14th, 2015
// =============
//  Modified by Tom E
//
//  ***************  ATtiny25/45/85 Version  ***************
//
//  This version is the base "eswitch" firmware merged in with STAR_NOINIT version to
// combine full e-switch support with a power switch capability to do mode changing, in
// addition to other enhancements.
//
// Capabilities:
//  - full e-switch support in standby low power mode
//  - full mode switching off a power switch (tail) w/memory using "NONINIT" or brownout method
//  - lock-out feature - uses a special sequence to lock out mode changes
//  - battery voltage level display in n.n format (ex: 4.1v blinks 4 times followed by 1 blink)
//  - multiple strobe and beacon modes
//  - mode configuration settings:
//			- 8 mode sets to choose from
//			- moonlight mode on/off
//			- lo->hi or hi->lo ordering
//			- mode memory on/off
// 		- turbo-timeout setting	
//
// Change History
// --------------
// 11/16/2015:
//   - OFF mode wasn't setting PWM mode properly - fixed this
//   - for all strobe modes, set PWM mode to PHASE only once, not repeatedly
// 10/30/2015 - restore 3 modes for med to 35%
// 10/29/2015 - lengthen CONFIG_ENTER_DUR from 128 to 160
//            - lengthen LOCK_OUT_TICKS from 12 to 16
//            - temp change 3 modes for med to 50% for LJ
// 10/27/2015 - bug fixes w/strobes
// 10/26/2015 - changes:
//   - added tk-delay.h into project (forgot it)
//   - add multiple strobe/beacon modes
// 
// 10/25/2015 - added "n.n" style voltage blinking status (Battery Check)
// 10/18/2015 - finalized config mode - checkpoint, tested in the SupFire and AS31
// 10/11/2015 - cleanups in header
// 10/07/2015 - full mode config options (4 options total), few bug fixes, lock-out added and working
// 09/17/2015 - renamed to eSwitchBrownOut
// 08/17/2015 - change over to 25/45/85 support
//				  - bug fix: hold if in 1st mode won't engage strobe
//
// 07/26/2015 - attempt to merge in NOINIT functionality to have rear tail clicky functional
// 02/14/2015 - copied from JohnnyC, merged in mods from my earlier e-switch version
//-------------------------------------------------------------------------------------

/*
 * ATtiny 25/45/85 Diagram
 *              ---
 *   Reset  1 -|   |- 8  VCC
 *  switch  2 -|   |- 7  Voltage ADC
 * not used 3 -|   |- 6  FET PWM
 *     GND  4 -|   |- 5  7135 PWM
 *              ---
 *
 * FUSES
 *  See this for fuse settings:
 *    http://www.engbedded.com/cgi-bin/fcx.cgi?P_PREV=ATtiny13A&P=ATtiny13A&M_LOW_0x0F=0x09&M_LOW_0x80=0x00&M_HIGH_0x06=0x04&M_HIGH_0x10=0x00&B_SPIEN=P&B_SELFPRGEN=P&B_SUT0=P&B_BODLEVEL0=P&B_CKSEL0=P&V_LOW=7A&V_HIGH=ED
 * 
 *  Following is the command options for the fuses used:
 *    -Ulfuse:w:0xe2:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m
 * 
 *		  Low: 0xE2 - 8 MHz CPU without a divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM
 *	    High: 0xDF - enable serial prog/dnld, no brown out (or 0xde for brown out)
 *    Extra: 0xFF - self programming not enabled
 *
 * STARS  (not used)
 *
 * VOLTAGE
 *		Resistor values for voltage divider (reference BLF-VLD README for more info)
 *		Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *           VCC
 *            |
 *           Vd (~.25 v drop from protection diode)
 *            |
 *          1912 (R1 19,100 ohms)
 *            |
 *            |---- PB2 from MCU
 *            |
 *          4701 (R2 4,700 ohms)
 *            |
 *           GND
 *
 *		ADC = ((V_bat - V_diode) * R2   * 255) / ((R1    + R2  ) * V_ref)
 *		125 = ((3.0   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *		121 = ((2.9   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *
 *		Well 125 and 121 were too close, so it shut off right after lowering to low mode, so I went with
 *		130 and 120
 *
 *		To find out what value to use, plug in the target voltage (V) to this equation
 *			value = (V * 4700 * 255) / (23800 * 1.1)
 *      
 */

#define FET_7135_LAYOUT
#define ATTINY 25
#include "tk-attiny.h"

#define byte uint8_t
#define word uint16_t

#define PHASE 0xA1          // phase-correct PWM both channels
#define FAST 0xA3           // fast PWM both channels

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define MOONLIGHT_LEVEL		3	// usually set to 2 or 3, depending on - multi-LED: 3, 380 7135: 3, 350 7135 and want low lo: 2

// ----- 2/14 TE: One-Click Turn OFF option --------------------------------------------
#define IDLE_TIME         75	// make the time-out 1.2 seconds (Comment out to disable)
// ----------------------------------------------------------------------------

// Switch handling:
#define LONG_PRESS_DUR    24	// How long is a long press, 24=0.375s (1/3s: too fast, 0.5s: too slow)
#define XLONG_PRESS_DUR   68	// 68=1.09s (any slower it can happen unintentionally too much)
#define CONFIG_ENTER_DUR 160	// 160=2.5s, 128=2s - hold button down for this time to enter config setting mode or lock-out

#define LOCK_OUT_TICKS    16	// fast click time for enable/disable of Lock-Out and batt check (16=0.256s, 12=0.192s)

#define VOLTAGE_MON		// Comment out to disable - ramp down and eventual shutoff when battery is low

// These values seem to work for wight DD+1 drivers using a 22k ohm R1 resistor
#define ADC_LOW          120	// When do we start ramping (~2.9v)
#define ADC_CRIT         112	// When do we shut the light off (~2.70v)
  
// These are typical for 3.1v (125 for 3.0v, 130 for 3.1v):
//#define ADC_LOW          125	// When do we start ramping (3.0v), 5 per 0.1v
//#define ADC_CRIT         115	// When do we shut the light off (2.80v)

#define ADC_DELAY        188	// Delay in ticks between low-bat ramp-downs (188 ~= 3s)

// output to use for blinks on battery check mode (primary PWM level, alt PWM level)
// Use 20,0 for a single-channel driver or 0,40 for a two-channel driver
#define BLINK_BRIGHTNESS 0,40

#define BATT_CHECK_MODE		80
#define SPECIAL_MODES		90		// base/lowest value for special modes
#define STROBE_MODE			SPECIAL_MODES+1
//#define RANDOM_STROBE		SPECIAL_MODES+2	// not used for now...
#define POLICE_STROBE		SPECIAL_MODES+2
#define BIKING_STROBE		SPECIAL_MODES+3
#define BEACON_2S_MODE		SPECIAL_MODES+4
#define BEACON_10S_MODE		SPECIAL_MODES+5

// Custom define your blinky mode set here:
#define SPECIAL_MODES_SET	STROBE_MODE, POLICE_STROBE, BIKING_STROBE, BEACON_2S_MODE, BEACON_10S_MODE

/*
 * =========================================================================
 */

#include <avr/pgmspace.h>
#include <avr/io.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>	
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>


#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_S         // Also use _delay_s(), not just _delay_ms()

#include "tk-delay.h"
#include "tk-random.h"
#include "tk-calibration.h"


// MCU I/O pin assignments (most are now in tk-attiny.h):
#define SWITCH_PIN  PB3		// Star 4,  MCU pin #2 - pin the switch is connected to

//#define DEBOUNCE_BOTH          // Comment out if you don't want to debounce the PRESS along with the RELEASE
                               // PRESS debounce is only needed in special cases where the switch can experience errant signals
#define DB_PRES_DUR 0b00000001 // time before we consider the switch pressed (after first realizing it was pressed)
#define DB_REL_DUR  0b00001111 // time before we consider the switch released
							   // each bit of 1 from the right equals 16ms, so 0x0f = 64ms

/*
 * The actual program
 * =========================================================================
 */

//------------------- MODE SETS --------------------------

// 1 mode (max)                         max
PROGMEM const byte modeFetSet1[] =  {   255};
PROGMEM const byte mode7135Set1[] = {     0};
PROGMEM const byte modePwmSet1[] =  { PHASE};

// 2 modes (10-max)                     ~10%   max
PROGMEM const byte modeFetSet2[] =  {     0,   255};
PROGMEM const byte mode7135Set2[] = {   255,     0};
PROGMEM const byte modePwmSet2[] =  { PHASE, PHASE};

// 3 modes (5-50-max - for LJ)           ~5%   ~50%   max
//PROGMEM const byte modeFetSet3[] =  {     0,   110,   255 };	// Must be low to high, and must start with 0
// 3 modes (5-35-max)                    ~5%   ~35%   max
PROGMEM const byte modeFetSet3[] =  {     0,    70,   255 };	// Must be low to high, and must start with 0
PROGMEM const byte mode7135Set3[] = {   120,   255,     0 };	// for secondary (7135) output. Comment out if no secondary output};
PROGMEM const byte modePwmSet3[] =  {  FAST,  FAST, PHASE };	// Define one per mode above. 0 tells the light to go to sleep

// 4 modes (2-10-40-max)                1-2%   ~10%   ~40%   max
PROGMEM const byte modeFetSet4[] =  {     0,     0,    80,   255 };	// Must be low to high, and must start with 0
PROGMEM const byte mode7135Set4[] = {    30,   255,   255,     0 };	// for secondary (7135) output. Comment out if no secondary output
PROGMEM const byte modePwmSet4[] =  {  FAST, PHASE, FAST, PHASE };	// Define one per mode above. 0 tells the light to go to sleep

// 5 modes (2-10-40-max)                1-2%    ~5%   ~10%   ~40%   max
PROGMEM const byte modeFetSet5[] =  {     0,     0,     0,    80,   255 };	// Must be low to high, and must start with 0
PROGMEM const byte mode7135Set5[] = {    30,   120,   255,   255,     0 };	// for secondary (7135) output. Comment out if no secondary output
PROGMEM const byte modePwmSet5[] =  {  FAST,  FAST, PHASE,  FAST, PHASE };	// Define one per mode above. 0 tells the light to go to sleep

// 6 modes - copy of BLF A6 7 mode set
PROGMEM const byte modeFetSet6[] =  {     0,     0,     7,    56,   137,   255};
PROGMEM const byte mode7135Set6[] = {    20,   110,   255,   255,   255,     0};
PROGMEM const byte modePwmSet6[] =  {  FAST,  FAST,  FAST,  FAST,  FAST, PHASE};

// #7:  3 modes (10-35-max)             ~10%   ~35%   max
PROGMEM const byte modeFetSet7[] =  {     0,    70,   255 };	// Must be low to high, and must start with 0
PROGMEM const byte mode7135Set7[] = {   255,   255,     0 };	// for secondary (7135) output. Comment out if no secondary output
PROGMEM const byte modePwmSet7[] =  { PHASE,  FAST, PHASE };	// Define one per mode above. 0 tells the light to go to sleep
	
// #8:  4 modes - copy of BLF A6 4 mode
PROGMEM const byte modeFetSet8[] =  {     0,     0,    90,   255};
PROGMEM const byte mode7135Set8[] = {    20,   230,   255,     0};
PROGMEM const byte modePwmSet8[] =  {  FAST,  FAST,  FAST, PHASE};


PROGMEM const byte modeSetCnts[] = {
        sizeof(modeFetSet1), sizeof(modeFetSet2), sizeof(modeFetSet3), sizeof(modeFetSet4),
		sizeof(modeFetSet5), sizeof(modeFetSet6), sizeof(modeFetSet7), sizeof(modeFetSet8)};

// Turbo timeout values, 16 msecs each: 30secs, 60 secs, 90 secs, 120 secs, 3 mins, 5 mins, 10 mins
PROGMEM const word turboTimeOutVals[] = {0, 1875, 3750, 5625, 7500, 11250, 18750, 37500};

const byte *(modeTableFet[]) =  { modeFetSet1, modeFetSet2, modeFetSet3, modeFetSet4,
								  modeFetSet5, modeFetSet6, modeFetSet7, modeFetSet8};
const byte *modeTable7135[] =   { mode7135Set1, mode7135Set2, mode7135Set3, mode7135Set4,
								  mode7135Set5, mode7135Set6, mode7135Set7, mode7135Set8};
const byte *modeTablePwm[] =    { modePwmSet1, modePwmSet2, modePwmSet3, modePwmSet4,
								  modePwmSet5, modePwmSet6, modePwmSet7, modePwmSet8};

byte modesCnt;			// total count of modes based on 'modes' arrays below

// Index 0 value must be zero for OFF state (up to 7 modes max, including moonlight)
byte modes[8];			// primary output (FET)
byte alt_modes[8];	// secondary output (7135)
byte mode_pwm[8];		// PHASE or FAST PWM's

const byte specialModes[] =    { SPECIAL_MODES_SET };
byte specialModesCnt = sizeof(specialModes);		// total count of modes in specialModes above
volatile byte specModeIdx;


//----------------------------------------------------------------
// Config Settings via UI:
//----------------------------------------------------------------
volatile byte modeSetIdx = 0;			// 0..7, mode set currently in effect, chosen by user
volatile byte moonLightEnable = 0;	// 1: enable moonlight mode, 0: disable moon mode
volatile byte highToLow = 0;			// 1: modes go from lowest to highest, 0: highest to lowest
volatile byte modeMemoryEnabled = 0;// 1: save/recall last mode set, 0: no memory
volatile byte turboTimeoutMode = 0;	// 0=disabled, 1=30s,2=60s,3=90s, 4=120s, 5=3min,6=5min,7=10min
//----------------------------------------------------------------

//----------------------------------------------------------------
// Global state options (TBD: may add to configuration setting UI):
//----------------------------------------------------------------
volatile byte OffTimeEnable = 0;		// 1: Do OFF time mode memory on power switching, 0: disabled
volatile byte byLockOutEnable = 1;	// button lock-out feature is enabled
//----------------------------------------------------------------


// State and count vars:
volatile byte byLockOutSet = 0;		// System is in LOCK OUT mode

volatile byte ConfigMode = 0;		// config mode is active: 1=init, 2=mode set,
											//   3=moonlight, 4=lo->hi, 5=mode memory, 6=done!)
volatile byte prevConfigMode = 0;
volatile byte configClicks = 0;
volatile byte configIdleTime = 0;

volatile byte modeIdx = 0;			// current mode selected
volatile byte prevModeIdx = 0;	// used to restore the initial mode when exiting strobe mode
volatile byte pressDuration = 0;

volatile byte quickClicks = 0;
volatile byte modeMemoryLastModeIdx = 0;

volatile byte mypwm=0;				// PWM output value, used in strobe mode

volatile word wIdleTicks = 0;
volatile word wTurboTickLimit = 0;

// Configuration settings storage in EEPROM
byte eepos = 0;	// (0..128) position of mode/settings stored in EEPROM
byte memory = 0;

// OFF Time Detection
volatile byte noinit_decay __attribute__ ((section (".noinit")));


PROGMEM const uint8_t voltage_blinks[] = {
    // 0 blinks for (shouldn't happen)
    ADC_25,(2<<5)+5,
    ADC_26,(2<<5)+6,
    ADC_27,(2<<5)+7,
    ADC_28,(2<<5)+8,
    ADC_29,(2<<5)+9,
    ADC_30,(3<<5)+0,
    ADC_31,(3<<5)+1,
    ADC_32,(3<<5)+2,
    ADC_33,(3<<5)+3,
    ADC_34,(3<<5)+4,
    ADC_35,(3<<5)+5,
    ADC_36,(3<<5)+6,
    ADC_37,(3<<5)+7,
    ADC_38,(3<<5)+8,
    ADC_39,(3<<5)+9,
    ADC_40,(4<<5)+0,
    ADC_41,(4<<5)+1,
    ADC_42,(4<<5)+2,
    ADC_43,(4<<5)+3,
    ADC_44,(4<<5)+4,
    255,   (1<<5)+1,  // Ceiling, don't remove
};

/**************************************************************************************
* strobe
* ======
**************************************************************************************/
void strobe(byte ontime, byte offtime)
{
	PWM_LVL = 255;
	_delay_ms(ontime);
	PWM_LVL = 0;
	_delay_ms(offtime);
}

/**************************************************************************************
* get_voltage
* ===========
**************************************************************************************/
uint8_t get_voltage()
{
	ADCSRA |= (1 << ADSC);				// Start conversion
	
	while (ADCSRA & (1 << ADSC))  ;	// Wait for completion
	
	return ADCH;	// Send back the result
}

/**************************************************************************************
* battcheck
* =========
**************************************************************************************/
inline uint8_t battcheck()
{
   // Return an composite int, number of "blinks", for approximate battery charge
   // Uses the table above for return values
   // Return value is 3 bits of whole volts and 5 bits of tenths-of-a-volt
   uint8_t i, voltage;

   voltage = get_voltage();

   // figure out how many times to blink
   for (i=0; voltage > pgm_read_byte(voltage_blinks + i); i += 2)  ;
   return pgm_read_byte(voltage_blinks + i + 1);
}

/**************************************************************************************
* DefineModeSet
* =============
**************************************************************************************/
inline void DefineModeSet()
{
	byte offset = 1;

	modesCnt = pgm_read_byte(modeSetCnts+modeSetIdx);

	// Set OFF mode states (index 0)
	modes[0] = alt_modes[0] = 0;
	mode_pwm[0] = PHASE;

	if (moonLightEnable)
	{
		offset = 2;
		modes[1] = 0;
		alt_modes[1] = MOONLIGHT_LEVEL;	// PWM moonlight level (3 for multi-LED's or 380 7135s, 2 for 350 7135's and 1 LED)
		mode_pwm[1] = PHASE;
	}

	// Populate the RAM based current mode set
	for (int i = 0; i < modesCnt; i++) 
	{
		modes[offset+i] = pgm_read_byte(modeTableFet[modeSetIdx]+i);
		alt_modes[offset+i] = pgm_read_byte(modeTable7135[modeSetIdx]+i);
		mode_pwm[offset+i] = pgm_read_byte(modeTablePwm[modeSetIdx]+i);
	}

	modesCnt += offset;		// adjust to total mode count
}

/**************************************************************************************
* set_output
* ==========
**************************************************************************************/
void set_output(uint8_t pwm1, uint8_t pwm2)
 {
	// Need PHASE to properly turn off the light
	if ((pwm1==0) && (pwm2==0))
		TCCR0A = PHASE;
	PWM_LVL = pwm1;
	ALT_PWM_LVL = pwm2;
}

/**************************************************************************************
* set_mode
* ========
**************************************************************************************/
void inline set_mode(uint8_t mode)
{
   TCCR0A = mode_pwm[mode];
	PWM_LVL = modes[mode];
	ALT_PWM_LVL = alt_modes[mode];
}

/**************************************************************************************
* blink - do a # of blinks with a speed in msecs
* =====
**************************************************************************************/
void blink(byte val, word speed)
{
	TCCR0A = PHASE;
	for (; val>0; val--)
	{
		set_output(BLINK_BRIGHTNESS);
		_delay_ms(speed);
		set_output(0,0);
		_delay_ms(speed<<2);	// 4X delay OFF
	}
}

/**************************************************************************************
* blink - do a # of blinks with a speed in msecs
* =====
**************************************************************************************/
void battBlink(byte val)
{
	TCCR0A = PHASE;
	for (; val>0; val--)
	{
		set_output(BLINK_BRIGHTNESS);
		_delay_ms(250);
		set_output(0,0);
		_delay_ms(375);
		if (modeIdx != BATT_CHECK_MODE)
			break;
	}
}

/**************************************************************************************
* clickBlink
* ==========
**************************************************************************************/
inline static void clickBlink()
{
	TCCR0A = PHASE;
	set_output(0,20);
	_delay_ms(100);
	set_output(0,0);
}


/**************************************************************************************
* is_pressed - debounce the switch release, not the switch press
* ==========
**************************************************************************************/
// Debounce switch press value
#ifdef DEBOUNCE_BOTH
int is_pressed()
{
	static byte pressed = 0;
	// Keep track of last switch values polled
	static byte buffer = 0x00;
	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
	
	if (pressed) {
		// Need to look for a release indicator by seeing if the last switch status has been 0 for n number of polls
		pressed = (buffer & DB_REL_DUR);
	} else {
		// Need to look for pressed indicator by seeing if the last switch status was 1 for n number of polls
		pressed = ((buffer & DB_PRES_DUR) == DB_PRES_DUR);
	}

	return pressed;
}
#else
static int is_pressed()
{
	// Keep track of last switch values polled
	static byte buffer = 0x00;
	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
	
	return (buffer & DB_REL_DUR);
}
#endif

/**************************************************************************************
* next_mode - switch's to next mode, higher output mode
* =========
**************************************************************************************/
void next_mode()
{
	if (modeIdx < 16)	// 11/16/14 TE: bug fix to exit strobe mode when doing a long press in strobe mode
		prevModeIdx	 = modeIdx;

	if (++modeIdx >= modesCnt)
	{
		// Wrap around
		modeIdx = 0;
	}	
}

/**************************************************************************************
* prev_mode - switch's to previous mode, lower output mode
* =========
**************************************************************************************/
void prev_mode()
{
	if (modeIdx < 16)	// 11/16/14 TE: bug fix to exit strobe mode when doing a long press in strobe mode
		prevModeIdx	 = modeIdx;

	if (modeIdx == 0)
		modeIdx = modesCnt - 1;	// Wrap around
	else
		--modeIdx;
}

/**************************************************************************************
* PCINT_on - Enable pin change interrupts
* ========
**************************************************************************************/
inline void PCINT_on() {
	// Enable pin change interrupts
	GIMSK |= (1 << PCIE);
}

/**************************************************************************************
* PCINT_off - Disable pin change interrupts
* =========
**************************************************************************************/
inline void PCINT_off() {
	// Disable pin change interrupts
	GIMSK &= ~(1 << PCIE);
}

// Need an interrupt for when pin change is enabled to ONLY wake us from sleep.
// All logic of what to do when we wake up will be handled in the main loop.
EMPTY_INTERRUPT(PCINT0_vect);

/**************************************************************************************
* WDT_on - Setup watchdog timer to only interrupt, not reset, every 16ms
* ======
**************************************************************************************/
inline void WDT_on() {
	// Setup watchdog timer to only interrupt, not reset, every 16ms.
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = (1<<WDIE);				// Enable interrupt every 16ms (was 1<<WDTIE)
	sei();							// Enable interrupts
}

/**************************************************************************************
* WDT_off - turn off the WatchDog timer
* =======
**************************************************************************************/
inline void WDT_off()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = 0x00;					// Disable WDT
	sei();							// Enable interrupts
}

/**************************************************************************************
* ADC_on - Turn the AtoD Converter ON
* ======
**************************************************************************************/
inline void ADC_on() {
// For 13A:	ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
	ADMUX  = (1 << REFS1) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
   DIDR0 |= (1 << ADC_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, pre-scale
}

/**************************************************************************************
* ADC_off - Turn the AtoD Converter OFF
* =======
**************************************************************************************/
inline void ADC_off() {
	ADCSRA &= ~(1<<7); //ADC off
}

/**************************************************************************************
* sleep_until_switch_press - only called with the light OFF
* ========================
**************************************************************************************/
void sleep_until_switch_press()
{
	// This routine takes up a lot of program memory :(
	// Turn the WDT off so it doesn't wake us from sleep
	// Will also ensure interrupts are on or we will never wake up
	WDT_off();
	// Need to reset press duration since a button release wasn't recorded
	pressDuration = 0;
	// Enable a pin change interrupt to wake us up
	// However, we have to make sure the switch is released otherwise we will wake when the user releases the switch
	while (is_pressed()) {
		_delay_ms(16);
	}
	PCINT_on();
	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	// Now go to sleep
	sleep_mode();
	// Hey, someone must have pressed the switch!!
	// Disable pin change interrupt because it's only used to wake us up
	PCINT_off();
	// Turn the WDT back on to check for switch presses
	WDT_on();
	// Go back to main program
}

/**************************************************************************************
* LoadConfig - gets the configuration settings from EEPROM
* ==========
**************************************************************************************/
inline void LoadConfig()
{
   byte config1, config2;

   // find the config data
   for (eepos=0; eepos < 128; eepos+=2)
	{
	   config1 = eeprom_read_byte((const byte *)eepos);
		config2 = eeprom_read_byte((const byte *)eepos+1);
	   if (config1 != 0xff)
   		break;
   }

   // unpack the config data
   if (eepos < 128)
	{
	   modeIdx = config1 & 0x07;
		modeSetIdx = (config1 >> 3) & 0x07;
		moonLightEnable = (config1 >> 6) & 1;
	   highToLow = (config1 >> 7) & 1;
	   modeMemoryEnabled = config2 & 1;
		turboTimeoutMode = (config2 >> 1) & 0x07;
	}
	else
		eepos = 0;
}

/**************************************************************************************
* SaveConfig - save the current mode with config settings
* ==========
*  Central method for writing (with wear leveling)
**************************************************************************************/
void SaveConfig()
{  
	// Pack all settings into one byte
	byte config1 = modeIdx | (modeSetIdx << 3) | (moonLightEnable << 6) | (highToLow << 7);
	byte config2 = modeMemoryEnabled | (turboTimeoutMode << 1);
	
	byte oldpos = eepos;
	
	eepos = (eepos+2) & 127;  // wear leveling, use next cell

	// Write the current settings (2 bytes)
	EEARL=eepos;   EEDR=config1; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
	while(EECR & 2)  ; // wait for completion
	EEARL=eepos+1; EEDR=config2; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
	while(EECR & 2)  ; // wait for completion

	// Erase the last settings (2 bytes)
	EEARL=oldpos;   EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
	while(EECR & 2)  ; // wait for completion
	EEARL=oldpos+1; EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}


/**************************************************************************************
* WDT_vect - The watchdog timer - this is invoked every 16ms
* ========
**************************************************************************************/
ISR(WDT_vect)
{
	static word wTurboTicks = 0;

  #ifdef VOLTAGE_MON
	static byte  adc_ticks = ADC_DELAY;
	static byte  lowbatt_cnt = 0;
  #endif

	//-------------------------------------------------------------------------------
   // Button is pressed
   //-------------------------------------------------------------------------------
	if (is_pressed())
	{
		if (pressDuration < 255)
			pressDuration++;
		
		if (ConfigMode > 0)
		{
			configIdleTime = 0;
			if (pressDuration == 35)		// make the hold time: 35*16 = 560 msecs
				++ConfigMode;
			return;
		}

		if (byLockOutSet == 0)
		{
			if (pressDuration == LONG_PRESS_DUR)
			{
				if (modeIdx < 16)
				{
					// Long press
					if (highToLow)
						next_mode();
					else
						prev_mode();
				}
				else if (modeIdx > SPECIAL_MODES)
				{
					if (specModeIdx > 0)
					{
						--specModeIdx;
						modeIdx = specialModes[specModeIdx];
					}
					else
						modeIdx = prevModeIdx;	// bail out of special modes
				}
			}

			if (pressDuration == XLONG_PRESS_DUR)
			{
				if ((quickClicks == 1) && (wIdleTicks < LOCK_OUT_TICKS))
				{
					modeIdx = BATT_CHECK_MODE;
				}
				else
				{
					if (modeIdx > SPECIAL_MODES)
					{
						modeIdx = prevModeIdx;	// restore last mormal mode
					}
					else
					{
                  // Engage first special mode!
						specModeIdx = 0;
						modeIdx = specialModes[specModeIdx];
					}
				}
			}
		}
		
		if (pressDuration == CONFIG_ENTER_DUR)
		{
			if ((byLockOutEnable == 1) && (quickClicks == 2) && (wIdleTicks < LOCK_OUT_TICKS))
			{
				modeIdx = 0;
				byLockOutSet = 1 - byLockOutSet;		// invert "LOCK OUT"
			}
			else if (!byLockOutSet)
			{
				modeIdx = 0;
				// turn the light off initially
				TCCR0A = PHASE;
				PWM_LVL = 0;
				ALT_PWM_LVL = 0;
				
				ConfigMode = 1;
				configClicks = 0;
			}
		}

		wTurboTicks = 0;	// Just always reset turbo timer whenever the button is pressed

	  #ifdef VOLTAGE_MON
		adc_ticks = ADC_DELAY;	// Same with the ramp down delay
	  #endif
	}
	
	//-------------------------------------------------------------------------------
   // Not pressed (debounced qualified)
   //-------------------------------------------------------------------------------
	else
	{
		if (ConfigMode > 0)
		{
			if (pressDuration > 0)
			{
				if (pressDuration < LONG_PRESS_DUR)
					++configClicks;
				configIdleTime = 0;
			}
			else
			{
				++configIdleTime;
			}
			pressDuration = 0;
		} // config mode
		
		else if (pressDuration > 0)
		{
   		// Was previously pressed
			if (pressDuration < LONG_PRESS_DUR)
			{
				// normal short click
				if (modeIdx == BATT_CHECK_MODE)		// battery check - reset to OFF
					modeIdx = 0;
				else
				{
					// track quick clicks in a row from OFF
					if ((modeIdx == 0) && !quickClicks)
						quickClicks = 1;
					else if (quickClicks)
						++quickClicks;

					if (byLockOutSet == 0)
					{
						if (modeMemoryEnabled && (modeMemoryLastModeIdx > 0) && (modeIdx == 0))
						{
							modeIdx = modeMemoryLastModeIdx;
							modeMemoryLastModeIdx = 0;
						}
						else if (modeIdx < 16)
						{
							if ((modeIdx > 0) && (wIdleTicks >= IDLE_TIME))
							{
								modeMemoryLastModeIdx = modeIdx;
								prevModeIdx = modeIdx;
								modeIdx = 0;	// Turn OFF the light
							}
							else
							{
								// Short press - normal modes
								if (highToLow)
									prev_mode();
								else
									next_mode();
							}
						}
						else  // special modes
						{
							if (wIdleTicks >= IDLE_TIME)
								modeIdx = prevModeIdx;
							else if (++specModeIdx > specialModesCnt)
								modeIdx = prevModeIdx;  // bail out of special modes
							else
								modeIdx = specialModes[specModeIdx];
						}
					}
				} // ...

				pressDuration = 0;
			} // short click
			
			wIdleTicks = 0;	// reset idle time

		} // previously pressed
		else
		{
			//------------------------------------
			// Not previously pressed
			//------------------------------------
			if (++wIdleTicks == 0)
				wIdleTicks = 30000;		// max it out at 30,000

			if (wIdleTicks > LOCK_OUT_TICKS)
				quickClicks = 0;

			// Only do turbo check when switch isn't pressed
			if (turboTimeoutMode > 0)
				if (modes[modeIdx] == 255)
				{
					if (++wTurboTicks > wTurboTickLimit)
						prev_mode();		// Go to the previous mode
				}

			// Only do voltage monitoring when the switch isn't pressed
		  #ifdef VOLTAGE_MON
			if (adc_ticks > 0)
				--adc_ticks;
			if (adc_ticks == 0)
			{
				// See if conversion is done
				if (ADCSRA & (1 << ADIF))
				{
					// See if voltage is lower than what we were looking for
					if (ADCH < ((modeIdx == 1) ? ADC_CRIT : ADC_LOW))
						++lowbatt_cnt;
					else
						lowbatt_cnt = 0;
				}
				
				// See if it's been low for a while
				if (lowbatt_cnt >= 4)
				{
					prev_mode();
					lowbatt_cnt = 0;
					// If we reach 0 here, main loop will go into sleep mode
					// Restart the counter to when we step down again
					adc_ticks = ADC_DELAY;
				}
				
				// Make sure conversion is running for next time through
				ADCSRA |= (1 << ADSC);
			}
		  #endif
		} // not previously pressed
		
		pressDuration = 0;
	} // Not pressed
}

/**************************************************************************************
* main - main program loop. This is where it all happens...
* ====
**************************************************************************************/
int main(void)
{	
	byte i;

	// Set all ports to input, and turn pull-up resistors on for the inputs we are using
	DDRB = 0x00;
	PORTB = (1 << SWITCH_PIN) | (1 << STAR3_PIN);

	// Set the switch as an interrupt for when we turn pin change interrupts on
	PCMSK = (1 << SWITCH_PIN);
	
    // Set primary and alternate PWN pins for output
    DDRB = (1 << PWM_PIN) | (1 << ALT_PWM_PIN);

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
	// Turn features on or off as needed
	#ifdef VOLTAGE_MON
	ADC_on();
	#else
	ADC_off();
	#endif
	ACSR   |=  (1<<7); //AC off

	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	// Load config settings: mode, mode set, lo-hi, mode memory
	LoadConfig();

	DefineModeSet();

	wTurboTickLimit = pgm_read_word(turboTimeOutVals+turboTimeoutMode);
	
	if (OffTimeEnable)
	{
		if (!noinit_decay)
		{
			// Indicates they did a short press, go to the next mode
			next_mode(); // Will handle wrap around's
			SaveConfig();
		}
	}
	else
		modeIdx = 0;
		
	if (modeIdx == 0)
	{
	   TCCR0A = PHASE;
		blink(2, 80);
	}

	// set noinit data for next boot
	noinit_decay = 0;  // will decay to non-zero after being off for a while

	byte last_modeIdx = 250;	// make it invalid for first time
   byte byPrevLockOutSet = 0;

   byte prevConfigClicks = 0;

	WDT_on();		// Turn it on now (mode can be non-zero on startup)
	
    //  We will never leave this loop.  The WDT will interrupt to check for switch presses
    // and will change the mode if needed. If this loop detects that the mode has changed,
    // run the logic for that mode while continuing to check for a mode change.
	while(1)		// run forever
	{
      //-------------------------------------------------------------------------------
		if (ConfigMode == 0)					// Normal mode
		{
			if (byPrevLockOutSet != byLockOutSet)
			{
				byPrevLockOutSet = byLockOutSet;

				set_output(0,0);
				_delay_ms(200);
				blink(4, 60);
			}

			if (modeIdx != last_modeIdx)
			{
				//---------------------------------------------------
				// Mode Handling - The WDT changed the mode
				//---------------------------------------------------
				if (modeIdx < 16)
				{
					set_mode(modeIdx);      // Set a solid mode here!!
					last_modeIdx = modeIdx;
				}
				else
				{
					last_modeIdx = modeIdx;

					// If coming from a standard mode, suppress alternate PWM output
					ALT_PWM_LVL = 0;

					if (modeIdx == BATT_CHECK_MODE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == BATT_CHECK_MODE)	// Battery Check
						{
							// blink out volts and tenths
							uint8_t result = battcheck();
							battBlink(result >> 5);
							if (modeIdx != BATT_CHECK_MODE)		break;
							_delay_ms(800);
							battBlink(result & 0b00011111);
							if (modeIdx != BATT_CHECK_MODE)		break;
							_delay_ms(2000);
						}
					}

					else if (modeIdx == STROBE_MODE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == STROBE_MODE)      // strobe at 12.5Hz
						{
							strobe(20,60);
						}
					}

#if RANDOM_STROBE
					else if (modeIdx == RANDOM_STROBE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == RANDOM_STROBE)		// pseudo-random strobe
						{
							byte ms = 34 + (pgm_rand() & 0x3f);
							strobe(ms, ms);
							strobe(ms, ms);
						}
					}
#endif

					else if (modeIdx == POLICE_STROBE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == POLICE_STROBE)		// police strobe
						{
							for(i=0;i<8;i++)
							{
								if (modeIdx != POLICE_STROBE)		break;
								strobe(20,40);
							}
							for(i=0;i<8;i++)
							{
								if (modeIdx != POLICE_STROBE)		break;
								strobe(40,80);
							}
						}
					}

					else if (modeIdx == BIKING_STROBE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == BIKING_STROBE)		// police strobe
						{
							// normal version
							for(i=0;i<4;i++)
							{
								if (modeIdx != BIKING_STROBE)		break;
								set_output(255,0);
								_delay_ms(5);
								set_output(0,255);
								_delay_ms(65);
							}
							for(i=0;i<10;i++)
							{
								if (modeIdx != BIKING_STROBE)		break;
								_delay_ms(72);
							}
						}
						set_output(0,0);
					}

					else if (modeIdx == BEACON_2S_MODE)
					{
						TCCR0A = PHASE;   // set all output for PHASE mode
						while (modeIdx == BEACON_2S_MODE)		// Beacon 2 sec mode
						{
							_delay_ms(300);	// pause a little initially
						
							strobe(125,125);		// two flash's
							strobe(125,125);
						
							for (i=0; i < 15; i++)	// 1.5 secs delay
							{
								if (modeIdx != BEACON_2S_MODE)		break;
								_delay_ms(100);
							}
						}
					}

					else if (modeIdx == BEACON_10S_MODE)
						while (modeIdx == BEACON_10S_MODE)		// Beacon 10 sec mode
						{
							TCCR0A = PHASE;   // set all output for PHASE mode

							_delay_ms(300);	// pause a little initially

							strobe(240,240);		// two slow flash's
							strobe(240,240);

							for (i=0; i < 100; i++)	// 10 secs delay
							{
								if (modeIdx != BEACON_10S_MODE)		break;
								_delay_ms(100);
							}
						}
				}
			} // mode change detected
			
			// Be sure switch is not pressed and light is OFF for at least 10 secs
			if ((modeIdx == 0) && !is_pressed() && (wIdleTicks > 625))
			{
				wIdleTicks = 0;
				_delay_ms(1); // Need this here, maybe instructions for PWM output not getting executed before shutdown?
				sleep_until_switch_press();	// Go to sleep
			}
		}
		
      //-------------------------------------------------------------------------------
		else                             // Configuration mode in effect
		{
			if (configClicks != prevConfigClicks)
			{
				prevConfigClicks = configClicks;
				if (configClicks > 0)
					clickBlink();
			}
			
			if (ConfigMode != prevConfigMode)
			{
				prevConfigMode = ConfigMode;
				configIdleTime = 0;

				switch (ConfigMode)
				{
					case 1:
						_delay_ms(500);
						blink(2, 80);
						++ConfigMode;
						configClicks = 0;
						break;
						
					case 3:	// 1 - exiting mode set selection
						if ((configClicks > 0) && (configClicks <= 8))
						{
							modeSetIdx = configClicks - 1;
							DefineModeSet();
							SaveConfig();
						}
						blink(2, 80);
						break;

					case 4:	// 2 - exiting moonlight enabling
						if (configClicks)
						{
							moonLightEnable = 1 - (configClicks & 1);
							DefineModeSet();
							SaveConfig();
						}
						blink(2, 80);
						break;

					case 5:	// 3 - exiting mode order setting
						if (configClicks)
						{
							highToLow = 1 - (configClicks & 1);
							SaveConfig();
						}
						blink(2, 80);
						break;

					case 6:	// 4 - exiting mode memory setting
						if (configClicks)
						{
							modeMemoryEnabled = 1 - (configClicks & 1);
							SaveConfig();
						}
						blink(2, 80);
						break;
						
					case 7:	// 5 - exiting turbo timeout setting
						if ((configClicks > 0) && (configClicks <= 8))
						{
							turboTimeoutMode = configClicks - 1;
							
							// Set the updated Turbo Tick count limit
							wTurboTickLimit = pgm_read_word(turboTimeOutVals+turboTimeoutMode);
							SaveConfig();
						}
						ConfigMode = 0;		// Exit Config mode
						blink(3, 80);
						modeIdx = 0;
						break;

				} // switch on new config mode
				
				configClicks = 0;

			} // ConfigMode changed

			else if (configIdleTime > 250)		// 4 secs
			{
				++ConfigMode;
			}
			
		} // config mode
	} // while(1)

   return 0; // Standard Return Code
}