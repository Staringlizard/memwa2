/*
 * memwa2 host-emulator interface
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

#ifndef _EMUCCIF_H
#define _EMUCCIF_H

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

#define UPPER_BORDER       32
#define LOWER_BORDER       50
#define LEFT_BORDER        44
#define RIGHT_BORDER       36
#define FORGROUND_WIDTH    (320 + LEFT_BORDER + RIGHT_BORDER)
#define FORGROUND_HEIGHT   (200 + UPPER_BORDER + LOWER_BORDER)

#endif

