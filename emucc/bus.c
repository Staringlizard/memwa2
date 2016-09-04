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
#include "tap.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* MEMORY MAP */

/*
 *            +----------------------------+
 *            |       8K KERNAL ROM        |
 * E000-FFFF  |           OR RAM           |
 *            +----------------------------+
 * D000-DFFF  | 4K I/O OR RAM OR CHAR. ROM |
 *            +----------------------------+
 * C000-CFFF  |           4K RAM           |
 *            +----------------------------+
 *            |    8K BASIC ROM OR RAM     |
 * A000-BFFF  |       OR ROM PLUG-IN       |
 *            +----------------------------+
 *            |            8K RAM          |
 * 8000-9FFF  |       OR ROM PLUG-IN       |
 *            +----------------------------+
 *            |                            |
 *            |                            |
 *            |          16 K RAM          |
 * 4000-7FFF  |                            |
 *            +----------------------------+
 *            |                            |
 *            |                            |
 *            |          16 K RAM          |
 * 0000-3FFF  |                            |
 *            +----------------------------+
 */


/* ADDRESS 0x1 */

/*
 * 7 - unused (Flash 8: 0=8MHz/1=1MHz)
 * 6 - unused (C128: ASCII/DIN sense/switch (1=ASCII/0=DIN))
 * 5 - Cassette motor control (0 = motor on)
 * 4 - Cassette switch sense (0 = PLAY pressed)
 * 3 - Cassette write line
 * 2 - CHAREN (0=Character ROM instead of I/O area)
 * 1 - HIRAM ($E000-$FFFF)
 * 0 - LORAM ($A000-$BFFF)
 */

/* I/O BREAKDOWN */

/*
 *  D000-D3FF   VIC (Video Controller)                     1 K Bytes
 *  D400-D7FF   SID (Sound Synthesizer)                    1 K Bytes
 *  D800-DBFF   Color RAM                                  1 K Nybbles
 *  DC00-DCFF   CIA1 (Keyboard)                            256 Bytes
 *  DD00-DDFF   CIA2 (Serial Bus, User Port/RS-232)        256 Bytes
 *  DE00-DEFF   Open I/O slot #l (CP/M Enable)             256 Bytes
 *  DF00-DFFF   Open I/O slot #2 (Disk)                    256 Bytes
 */

/* MEMORY MUX */

/*
 * A000-C000   D000-E000   E000-FFFF
 * RAM         RAM         RAM
 * RAM         Charset     RAM
 * RAM         Charset     Kernal
 * Basic       Charset     Kernal
 * RAM         RAM         RAM
 * RAM         IO          RAM
 * RAM         IO          Kernal
 * Basic       IO          Kernal
 */
                                 
#define MASK_MEMORY_CONFIG_0 0x0
#define MASK_MEMORY_CONFIG_1 0x1
#define MASK_MEMORY_CONFIG_2 0x2
#define MASK_MEMORY_CONFIG_3 0x3
#define MASK_MEMORY_CONFIG_4 0x4
#define MASK_MEMORY_CONFIG_5 0x5
#define MASK_MEMORY_CONFIG_6 0x6
#define MASK_MEMORY_CONFIG_7 0x7

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

static memory_config_t g_memory_config_a[4];
memory_t g_memory;

