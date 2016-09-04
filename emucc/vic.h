/*
 * memwa2 vic component
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


#ifndef _VIC_H
#define _VIC_H

#include "emuccif.h"

/*

The VIC has 47 read/write registers for the processor to control its
functions:

 #| Adr.  |Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0| Function
--+-------+----+----+----+----+----+----+----+----+------------------------
 0| $d000 |                  M0X                  | X coordinate sprite 0
--+-------+---------------------------------------+------------------------
 1| $d001 |                  M0Y                  | Y coordinate sprite 0
--+-------+---------------------------------------+------------------------
 2| $d002 |                  M1X                  | X coordinate sprite 1
--+-------+---------------------------------------+------------------------
 3| $d003 |                  M1Y                  | Y coordinate sprite 1
--+-------+---------------------------------------+------------------------
 4| $d004 |                  M2X                  | X coordinate sprite 2
--+-------+---------------------------------------+------------------------
 5| $d005 |                  M2Y                  | Y coordinate sprite 2
--+-------+---------------------------------------+------------------------
 6| $d006 |                  M3X                  | X coordinate sprite 3
--+-------+---------------------------------------+------------------------
 7| $d007 |                  M3Y                  | Y coordinate sprite 3
--+-------+---------------------------------------+------------------------
 8| $d008 |                  M4X                  | X coordinate sprite 4
--+-------+---------------------------------------+------------------------
 9| $d009 |                  M4Y                  | Y coordinate sprite 4
--+-------+---------------------------------------+------------------------
10| $d00a |                  M5X                  | X coordinate sprite 5
--+-------+---------------------------------------+------------------------
11| $d00b |                  M5Y                  | Y coordinate sprite 5
--+-------+---------------------------------------+------------------------
12| $d00c |                  M6X                  | X coordinate sprite 6
--+-------+---------------------------------------+------------------------
13| $d00d |                  M6Y                  | Y coordinate sprite 6
--+-------+---------------------------------------+------------------------
14| $d00e |                  M7X                  | X coordinate sprite 7
--+-------+---------------------------------------+------------------------
15| $d00f |                  M7Y                  | Y coordinate sprite 7
--+-------+----+----+----+----+----+----+----+----+------------------------
16| $d010 |M7X8|M6X8|M5X8|M4X8|M3X8|M2X8|M1X8|M0X8| MSBs of X coordinates
--+-------+----+----+----+----+----+----+----+----+------------------------
17| $d011 |RST8| ECM| BMM| DEN|RSEL|    YSCROLL   | Control register 1
--+-------+----+----+----+----+----+--------------+------------------------
18| $d012 |                 RASTER                | Raster counter
--+-------+---------------------------------------+------------------------
19| $d013 |                  LPX                  | Light pen X
--+-------+---------------------------------------+------------------------
20| $d014 |                  LPY                  | Light pen Y
--+-------+----+----+----+----+----+----+----+----+------------------------
21| $d015 | M7E| M6E| M5E| M4E| M3E| M2E| M1E| M0E| Sprite enabled
--+-------+----+----+----+----+----+----+----+----+------------------------
22| $d016 |  - |  - | RES| MCM|CSEL|    XSCROLL   | Control register 2
--+-------+----+----+----+----+----+----+----+----+------------------------
23| $d017 |M7YE|M6YE|M5YE|M4YE|M3YE|M2YE|M1YE|M0YE| Sprite Y expansion
--+-------+----+----+----+----+----+----+----+----+------------------------
24| $d018 |VM13|VM12|VM11|VM10|CB13|CB12|CB11|  - | Memory pointers
--+-------+----+----+----+----+----+----+----+----+------------------------
25| $d019 | IRQ|  - |  - |  - | ILP|IMMC|IMBC|IRST| Interrupt register
--+-------+----+----+----+----+----+----+----+----+------------------------
26| $d01a |  - |  - |  - |  - | ELP|EMMC|EMBC|ERST| Interrupt enabled
--+-------+----+----+----+----+----+----+----+----+------------------------
27| $d01b |M7DP|M6DP|M5DP|M4DP|M3DP|M2DP|M1DP|M0DP| Sprite data priority
--+-------+----+----+----+----+----+----+----+----+------------------------
28| $d01c |M7MC|M6MC|M5MC|M4MC|M3MC|M2MC|M1MC|M0MC| Sprite multicolor
--+-------+----+----+----+----+----+----+----+----+------------------------
29| $d01d |M7XE|M6XE|M5XE|M4XE|M3XE|M2XE|M1XE|M0XE| Sprite X expansion
--+-------+----+----+----+----+----+----+----+----+------------------------
30| $d01e | M7M| M6M| M5M| M4M| M3M| M2M| M1M| M0M| Sprite-sprite collision
--+-------+----+----+----+----+----+----+----+----+------------------------
31| $d01f | M7D| M6D| M5D| M4D| M3D| M2D| M1D| M0D| Sprite-data collision
--+-------+----+----+----+----+----+----+----+----+------------------------
32| $d020 |  - |  - |  - |  - |         EC        | Border color
--+-------+----+----+----+----+-------------------+------------------------
33| $d021 |  - |  - |  - |  - |        B0C        | Background color 0
--+-------+----+----+----+----+-------------------+------------------------
34| $d022 |  - |  - |  - |  - |        B1C        | Background color 1
--+-------+----+----+----+----+-------------------+------------------------
35| $d023 |  - |  - |  - |  - |        B2C        | Background color 2
--+-------+----+----+----+----+-------------------+------------------------
36| $d024 |  - |  - |  - |  - |        B3C        | Background color 3
--+-------+----+----+----+----+-------------------+------------------------
37| $d025 |  - |  - |  - |  - |        MM0        | Sprite multicolor 0
--+-------+----+----+----+----+-------------------+------------------------
38| $d026 |  - |  - |  - |  - |        MM1        | Sprite multicolor 1
--+-------+----+----+----+----+-------------------+------------------------
39| $d027 |  - |  - |  - |  - |        M0C        | Color sprite 0
--+-------+----+----+----+----+-------------------+------------------------
40| $d028 |  - |  - |  - |  - |        M1C        | Color sprite 1
--+-------+----+----+----+----+-------------------+------------------------
41| $d029 |  - |  - |  - |  - |        M2C        | Color sprite 2
--+-------+----+----+----+----+-------------------+------------------------
42| $d02a |  - |  - |  - |  - |        M3C        | Color sprite 3
--+-------+----+----+----+----+-------------------+------------------------
43| $d02b |  - |  - |  - |  - |        M4C        | Color sprite 4
--+-------+----+----+----+----+-------------------+------------------------
44| $d02c |  - |  - |  - |  - |        M5C        | Color sprite 5
--+-------+----+----+----+----+-------------------+------------------------
45| $d02d |  - |  - |  - |  - |        M6C        | Color sprite 6
--+-------+----+----+----+----+-------------------+------------------------
46| $d02e |  - |  - |  - |  - |        M7C        | Color sprite 7
--+-------+----+----+----+----+-------------------+------------------------

Notes:

 � The bits marked with '-' are not connected and give "1" on reading
 � The VIC registers are repeated each 64 bytes in the area $d000-$d3ff,
   i.e. register 0 appears on addresses $d000, $d040, $d080 etc.
 � The unused addresses $d02f-$d03f give $ff on reading, a write access is
   ignored
 � The registers $d01e and $d01f cannot be written and are automatically
   cleared on reading
 � The RES bit (bit 5) of register $d016 has no function on the VIC
   6567/6569 examined as yet. On the 6566, this bit is used to stop the
   VIC.
 � Bit 7 in register $d011 (RST8) is bit 8 of register $d012. Together they
   are called "RASTER" in the following. A write access to these bits sets
   the comparison line for the raster interrupt (see section 3.12.).

*/

