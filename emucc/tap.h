/*
 * memwa2 tape (datasette) component
 *
 * Copyright (c) 2016 Mathias Edman <mail@dicetec.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 *
 */


#ifndef _TAP_H
#define _TAP_H

#include "emuccif.h"

#define MASK_DATASETTE_OUTPUT_SIGNAL_LEVEL      0x08
#define MASK_DATASETTE_BUTTON_STATUS            0x10
#define MASK_DATASETTE_MOTOR_CTRL               0x20

void tap_init();
void tap_play();
void tap_step(uint8_t cc);
void tap_stop();
void tap_insert_tape(uint32_t *fd_p);
void tap_set_motor(uint8_t status);
uint8_t tap_get_play();

#endif
