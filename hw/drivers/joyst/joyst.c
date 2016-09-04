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


/**
 * Handles both joysticks.
 */

#include "joyst.h"
#include "if.h"

extern if_emu_cc_t g_if_cc_emu;

__weak void HAL_JOYST_MspInit()
{
  ;
}

void joyst_irq(joyst_port_t joyst_port, joyst_action_t joyst_action, joyst_action_state_t action_state)
{
  /* All types happens to fit perfectly to interface, so just cast and forget :) */
  g_if_cc_emu.if_emu_cc_ue.ue_joyst_fp((if_joyst_port_t)joyst_port,
                               (if_joyst_action_t)joyst_action,
                               (if_joyst_action_state_t)action_state);
}

void joyst_init()
{
    HAL_JOYST_MspInit();
}
