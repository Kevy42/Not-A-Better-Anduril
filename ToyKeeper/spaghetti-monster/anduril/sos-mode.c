/*
 * sos-mode.c: SOS mode for Anduril.
 *
 * Copyright (C) 2017 Selene Scriven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOS_MODE_C
#define SOS_MODE_C

#include "sos-mode.h"

#ifdef USE_SOS_MODE_IN_BLINKY_GROUP
uint8_t sos_state(Event event, uint16_t arg) {
    // 1 click: off
    if (event == EV_1click) {
        sos_easter_egg = 0; // Disable easteregg when exiting // MODIFICATION
        set_state(off_state, 0);
        return MISCHIEF_MANAGED;
    }
    // 2 clicks: next blinky mode
    else if (event == EV_2clicks) {
        #if defined(USE_BATTCHECK_MODE)
        set_state(battcheck_state, 0);
        #elif defined(USE_THERMAL_REGULATION)
        set_state(tempcheck_state, 0);
        #elif defined(USE_BEACON_MODE)
        set_state(beacon_state, 0);
        #endif
        return MISCHIEF_MANAGED;
    }

    // 3 clicks: previous blinky mode // MODIFICATION
    else if (event == EV_3clicks) { // MODIFICATION
        #if defined(USE_BEACON_MODE) // MODIFICATION
        set_state(beacon_state, 0); // MODIFICATION
        #elif defined(USE_THERMAL_REGULATION) // MODIFICATION
        set_state(tempcheck_state, 0); // MODIFICATION
        #elif defined(USE_BATTCHECK) // MODIFICATION
        set_state(battcheck_state, 0); // MODIFICATION
        #endif // MODIFICATION
        return MISCHIEF_MANAGED; // MODIFICATION
    } // MODIFICATION

    // 10 clicks: easter egg // MODIFICATION
    if (event == EV_10clicks) { // MODIFICATION
        sos_easter_egg = 1; // MODIFICATION
        return MISCHIEF_MANAGED; // MODIFICATION
    } // MODIFICATION

    return EVENT_NOT_HANDLED;
}
#endif

void sos_blink(uint8_t num, uint8_t dah) {
    #define DIT_LENGTH 200
    for (; num > 0; num--) {
        set_level(memorized_level);
        nice_delay_ms(DIT_LENGTH);
        if (dah) {  // dah is 3X as long as a dit
            nice_delay_ms(DIT_LENGTH*2);
        }
        set_level(0);
        // one "off" dit between blinks
        nice_delay_ms(DIT_LENGTH);
    }
    // three "off" dits (or one "dah") between letters
    // (except for SOS, which is collectively treated as a single "letter")
    //nice_delay_ms(DIT_LENGTH*2);
}

inline void sos_mode_iter() {
    // one iteration of main loop()
    //nice_delay_ms(1000);

    if (sos_easter_egg == 0) { // MODIFICATION
        sos_blink(3, 0);  // S
        sos_blink(3, 1);  // O
        sos_blink(3, 0);  // S
        nice_delay_ms(2000);
    } else { // MODIFICATION
    
        // "Will ship accordingly" easteregg. // MODIFICATION
        // Disabled (for now) to save space and make all builds compile
/*      sos_blink(1, 0); // MODIFICATION
        sos_blink(2, 1); // MODIFICATION

        sos_blink(2, 0); // MODIFICATION

        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(2, 0); // MODIFICATION
        
        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(2, 0); // MODIFICATION

        nice_delay_ms(1000); // MODIFICATION

        sos_blink(3, 0); // MODIFICATION

        sos_blink(4, 0); // MODIFICATION

        sos_blink(2, 0); // MODIFICATION

        sos_blink(1, 0); // MODIFICATION
        sos_blink(2, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        nice_delay_ms(1000); // MODIFICATION

        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION

        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        sos_blink(3, 1); // MODIFICATION

        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        sos_blink(1, 1); // MODIFICATION
        sos_blink(2, 0); // MODIFICATION
        
        sos_blink(2, 0); // MODIFICATION

        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        sos_blink(2, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION

        sos_blink(1, 0); // MODIFICATION
        sos_blink(1, 1); // MODIFICATION
        sos_blink(2, 0); // MODIFICATION

        sos_blink(1, 1); // MODIFICATION
        sos_blink(1, 0); // MODIFICATION
        sos_blink(2, 1); // MODIFICATION */

        nice_delay_ms(2000); // MODIFICATION
    } // MODIFICATION
}


#endif

