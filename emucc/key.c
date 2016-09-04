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


/**
 * Contains functions for controlling the keyboard matrix
 * connected to the keyboard. These functions will indirectly
 * inform CIA about which keys that are pressed.
 */

#include "key.h"
#include "joy.h"
#include <string.h>

#define JOYSTICK1_UP_PORT_VAL     0xFE
#define JOYSTICK1_DOWN_PORT_VAL   0xFD
#define JOYSTICK1_LEFT_PORT_VAL   0xFB
#define JOYSTICK1_RIGHT_PORT_VAL  0xF7
#define JOYSTICK1_FIRE_PORT_VAL   0xEF

#define JOYSTICK2_UP_PORT_VAL     0xFE
#define JOYSTICK2_DOWN_PORT_VAL   0xFD
#define JOYSTICK2_LEFT_PORT_VAL   0xFB
#define JOYSTICK2_RIGHT_PORT_VAL  0xF7
#define JOYSTICK2_FIRE_PORT_VAL   0xEF

typedef enum
{
    KEY_RELEASE,
    KEY_PRESS
} key_state_t;

typedef struct
{
    uint8_t matrix_x;
    uint8_t matrix_y;
    uint8_t active;
} key_map_t;

typedef struct
{
  uint8_t col;
  uint8_t row;
} key_coor_t;

/* Contains matrix of pushed keys */
uint8_t g_matrix_active_aa[8][8];

static key_map_t g_key_map_a[256]; /* No scan code is larger than this it seems */

void key_pressed(uint8_t key, uint8_t shift, uint8_t ctrl)
{
  if(shift == KEY_PRESS)
  {
    g_matrix_active_aa[0x7][0x1] = KEY_PRESS;
  }

  if(ctrl == KEY_PRESS)
  {
    g_matrix_active_aa[0x2][0x7] = KEY_PRESS;
  }

  if(g_key_map_a[key].active)
  {
    if(g_key_map_a[key].matrix_x < 8 && g_key_map_a[key].matrix_y < 8)
    {
      /* Normal key */
      g_matrix_active_aa[g_key_map_a[key].matrix_x][g_key_map_a[key].matrix_y] = KEY_PRESS;
    }
    else
    {
      /* Joystick key */
      if(g_key_map_a[key].matrix_x == 8) /* This means joystick A */
      {
        switch(g_key_map_a[key].matrix_y)
        {
        case 0:
          joy_set(JOY_PORT_A, JOY_ACTION_UP, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 1:
          joy_set(JOY_PORT_A, JOY_ACTION_DOWN, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 2:
          joy_set(JOY_PORT_A, JOY_ACTION_RIGHT, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 3:
          joy_set(JOY_PORT_A, JOY_ACTION_LEFT, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 4:
          joy_set(JOY_PORT_A, JOY_ACTION_FIRE, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        }
      }
      else if(g_key_map_a[key].matrix_y == 8) /* This means joystick B */
      {
        switch(g_key_map_a[key].matrix_x)
        {
        case 0:
          joy_set(JOY_PORT_B, JOY_ACTION_UP, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 1:
          joy_set(JOY_PORT_B, JOY_ACTION_DOWN, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 2:
          joy_set(JOY_PORT_B, JOY_ACTION_RIGHT, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 3:
          joy_set(JOY_PORT_B, JOY_ACTION_LEFT, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        case 4:
          joy_set(JOY_PORT_B, JOY_ACTION_FIRE, JOY_ACTION_STATE_PRESSED, JOY_ACTION_SOURCE_KEY);
          break;
        }
      }
    }
  }
}

void key_clear_keys()
{
  memset(g_matrix_active_aa, 0, sizeof(g_matrix_active_aa));
}

void key_map_insert(uint8_t scan_code, uint8_t matrix_x, uint8_t matrix_y)
{
  g_key_map_a[scan_code].matrix_x = matrix_x;
  g_key_map_a[scan_code].matrix_y = matrix_y;
  g_key_map_a[scan_code].active = 1;
}

uint8_t key_init()
{
  uint8_t i;
  uint8_t k;

  memset(g_matrix_active_aa, 0x00, sizeof(g_matrix_active_aa));

  for(i = 0; i < 2; i++)
  {
    for(k = 0; k < 5; k++)
    {
      joy_set(i, k, JOY_ACTION_STATE_RELEASED, JOY_ACTION_SOURCE_KEY);
    }
  }

  return 0;
}
