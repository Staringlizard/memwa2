/*
 * memwa2 cpu component
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


#ifndef _CPU_H
#define _CPU_H

#include "emuccif.h"

#define PORT_DIRECTION_DEFAULT  0x2F
#define PORT_DEFAULT            0x37

#define REG_PORT_DIRECTION      0x0000
#define REG_PORT_INPUT_OUTPUT   0x0001

typedef struct
{
  uint8_t addr_zero;
  uint8_t addr_one;
} cpu_on_chip_port_t;

#define INTR_NMI  0x0
#define INTR_IRQ  0x1

uint32_t cpu_step();
void cpu_init();
void cpu_reset();
void cpu_clear_nmi();
uint8_t cpu_get_nmi();

#endif
