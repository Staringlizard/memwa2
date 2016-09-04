/*
 * memwa2 adv7511w driver
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


#ifndef _ADV7511_H
#define _ADV7511_H

#include "stm32f7xx_hal.h"
#include "main.h"

void adv7511_init();
void adv7511_configure();
void adv7511_wr_reg(uint8_t reg, uint8_t val);
uint8_t adv7511_rd_reg(uint8_t reg);
void adv7511_set_bits(uint8_t reg, uint8_t bits_to_set);
void adv7511_clear_bits(uint8_t reg, uint8_t bits_to_clear);
void adv7511_irq();

#endif
