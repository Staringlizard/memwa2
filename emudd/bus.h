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


#ifndef _BUS_H
#define _BUS_H

#include "emuddif.h"

/* MEMORY SIZE */

#define MEMORY_ALL_SIZE     0x10000
#define MEMORY_RAM_SIZE     0xC000

/* MEMORY OFFSETS */

#define OFFSET_RAM          0x0000
#define OFFSET_ROM          0xC000
#define OFFSET_STACK        0x0100

typedef uint8_t (*bus_event_read_t)(uint16_t addr);
typedef void (*bus_event_write_t)(uint16_t addr, uint8_t value);

typedef enum
{
  MEMORY_ALL,
  MEMORY_UTIL1,
  MEMORY_UTIL2
} memory_bank_dd_t;

typedef struct
{
  uint8_t *all_p;
  bus_event_read_t *event_read_fpp;
  bus_event_write_t *event_write_fpp;
} memory_dd_t;

void bus_dd_init();
uint8_t bus_dd_read_byte(uint16_t addr);
void bus_dd_write_byte(uint16_t addr, uint8_t byte);
void bus_dd_set_memory(uint8_t *mem_p, memory_bank_dd_t memory_bank);
uint8_t *bus_dd_translate_emu_to_host_addr(uint16_t addr);
void bus_dd_event_read_subscribe(uint16_t addr, bus_event_read_t bus_event_read_fp);
void bus_dd_event_read_unsubscribe(uint16_t addr, bus_event_read_t bus_event_read_fp);
void bus_dd_event_write_subscribe(uint16_t addr, bus_event_write_t bus_event_write_fp);
void bus_dd_event_write_unsubscribe(uint16_t addr, bus_event_write_t bus_event_write_fp);

#endif