void bus_mem_conifig(uint8_t mem_config)
{
  g_memory_config_a[0].memory_p = g_memory.ram_p;
  g_memory_config_a[0].memory_type = MEMORY_TYPE_READ_WRITE;
  switch(mem_config)
  {
    case MASK_MEMORY_CONFIG_0:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.ram_p;
      g_memory_config_a[3].memory_p = g_memory.ram_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_WRITE;
    break;
    case MASK_MEMORY_CONFIG_1:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.crom_p;
      g_memory_config_a[3].memory_p = g_memory.ram_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_ONLY;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_WRITE;
    break;
    case MASK_MEMORY_CONFIG_2:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.crom_p;
      g_memory_config_a[3].memory_p = g_memory.krom_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_ONLY;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_ONLY;
    break;
    case MASK_MEMORY_CONFIG_3:
      g_memory_config_a[1].memory_p = g_memory.brom_p;
      g_memory_config_a[2].memory_p = g_memory.crom_p;
      g_memory_config_a[3].memory_p = g_memory.krom_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_ONLY;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_ONLY;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_ONLY;
    break;
    case MASK_MEMORY_CONFIG_4:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.ram_p;
      g_memory_config_a[3].memory_p = g_memory.ram_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_WRITE;
    break;
    case MASK_MEMORY_CONFIG_5:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.io_p;
      g_memory_config_a[3].memory_p = g_memory.ram_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_WRITE;
    break;
    case MASK_MEMORY_CONFIG_6:
      g_memory_config_a[1].memory_p = g_memory.ram_p;
      g_memory_config_a[2].memory_p = g_memory.io_p;
      g_memory_config_a[3].memory_p = g_memory.krom_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_ONLY;
    break;
    case MASK_MEMORY_CONFIG_7:
      g_memory_config_a[1].memory_p = g_memory.brom_p;
      g_memory_config_a[2].memory_p = g_memory.io_p;
      g_memory_config_a[3].memory_p = g_memory.krom_p;
      g_memory_config_a[1].memory_type = MEMORY_TYPE_READ_ONLY;
      g_memory_config_a[2].memory_type = MEMORY_TYPE_READ_WRITE;
      g_memory_config_a[3].memory_type = MEMORY_TYPE_READ_ONLY;
    break;
  }
}

void bus_set_memory(uint8_t *mem_p, memory_bank_t memory_bank)
{
  switch(memory_bank)
  {
    case MEMORY_RAM:
      g_memory.ram_p = mem_p;
    break;
    case MEMORY_KROM:
      g_memory.krom_p = mem_p;
    break;
    case MEMORY_BROM:
      g_memory.brom_p = mem_p;
    break;
    case MEMORY_CROM:
      g_memory.crom_p = mem_p;
    break;
    case MEMORY_IO:
      g_memory.io_p = mem_p;
    break;
    case MEMORY_UTIL1:
      g_memory.event_read_fpp = (bus_event_read_t *)mem_p;
    break;
    case MEMORY_UTIL2:
      g_memory.event_write_fpp = (bus_event_write_t *)mem_p;
    break;
  }
}

/*
 * Write and Read subscriptions can be made exactly once for an addr < 0xA000
 * and 0xD000 < addr < 0xE000.
 */
void bus_event_read_subscribe(uint16_t addr, bus_event_read_t bus_event_read_fp)
{
  g_memory.event_read_fpp[addr] = bus_event_read_fp;
}

void bus_event_read_unsubscribe(uint16_t addr, bus_event_read_t bus_event_read_fp)
{
  g_memory.event_read_fpp[addr] = bus_event_read_fp;
}

void bus_event_write_subscribe(uint16_t addr, bus_event_write_t bus_event_write_fp)
{
  g_memory.event_write_fpp[addr] = bus_event_write_fp;
}

void bus_event_write_unsubscribe(uint16_t addr, bus_event_write_t bus_event_write_fp)
{
  g_memory.event_write_fpp[addr] = NULL;
}

void bus_init()
{
  bus_mem_conifig(MASK_MEMORY_CONFIG_7);

  memset(g_memory.ram_p, 0x00, MEMORY_RAM_SIZE);
  memset(g_memory.io_p, 0x00, MEMORY_IO_SIZE);
}

uint8_t bus_read_byte(uint16_t addr)
{
  if(addr >= 0 && addr < 0xA000)
  {
    if(g_memory.event_read_fpp[addr] != NULL)
    {
      /* If subscribed, then bus takes no responsibility for this address */
      return g_memory.event_read_fpp[addr](addr);
    }
    return g_memory_config_a[0].memory_p[addr];
  }
  else if(addr >= 0xA000 && addr < 0xC000)
  {
    return g_memory_config_a[1].memory_p[addr];
  }
  else if(addr >= 0xC000 && addr < 0xD000)
  {
    return g_memory_config_a[0].memory_p[addr];
  }
  else if(addr >= 0xD000 && addr < 0xE000)
  {
    if(g_memory_config_a[2].memory_p == g_memory.io_p)
    {
      /*
       * Some areas in io memory are not mapped, therefore any address
       * in this unmapped memory will overflow and be repeated.
       */
      if(addr >= 0xD040 && addr < 0xD400) /* Vic repeat */
      {
        addr &= ~0x03C0;
      }
      else if(addr >= 0xD420 && addr < 0xD800) /* Sid repeat */
      {
        addr &= ~0x03E0;
      }
      else if(addr >= 0xDC10 && addr < 0xDD00) /* Cia1 repeat */
      {
        addr &= ~0x00F0;
      }
      else if(addr >= 0xDD10 && addr < 0xDE00) /* Cia2 repeat */
      {
        addr &= ~0x00F0;
      }

      if(g_memory.event_read_fpp[addr] != NULL)
      {
        /* If subscribed, then bus takes no responsibility for this address */
        return g_memory.event_read_fpp[addr](addr);
      }
    }

    return g_memory_config_a[2].memory_p[addr];
  }
  else if(addr >= 0xE000 && addr <= 0xFFFF)
  {
    return g_memory_config_a[3].memory_p[addr];
  }
  else
  {
    assert(0);
  }
}

