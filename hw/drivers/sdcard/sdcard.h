/*
 * memwa2 sdcard driver
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


#ifndef _SDCARD_H
#define _SDCARD_H

#include "stm32f7xx_hal_dma.h"
#include "stm32f7xx_hal_sd.h"
#include "stm32f7xx_hal_hcd.h"
#include "stm32f7xx_hal_i2c.h"
#include "stm32f7xx_hal_sdram.h"
#include "main.h"

void sdcard_init();
uint8_t sdcard_inserted();

#endif
