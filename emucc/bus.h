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

#include "emuccif.h"

/* MEMORY SIZE */

#define MEMORY_RAM_SIZE         0x10000
#define MEMORY_KROM_SIZE        0x2000
#define MEMORY_BROM_SIZE        0x2000
#define MEMORY_CROM_SIZE        0x1000
#define MEMORY_IO_SIZE          0x10000
#define MEMORY_VIC_SIZE         0x400
#define MEMORY_SID_SIZE         0x400

/* MEMORY OFFSETS */

#define OFFSET_RAM          0x0000
#define OFFSET_CROM         0xD000
#define OFFSET_STACK        0x0100
#define OFFSET_COLOR_RAM    0xD800

typedef uint8_t (*bus_event_read_t)(uint16_t addr);
typedef void (*bus_event_write_t)(uint16_t addr, uint8_t value);

typedef enum
{
  MEMORY_RAM,
  MEMORY_KROM,
  MEMORY_BROM,
  MEMORY_CROM,
  MEMORY_IO,
  MEMORY_UTIL1,
  MEMORY_UTIL2
} memory_bank_t;

typedef struct
{
  uint8_t *ram_p;
  uint8_t *krom_p;
  uint8_t *brom_p;
  uint8_t *crom_p;
  uint8_t *io_p;
  bus_event_read_t *event_read_fpp;
  bus_event_write_t *event_write_fpp;
} memory_t;

void bus_init();
void bus_mem_conifig(uint8_t mem_config);
uint8_t bus_read_byte(uint16_t addr);
void bus_write_byte(uint16_t addr, uint8_t byte);
void bus_set_memory(uint8_t *mem_p, memory_bank_t memory_bank);
uint8_t *bus_translate_emu_to_host_addr(uint16_t addr);
void bus_event_read_subscribe(uint16_t addr, bus_event_read_t bus_event_read_fp);
void bus_event_read_unsubscribe(uint16_t addr, bus_event_read_t bus_event_read_fp);
void bus_event_write_subscribe(uint16_t addr, bus_event_write_t bus_event_write_fp);
void bus_event_write_unsubscribe(uint16_t addr, bus_event_write_t bus_event_write_fp);

#endif
