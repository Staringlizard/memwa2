/*
 * memwa2 emulator bus
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
 * This is the main bus for the emulator. The cpu is connected to this bus
 * and all data communication from and to the cpu will go through these functions
 * (bus_read_byte, bus_write_byte).
 * All emulated components can subscribe to a memory address on this bus so that
 * they will get a callback when read or write events takes place for the address.
 */

#include "bus.h"
#include "cpu.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* MEMORY MAP */

/*
 *            +----------------------------+
 *            |16KB ROM, DOS and controller|
 * C000-FFFF  |           routines         |
 *            +----------------------------+
 * 1C00-180F  |           VIA 2            |
 *            +----------------------------+
 * 1800-180F  |           VIA 1            |
 *            +----------------------------+
 * 0000-07FF  |          2K RAM            |
 *            +----------------------------+
 */

typedef enum
{
  MEMORY_TYPE_READ_WRITE,
  MEMORY_TYPE_READ_ONLY,
  MEMORY_TYPE_WRITE_ONLY
} memory_type_t;

typedef struct
{
  uint8_t *memory_p;
  memory_type_t memory_type;
} memory_config_t;

static memory_config_t g_memory_config_a[2];
memory_dd_t g_memory_dd;

static void bus_dd_mem_conifig()
{
  g_memory_config_a[0].memory_p = g_memory_dd.all_p;
  g_memory_config_a[0].memory_type = MEMORY_TYPE_READ_WRITE; /* Ok, not really true, but this is not needed anyway... */
}

void bus_dd_set_memory(uint8_t *mem_p, memory_bank_dd_t memory_bank)
{
  switch(memory_bank)
  {
    case MEMORY_ALL:
      g_memory_dd.all_p = mem_p;
    break;
    case MEMORY_UTIL1:
      g_memory_dd.event_read_fpp = (bus_event_read_t *)mem_p;
    break;
    case MEMORY_UTIL2:
      g_memory_dd.event_write_fpp = (bus_event_write_t *)mem_p;
    break;
  }
}

/* Subscriptions can only be made for io and for ram memory up to address 0xA000 */
void bus_dd_event_read_subscribe(uint16_t addr, bus_event_read_t bus_event_read_fp)
{
  g_memory_dd.event_read_fpp[addr] = bus_event_read_fp;
}

void bus_dd_event_read_unsubscribe(uint16_t addr, bus_event_read_t bus_event_read_fp)
{
  g_memory_dd.event_read_fpp[addr] = bus_event_read_fp;
}

void bus_dd_event_write_subscribe(uint16_t addr, bus_event_write_t bus_event_write_fp)
{
  g_memory_dd.event_write_fpp[addr] = bus_event_write_fp;
}

void bus_dd_event_write_unsubscribe(uint16_t addr, bus_event_write_t bus_event_write_fp)
{
  g_memory_dd.event_write_fpp[addr] = NULL;
}

void bus_dd_init()
{
  memset(g_memory_dd.all_p, 0, MEMORY_RAM_SIZE);

  bus_dd_mem_conifig();
}

uint8_t bus_dd_read_byte(uint16_t addr)
{
  if(addr >= MEMORY_ALL && addr < (MEMORY_ALL + MEMORY_ALL_SIZE))
  {
    if(g_memory_dd.event_read_fpp[addr] != NULL)
    {
      /* If subscribed, then bus takes no responsibility for this address */
      return g_memory_dd.event_read_fpp[addr](addr);
    }

    return g_memory_config_a[0].memory_p[addr];
  }
  else
  {
    assert(0);
  }
}

void bus_dd_write_byte(uint16_t addr, uint8_t byte)
{
  if(addr >= MEMORY_ALL && addr < (MEMORY_ALL + MEMORY_ALL_SIZE))
  {
    if(g_memory_dd.event_write_fpp[addr] != NULL)
    {
      /* If subscribed, then bus takes no responsibility for this address */
      g_memory_dd.event_write_fpp[addr](addr, byte);
      return;
    }

    g_memory_config_a[0].memory_p[addr] = byte;
  }
  else
  {
    assert(0);
  }
}

uint8_t *bus_dd_translate_emu_to_host_addr(uint16_t addr)
{
  if(addr >= MEMORY_ALL && addr < (MEMORY_ALL + MEMORY_ALL_SIZE))
  {
    return g_memory_config_a[0].memory_p + addr;
  }
  else
  {
    assert(0);
  }

  return 0;
}
