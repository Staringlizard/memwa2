/*
 * memwa2 joystick component
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
 

#ifndef _JOY_H
#define _JOY_H

#include "emuccif.h"

#define JOY_A_UP_PORT_VAL     0xFE /* Connected to PB0 */
#define JOY_A_DOWN_PORT_VAL   0xFD /* Connected to PB1 */
#define JOY_A_LEFT_PORT_VAL   0xFB /* Connected to PB2 */
#define JOY_A_RIGHT_PORT_VAL  0xF7 /* Connected to PB3 */
#define JOY_A_FIRE_PORT_VAL   0xEF /* Connected to PB4 */

#define JOY_B_UP_PORT_VAL     0xFE /* Connected to PA0 */
#define JOY_B_DOWN_PORT_VAL   0xFD /* Connected to PA1 */
#define JOY_B_LEFT_PORT_VAL   0xFB /* Connected to PA2 */
#define JOY_B_RIGHT_PORT_VAL  0xF7 /* Connected to PA3 */
#define JOY_B_FIRE_PORT_VAL   0xEF /* Connected to PA4 */

typedef enum
{
    JOY_PORT_A,
    JOY_PORT_B
} joy_port_t;

typedef enum
{
    JOY_ACTION_UP,
    JOY_ACTION_DOWN,
    JOY_ACTION_RIGHT,
    JOY_ACTION_LEFT,
    JOY_ACTION_FIRE
} joy_action_t;

typedef enum
{
    JOY_ACTION_STATE_RELEASED,
    JOY_ACTION_STATE_PRESSED
} joy_action_state_t;

typedef enum
{
    JOY_ACTION_SOURCE_JOY, /* Will always have priority */
    JOY_ACTION_SOURCE_KEY
} joy_action_source_t;

void joy_set(joy_port_t joy_port,
             joy_action_t joy_action,
             joy_action_state_t joy_action_state,
             joy_action_source_t joy_action_source);
void joy_clear(joy_action_source_t joy_action_source);
uint8_t joy_init();

#endif
