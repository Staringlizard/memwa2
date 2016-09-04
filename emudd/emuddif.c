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
 * DD (Disk Drive) interface implemenation. Contains all functions
 * necessary for the host to work. The interface for this file is dictated
 * by if.h file.
 */

#include "emuddif.h"
#include "bus.h"
#include "via.h"
#include "cpu.h"
#include "if.h"
#include "fdd.h"

void if_emu_dd_mem_set(uint8_t *mem_p, if_mem_dd_type_t mem_type);
void if_emu_dd_op_init();
void if_emu_dd_op_run(int32_t cycles);
void if_emu_dd_op_reset();
void if_emu_dd_disk_drive_load(uint32_t *fd_p);
void if_emu_dd_ports_write_serial(uint8_t data);

static int32_t g_cycle_queue;

if_emu_dd_t g_if_dd_emu =
{
  {
    if_emu_dd_mem_set
  },
  {
    if_emu_dd_op_init,
    if_emu_dd_op_run,
    if_emu_dd_op_reset
  },
  {
    if_emu_dd_disk_drive_load
  },
  {
    if_emu_dd_ports_write_serial
  }
};

void if_emu_dd_mem_set(uint8_t *mem_p, if_mem_dd_type_t mem_type)
{
  switch(mem_type)
  {
    case IF_MEM_DD_TYPE_ALL:
    case IF_MEM_DD_TYPE_UTIL1:
    case IF_MEM_DD_TYPE_UTIL2:
      bus_dd_set_memory(mem_p, (memory_bank_dd_t)mem_type);
      break;
}
}

void if_emu_dd_op_init()
{
  bus_dd_init();
  via_init();
  cpu_dd_init();
  fdd_init();

  /* Lets boot it up a bit before halting */
  cpu_dd_boot();
}

void if_emu_dd_op_run(int32_t cycles)
{
  uint32_t cc = 0;

  g_cycle_queue += cycles;

  while(g_cycle_queue > 0)
  {
    cc = cpu_dd_step();

    via_step(cc);

    g_cycle_queue -= cc;
  }
}

void if_emu_dd_op_reset()
{
  cpu_dd_reset();
  /* Lets boot it up a bit before halting */
  cpu_dd_boot();
}

void if_emu_dd_disk_drive_load(uint32_t *fd_p)
{
  fdd_insert_disk(fd_p);
}

void if_emu_dd_ports_write_serial(uint8_t data)
{
  via_serial_port_activity(data);
}