#define REG_X_COOR_SPRITE_0         0xD000
#define REG_Y_COOR_SPRITE_0         0xD001
#define REG_X_COOR_SPRITE_1         0xD002
#define REG_Y_COOR_SPRITE_1         0xD003
#define REG_X_COOR_SPRITE_2         0xD004
#define REG_Y_COOR_SPRITE_2         0xD005
#define REG_X_COOR_SPRITE_3         0xD006
#define REG_Y_COOR_SPRITE_3         0xD007
#define REG_X_COOR_SPRITE_4         0xD008
#define REG_Y_COOR_SPRITE_4         0xD009
#define REG_X_COOR_SPRITE_5         0xD00A
#define REG_Y_COOR_SPRITE_5         0xD00B
#define REG_X_COOR_SPRITE_6         0xD00C
#define REG_Y_COOR_SPRITE_6         0xD00D
#define REG_X_COOR_SPRITE_7         0xD00E
#define REG_Y_COOR_SPRITE_7         0xD00F

#define REG_MSB_X_COOR              0xD010
#define MASK_MSB_X_COOR_0           0x01
#define MASK_MSB_X_COOR_1           0x02
#define MASK_MSB_X_COOR_2           0x04
#define MASK_MSB_X_COOR_3           0x08
#define MASK_MSB_X_COOR_4           0x10
#define MASK_MSB_X_COOR_5           0x20
#define MASK_MSB_X_COOR_6           0x40
#define MASK_MSB_X_COOR_7           0x80

