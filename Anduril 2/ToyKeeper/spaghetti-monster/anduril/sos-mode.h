/*
 * sos-mode.h: SOS mode for Anduril.
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

#ifndef SOS_MODE_H
#define SOS_MODE_H

#ifdef USE_SOS_MODE_IN_BLINKY_GROUP
// automatic SOS emergency signal
uint8_t sos_state(Event event, uint16_t arg);
#endif

uint8_t sos_easter_egg = 0; // MODIFICATION

#endif