void bus_write_byte(uint16_t addr, uint8_t byte)
{
  if(addr >= 0 && addr < 0xA000)
  {
    if(g_memory.event_write_fpp[addr] != NULL)
    {
      /* If subscribed, then bus takes no responsibility for this address */
      g_memory.event_write_fpp[addr](addr, byte);
      return;
    }
    
    g_memory_config_a[0].memory_p[addr] = byte;
  }
  else if(addr >= 0xA000 && addr < 0xC000)
  {
    if(g_memory_config_a[1].memory_type == MEMORY_TYPE_READ_WRITE)
    {
      g_memory_config_a[1].memory_p[addr] = byte;
    }
    else
    {
      /* If trying to write to rom, then ram will be updated instead */
      g_memory_config_a[0].memory_p[addr] = byte;
    }
  }
  else if(addr >= 0xC000 && addr < 0xD000)
  {
    g_memory_config_a[0].memory_p[addr] = byte;
  }
  else if(addr >= 0xD000 && addr < 0xE000)
  {
    if(g_memory_config_a[2].memory_type == MEMORY_TYPE_READ_WRITE)
    {
      if(g_memory_config_a[2].memory_p == g_memory.io_p)
      {
        /*
         * Some areas in io memory are not mapped, therefore any address
         * in this unmapped memory will overflow and be repeated.
         */
        if(addr >= 0xD040 && addr < 0xD400) /* Vic repeat */
        {
          addr &= ~0x03C0;
        }
        else if(addr >= 0xD420 && addr < 0xD800) /* Sid repeat */
        {
          addr &= ~0x03E0;
        }
        else if(addr >= 0xDC10 && addr < 0xDD00) /* Cia1 repeat */
        {
          addr &= ~0x00F0;
        }
        else if(addr >= 0xDD10 && addr < 0xDE00) /* Cia2 repeat */
        {
          addr &= ~0x00F0;
        }

        if(g_memory.event_write_fpp[addr] != NULL)
        {
          /* If subscribed, then bus takes no responsibility for this address */
          g_memory.event_write_fpp[addr](addr, byte);
          return;
        }
      }

      g_memory_config_a[2].memory_p[addr] = byte;
    }
    else
    {
      /* If trying to write to rom, then ram will be updated instead */
      g_memory_config_a[0].memory_p[addr] = byte;
    }
  }
  else if(addr >= 0xE000 && addr <= 0xFFFF)
  {
    if(g_memory_config_a[3].memory_type == MEMORY_TYPE_READ_WRITE)
    {
      g_memory_config_a[3].memory_p[addr] = byte;
    }
    else
    {
      /* If trying to write to rom, then ram will be updated instead */
      g_memory_config_a[0].memory_p[addr] = byte;
    }
  }
}

uint8_t *bus_translate_emu_to_host_addr(uint16_t addr)
{
  if(addr >= 0 && addr < 0xA000)
  {
    return g_memory_config_a[0].memory_p + addr;
  }
  else if(addr >= 0xA000 && addr < 0xC000)
  {
    return g_memory_config_a[1].memory_p + addr;
  }
  else if(addr >= 0xC000 && addr < 0xD000)
  {
    return g_memory_config_a[0].memory_p + addr;
  }
  else if(addr >= 0xD000 && addr < 0xE000)
  {
    return g_memory_config_a[2].memory_p + addr;
  }
  else if(addr >= 0xE000 && addr <= 0xFFFF)
  {
    return g_memory_config_a[3].memory_p + addr;
  }
  else
  {
    assert(0);
  }

  return g_memory_config_a[3].memory_p + addr;
}