#define REG_CR_1                    0xD011
#define MASK_CR_1_RST8              0x80
#define MASK_CR_1_ECM               0x40
#define MASK_CR_1_BMM               0x20
#define MASK_CR_1_DEN               0x10
#define MASK_CR_1_RSEL              0x08
#define MASK_CR_1_YSCROLL           0x07

#define REG_RASTER                  0xD012

#define REG_LIGHT_PEN_X             0xD013

#define REG_LIGHT_PEN_Y             0xD014

#define REG_SPRITE_ENABLED          0xD015
#define MASK_SPRITE_ENABLED_0       0x01
#define MASK_SPRITE_ENABLED_1       0x02
#define MASK_SPRITE_ENABLED_2       0x04
#define MASK_SPRITE_ENABLED_3       0x08
#define MASK_SPRITE_ENABLED_4       0x10
#define MASK_SPRITE_ENABLED_5       0x20
#define MASK_SPRITE_ENABLED_6       0x40
#define MASK_SPRITE_ENABLED_7       0x80

#define REG_CR_2                    0xD016
#define MASK_CR_2_RES               0x20
#define MASK_CR_2_MCM               0x10
#define MASK_CR_2_CSEL              0x08
#define MASK_CR_2_XSCROLL           0x07

#define REG_SPRITE_Y_EXP            0xD017
#define MASK_SPRITE_Y_EXP_0         0x01
#define MASK_SPRITE_Y_EXP_1         0x02
#define MASK_SPRITE_Y_EXP_2         0x04
#define MASK_SPRITE_Y_EXP_3         0x08
#define MASK_SPRITE_Y_EXP_4         0x10
#define MASK_SPRITE_Y_EXP_5         0x20
#define MASK_SPRITE_Y_EXP_6         0x40
#define MASK_SPRITE_Y_EXP_7         0x80

#define REG_MEM_POINTERS            0xD018
#define MASK_MEM_POINTERS_VM13      0x80
#define MASK_MEM_POINTERS_VM12      0x40
#define MASK_MEM_POINTERS_VM11      0x20
#define MASK_MEM_POINTERS_VM10      0x10
#define MASK_MEM_POINTERS_CB13      0x08
#define MASK_MEM_POINTERS_CB12      0x04
#define MASK_MEM_POINTERS_CB11      0x02

#define REG_INTRRUPT                0xD019
#define MASK_INTRRUPT_IRQ           0x80
#define MASK_INTRRUPT_ILP           0x08
#define MASK_INTRRUPT_IMMC          0x04
#define MASK_INTRRUPT_IMBC          0x02
#define MASK_INTRRUPT_IRST          0x01

#define REG_INTRRUPT_ENABLE         0xD01A
#define MASK_INTRRUPT_ENABLE_LP     0x08
#define MASK_INTRRUPT_ENABLE_MCM    0x04
#define MASK_INTRRUPT_ENABLE_MBC    0x02
#define MASK_INTRRUPT_ENABLE_ERST   0x01

#define REG_SPRITE_DATA_PRIO        0xD01B
#define MASK_SPRITE_DATA_PRIO_0     0x01
#define MASK_SPRITE_DATA_PRIO_1     0x02
#define MASK_SPRITE_DATA_PRIO_2     0x04
#define MASK_SPRITE_DATA_PRIO_3     0x08
#define MASK_SPRITE_DATA_PRIO_4     0x10
#define MASK_SPRITE_DATA_PRIO_5     0x20
#define MASK_SPRITE_DATA_PRIO_6     0x40
#define MASK_SPRITE_DATA_PRIO_7     0x80

#define REG_SPRITE_MULTICOLOR       0xD01C
#define MASK_SPRITE_MCOLOR_0        0x01
#define MASK_SPRITE_MCOLOR_1        0x02
#define MASK_SPRITE_MCOLOR_2        0x04
#define MASK_SPRITE_MCOLOR_3        0x08
#define MASK_SPRITE_MCOLOR_4        0x10
#define MASK_SPRITE_MCOLOR_5        0x20
#define MASK_SPRITE_MCOLOR_6        0x40
#define MASK_SPRITE_MCOLOR_7        0x80

