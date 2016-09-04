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


/**
 * CC (Commodore Computer) interface implemenation. Contains all functions
 * necessary for the host to work. The interface for this file is dictated
 * by if.h file.
 */

#include "emuccif.h"
#include "vic.h"
#include "if.h"
#include "key.h"
#include "joy.h"
#include "bus.h"
#include "cia.h"
#include "cpu.h"
#include "tap.h"
#include "sid.h"

void if_emu_cc_ue_joyst(if_joyst_port_t if_joyst_port, if_joyst_action_t if_joyst_action, if_joyst_action_state_t if_action_state);
void if_emu_cc_ue_keybd(uint8_t *keybd_keys_p, uint8_t max_keys, if_key_state_t key_shift, if_key_state_t key_ctrl);
void if_emu_cc_ue_keybd_map_set(if_keybd_map_t *if_keybd_map_p);
void if_emu_cc_display_layer_set(uint8_t *layer_p);
void if_emu_cc_display_limit_frame_rate(uint8_t active);
void if_emu_cc_display_lock_frame_rate(uint8_t active);
void if_emu_cc_mem_set(uint8_t *mem_p, if_mem_cc_type_t mem_type);
void if_emu_cc_op_init();
void if_emu_cc_op_run(int32_t cycles);
void if_emu_cc_op_reset();
void if_emu_cc_tape_drive_load(uint32_t *fd_p);
void if_emu_cc_tape_drive_play();
void if_emu_cc_tape_drive_stop();
void if_emu_cc_time_tenth_second();
void if_emu_cc_ports_write_serial(uint8_t data);

static int32_t g_cycle_queue;

if_emu_cc_t g_if_cc_emu =
{
  {
    if_emu_cc_ue_joyst,
    if_emu_cc_ue_keybd,
    if_emu_cc_ue_keybd_map_set
  },
  {
    if_emu_cc_display_layer_set,
    if_emu_cc_display_limit_frame_rate,
    if_emu_cc_display_lock_frame_rate
  },
  {
    if_emu_cc_mem_set
  },
  {
    if_emu_cc_op_init,
    if_emu_cc_op_run,
    if_emu_cc_op_reset
  },
  {
    if_emu_cc_tape_drive_load,
    if_emu_cc_tape_drive_play,
    if_emu_cc_tape_drive_stop
  },
  {
    if_emu_cc_time_tenth_second
  },
  {
    if_emu_cc_ports_write_serial
  }
};

void if_emu_cc_ue_joyst(if_joyst_port_t if_joyst_port, if_joyst_action_t if_joyst_action, if_joyst_action_state_t if_joyst_action_state)
{
  /* Types are mapped perfectly to interface so no problemo :) */
  joy_set((joy_port_t)if_joyst_port, (joy_action_t)if_joyst_action, (joy_action_state_t)if_joyst_action_state, JOY_ACTION_SOURCE_JOY);
}

void if_emu_cc_ue_keybd(uint8_t *keybd_keys_p, uint8_t max_keys, if_key_state_t key_shift, if_key_state_t key_ctrl)
{
  uint32_t i;
  key_clear_keys();
  joy_clear(JOY_ACTION_SOURCE_KEY); /* Only joy actions that has been set with keyboard should be cleared */

  for(i = 0; i < max_keys; i++)
  {
    if(keybd_keys_p[i] != 0x00)
    {
      key_pressed(keybd_keys_p[i], (uint8_t)key_shift, (uint8_t)key_ctrl);
    }
  }
}

void if_emu_cc_ue_keybd_map_set(if_keybd_map_t *if_keybd_map_p)
{
  uint8_t i;

  for(i = 0; if_keybd_map_p[i].scan_code != 0; i++)
  {
    key_map_insert(if_keybd_map_p[i].scan_code, if_keybd_map_p[i].matrix_x, if_keybd_map_p[i].matrix_y);
  }
}

void if_emu_cc_display_layer_set(uint8_t *layer_p)
{
  vic_set_layer(layer_p);
}

void if_emu_cc_mem_set(uint8_t *mem_p, if_mem_cc_type_t mem_type)
{
  switch(mem_type)
  {
    case IF_MEM_CC_TYPE_RAM:
    case IF_MEM_CC_TYPE_KROM:
    case IF_MEM_CC_TYPE_BROM:
    case IF_MEM_CC_TYPE_CROM:
    case IF_MEM_CC_TYPE_IO:
    case IF_MEM_CC_TYPE_UTIL1:
    case IF_MEM_CC_TYPE_UTIL2:
      bus_set_memory(mem_p, (memory_bank_t)mem_type);
      break;
    case IF_MEM_CC_TYPE_SPRITE1:
      vic_set_memory(mem_p, VIC_MEM_SPRITE_BACKGROUND);
      break;
    case IF_MEM_CC_TYPE_SPRITE2:
      vic_set_memory(mem_p, VIC_MEM_SPRITE_FORGROUND);
      break;
    case IF_MEM_CC_TYPE_SPRITE3:
      vic_set_memory(mem_p, VIC_MEM_SPRITE_MAP);
      break;
  }
}

void if_emu_cc_op_init()
{
  bus_init();
  vic_init();
  key_init();
  joy_init();
  cia_init();
  tap_init();
  sid_init();
  cpu_init();
}

void if_emu_cc_op_run(int32_t cycles)
{
  uint32_t cc = 0;

  g_cycle_queue += cycles;

  while(g_cycle_queue > 0)
  {
    /*
     * Normally it is vic that will act clock in the system
     * but here, instead it is the cpu that does this. This should not
     * matter though, since vic and cpu is really taking turns at
     * at reading the bus so it can be considered "same time" access.
     */
    cc = cpu_step();

    /*
     * Vic has the ability to "stun" the cpu
     */
    vic_step(cc);
    cia_step(cc);
    tap_step(cc);

    g_cycle_queue -= cc;
  }
}

void if_emu_cc_op_reset()
{
  cpu_reset();
}

void if_emu_cc_tape_drive_load(uint32_t *fd_p)
{
  tap_insert_tape(fd_p);
}

void if_emu_cc_tape_drive_play()
{
  tap_play();
}

void if_emu_cc_tape_drive_stop()
{
  tap_stop();
}

void if_emu_cc_time_tenth_second()
{
  cia_tenth_second();
}

void if_emu_cc_ports_write_serial(uint8_t data)
{
  cia_serial_port_activity(data);
}

void if_emu_cc_display_limit_frame_rate(uint8_t active)
{
    if(active)
    {
        vic_set_half_frame_rate();
    }
    else
    {
        vic_set_full_frame_rate();
    }
}

void if_emu_cc_display_lock_frame_rate(uint8_t active)
{
    if(active)
    {
        vic_lock_frame_rate();
    }
    else
    {
        vic_unlock_frame_rate();
    }
}
