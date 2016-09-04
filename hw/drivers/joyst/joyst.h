/*
 * memwa2 joystick driver
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



#ifndef _JOYST_H
#define _JOYST_H

#include "stm32f7xx_hal.h"
#include "main.h"

typedef enum
{
    JOYST_PORT_A,
    JOYST_PORT_B
} joyst_port_t;

typedef enum
{
    JOYST_ACTION_UP,
    JOYST_ACTION_DOWN,
    JOYST_ACTION_RIGHT,
    JOYST_ACTION_LEFT,
    JOYST_ACTION_FIRE
} joyst_action_t;

typedef enum
{
    JOYST_ACTION_STATE_RELEASED,
    JOYST_ACTION_STATE_PRESSED
} joyst_action_state_t;

void joyst_irq(joyst_port_t joyst_port, joyst_action_t joyst_action, joyst_action_state_t action_state);
void joyst_init();

#endif
