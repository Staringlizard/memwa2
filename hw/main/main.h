/*
 * memwa2 main
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


#ifndef _MAIN_H
#define _MAIN_H

#include "stm32f7xx_hal.h"
#include <stdio.h>

/* This must ALWAYS be uppdated for every release */
#define FW_REVISION             "V1.0.1"

#define SDRAM_BANK              FMC_SDRAM_BANK1
#define SDRAM_ADDR              0x60000000
#define SDRAM_SIZE              0x800000

#define CC_DISP_BUFFER1_ADDR   (SDRAM_ADDR)
#define CC_DISP_BUFFER2_ADDR   (CC_DISP_BUFFER1_ADDR + IF_MEMORY_CC_SCREEN_BUFFER1_SIZE)
#define CC_DISP_BUFFER3_ADDR   (CC_DISP_BUFFER2_ADDR + IF_MEMORY_CC_SCREEN_BUFFER2_SIZE)
#define CC_SPRITE1_BASE_ADDR   (CC_DISP_BUFFER3_ADDR + IF_MEMORY_CC_SCREEN_BUFFER3_SIZE)
#define CC_SPRITE2_BASE_ADDR   (CC_SPRITE1_BASE_ADDR + IF_MEMORY_CC_SPRITE1_SIZE)
#define CC_SPRITE3_BASE_ADDR   (CC_SPRITE2_BASE_ADDR + IF_MEMORY_CC_SPRITE2_SIZE)
#define CC_RAM_BASE_ADDR       (CC_SPRITE3_BASE_ADDR + IF_MEMORY_CC_SPRITE3_SIZE)

/* These can have same memory space, since they are loaded at different address */
#define CC_KROM_BASE_ADDR      (CC_RAM_BASE_ADDR + IF_MEMORY_CC_RAM_SIZE)
#define CC_BROM_BASE_ADDR      (CC_RAM_BASE_ADDR + IF_MEMORY_CC_RAM_SIZE)
#define CC_CROM_BASE_ADDR      (CC_RAM_BASE_ADDR + IF_MEMORY_CC_RAM_SIZE)

/* CC IO address needs to be unique, lets use the fast DTCM for this memory */
#define CC_IO_BASE_ADDR        0x20000000
#define CC_UTIL1_BASE_ADDR     (CC_CROM_BASE_ADDR + IF_MEMORY_CC_CROM_SIZE)
#define CC_UTIL2_BASE_ADDR     (CC_UTIL1_BASE_ADDR + IF_MEMORY_CC_UTIL1_SIZE)
#define DD_ALL_BASE_ADDR       (CC_UTIL2_BASE_ADDR + IF_MEMORY_CC_UTIL2_SIZE)
#define DD_UTIL1_BASE_ADDR     (DD_ALL_BASE_ADDR + IF_MEMORY_DD_ALL_SIZE)
#define DD_UTIL2_BASE_ADDR     (DD_UTIL1_BASE_ADDR + IF_MEMORY_DD_UTIL1_SIZE)

/*
 * This can get all the remaining memory to support as
 * many filenames as possible.
 */
#define CC_STAGE_FILES_ADDR    (DD_UTIL2_BASE_ADDR + IF_MEMORY_DD_UTIL2_SIZE)

#define CC_BROM_LOAD_ADDR      0x0000A000
#define CC_CROM_LOAD_ADDR      0x0000D000
#define CC_KROM_LOAD_ADDR      0x0000E000

#define DD_DOS_LOAD_ADDR       0x0000C000

void main_error(char *string_p, char *file_p, uint32_t line, uint32_t extra);
void main_warning(char *string_p, char *file_p, uint32_t line, uint32_t extra);
char *main_get_fw_revision();

#endif
