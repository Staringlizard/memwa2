/*
 * memwa2 keyboard component
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


#ifndef _KEY_H
#define _KEY_H

#include "emuccif.h"

#define REG_KEY_PORT_A    0xDC00
#define REG_KEY_PORT_B    0xDC01

void key_pressed(uint8_t key, uint8_t shift, uint8_t ctrl);
void key_clear_keys();
void key_map_insert(uint8_t scan_code, uint8_t matrix_x, uint8_t matrix_y);
uint8_t key_init();

#endif
