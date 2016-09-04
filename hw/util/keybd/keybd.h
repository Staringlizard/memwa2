/*
 * memwa2 keyboard utility
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


#ifndef _KEYBD_H
#define _KEYBD_H

#include "stm32f7xx_hal.h"
#include "main.h"
#include "if.h"

typedef enum
{
    KEYBD_STATE_RELEASED,
    KEYBD_STATE_PRESSED
} keybd_state_t;

void keybd_init();
void keybd_poll();
keybd_state_t keybd_key_state();
keybd_state_t keybd_get_shift_state();
keybd_state_t keybd_get_ctrl_state();
uint8_t keybd_get_active_key();
uint8_t *keybd_get_active_keys();
uint8_t keybd_get_active_keys_hash();
uint8_t keybd_get_active_number_of_keys();
uint8_t keybd_get_active_ascii_key();
if_keybd_map_t *keybd_get_default_map();
void keybd_populate_map(uint8_t *conf_text, if_keybd_map_t *keybd_map_p);

#endif
