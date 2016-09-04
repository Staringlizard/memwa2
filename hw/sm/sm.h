/*
 * memwa2 state machine
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
 

#ifndef _SM_H
#define _SM_H

#include "stm32f7xx_hal.h"
#include "main.h"

typedef enum
{
    SM_STATE_MENU,
    SM_STATE_FILE_SELECT,
    SM_STATE_COLOR_SCREEN,
    SM_STATE_EMULATOR,
    SM_STATE_ERROR,
} sm_state_t;

void sm_init();
void sm_run();
uint8_t sm_get_disp_stats_flag();
void sm_error_occured();
sm_state_t sm_get_state();
void sm_tape_play(uint8_t play);

#endif