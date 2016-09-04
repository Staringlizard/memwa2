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


/**
 * Contains functions for controlling the keyboard matrix
 * connected to the joysticks. These functions will indirectly
 * inform CIA about which keys that are pressed.
 */

#include "joy.h"
#include <string.h>

/* First is joystick A and B, second is direction. Contains action status */
joy_action_state_t g_joy_action_state_aa[2][5] =
{
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}
};

/* First is joystick A and B, second is direction. Contains source for an action */
static joy_action_source_t g_joy_action_source_aa[2][5] =
{
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0}
};

void joy_set(joy_port_t joy_port,
             joy_action_t joy_action,
             joy_action_state_t joy_action_state,
             joy_action_source_t joy_action_source)
{
  switch(joy_action_source)
  {
    case JOY_ACTION_SOURCE_JOY:
      g_joy_action_state_aa[joy_port][joy_action] = joy_action_state;
      g_joy_action_source_aa[joy_port][joy_action] = joy_action_source;
      break;
    case JOY_ACTION_SOURCE_KEY:
      /* If joy is pressed and action was set by the joystick itself... */
      if(g_joy_action_state_aa[joy_port][joy_action] == JOY_ACTION_STATE_PRESSED &&
         g_joy_action_source_aa[joy_port][joy_action] == JOY_ACTION_SOURCE_JOY)
      {
        ; /* Then action by keyboard should be ignored */
      }
      else
      {
        /* In any other case it is safe to let keyboard action modify state */
        g_joy_action_state_aa[joy_port][joy_action] = joy_action_state;
        g_joy_action_source_aa[joy_port][joy_action] = joy_action_source;
      }
      break;
  }

}

void joy_clear(joy_action_source_t joy_action_source)
{
  uint8_t i;
  uint8_t k;

  for(i = 0; i < 2; i++)
  {
    for(k = 0; k < 5; k ++)
    {
      if(g_joy_action_source_aa[i][k] == joy_action_source)
      {
        g_joy_action_state_aa[i][k] = JOY_ACTION_STATE_RELEASED;
      }
    }
  }
}

uint8_t joy_init()
{
  memset(g_joy_action_state_aa, 0x00, sizeof(g_joy_action_state_aa));
  return 0;
}