#define REG_SPRITE_X_EXP            0xD01D
#define MASK_SPRITE_X_EXP_0         0x01
#define MASK_SPRITE_X_EXP_1         0x02
#define MASK_SPRITE_X_EXP_2         0x04
#define MASK_SPRITE_X_EXP_3         0x08
#define MASK_SPRITE_X_EXP_4         0x10
#define MASK_SPRITE_X_EXP_5         0x20
#define MASK_SPRITE_X_EXP_6         0x40
#define MASK_SPRITE_X_EXP_7         0x80

#define REG_SS_COLL                 0xD01E

#define REG_SG_COLL                 0xD01F

#define REG_COLOR_BORDER            0xD020
#define MASK_COLOR_BORDER_EC        0x0F

#define REG_COLOR_BG_0              0xD021
#define MASK_COLOR_BG_B0C           0x0F

#define REG_COLOR_BG_1              0xD022
#define MASK_COLOR_BG_B1C           0x0F

#define REG_COLOR_BG_2              0xD023
#define MASK_COLOR_BG_B2C           0x0F

#define REG_COLOR_BG_3              0xD024
#define MASK_COLOR_BG_B3C           0x0F

#define REG_SPRITE_MCOLOR_0         0xD025
#define MASK_SPRITE_MCOLOR_MM0      0x0F

#define REG_SPRITE_MCOLOR_1         0xD026
#define MASK_SPRITE_MCOLOR_MM1      0x0F

#define REG_COLOR_SPRITE_0          0xD027
#define MASK_COLOR_SPRITE_0_M0C     0x0F

#define REG_COLOR_SPRITE_1          0xD028
#define MASK_COLOR_SPRITE_1_M1C     0x0F

#define REG_COLOR_SPRITE_2          0xD029
#define MASK_COLOR_SPRITE_2_M2C     0x0F

#define REG_COLOR_SPRITE_3          0xD02A
#define MASK_COLOR_SPRITE_3_M3C     0x0F

#define REG_COLOR_SPRITE_4          0xD02B
#define MASK_COLOR_SPRITE_4_M4C     0x0F

#define REG_COLOR_SPRITE_5          0xD02C
#define MASK_COLOR_SPRITE_5_M5C     0x0F

#define REG_COLOR_SPRITE_6          0xD02D
#define MASK_COLOR_SPRITE_6_M6C     0x0F

#define REG_COLOR_SPRITE_7          0xD02E
#define MASK_COLOR_SPRITE_7_M7C     0x0F

#define MASK_COLOR_MTM              0x07
#define MASK_MC_FLAG                0x08

#define MASK_SPRITE_7               0x80
#define MASK_SPRITE_6               0x40
#define MASK_SPRITE_5               0x20
#define MASK_SPRITE_4               0x10
#define MASK_SPRITE_3               0x08
#define MASK_SPRITE_2               0x04
#define MASK_SPRITE_1               0x02
#define MASK_SPRITE_0               0x01

/* GRAPHIC MODES */

#define GRAPHIC_MODE_STM            0x00
#define GRAPHIC_MODE_MTM            0x01
#define GRAPHIC_MODE_SBM            0x02
#define GRAPHIC_MODE_MBM            0x03
#define GRAPHIC_MODE_ECM            0x04
#define GRAPHIC_MODE_ITM            0x05
#define GRAPHIC_MODE_IBM_1          0x06
#define GRAPHIC_MODE_IBM_2          0x07
#define GRAPHIC_MODE_BORDER_EXT     0x08

typedef enum
{
    VIC_MEM_SPRITE_BACKGROUND,
    VIC_MEM_SPRITE_FORGROUND,
    VIC_MEM_SPRITE_MAP,
    VIC_MEM_MAX
} vic_mem_sprite_t;

void vic_set_layer(uint8_t *layer_addr_p);
void vic_set_memory(uint8_t *mem_addr_p, vic_mem_sprite_t vic_mem_sprite);
void vic_set_bank(uint8_t value);
void vic_init();
void vic_step(uint32_t cc);
void vic_set_half_frame_rate();
void vic_set_full_frame_rate();
void vic_unlock_frame_rate();
void vic_lock_frame_rate();

#endif
