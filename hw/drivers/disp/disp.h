/*
 * memwa2 display driver
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


#ifndef _DISP_H
#define _DISP_H

#include "stm32f7xx_hal.h"
#include "main.h"

#define MEM_ADDR_BUFFER0    0
#define MEM_ADDR_BUFFER1    1
#define MEM_ADDR_BUFFER2    2

typedef enum
{
    DISP_MODE_VGA,
    DISP_MODE_SVGA
} disp_mode_t;

typedef enum
{
    DISP_COLOR_BLACK,
    DISP_COLOR_WHITE,
    DISP_COLOR_RED,
    DISP_COLOR_CYAN,
    DISP_COLOR_VIOLET,
    DISP_COLOR_GREEN,
    DISP_COLOR_BLUE,
    DISP_COLOR_YELLOW,
    DISP_COLOR_ORANGE,
    DISP_COLOR_BROWN,
    DISP_COLOR_LRED,
    DISP_COLOR_GREY1,
    DISP_COLOR_GREY2,
    DISP_COLOR_LGREEN,
    DISP_COLOR_LBLUE,
    DISP_COLOR_GREY3,
    DISP_COLOR_TEXT_BG,
    DISP_COLOR_TEXT_FG,
    DISP_COLOR_MARKER,
} disp_color_t;

void disp_init(disp_mode_t disp_mode);
void *disp_get_layer(uint8_t layer);
void disp_set_memory(uint8_t layer, uint32_t memory);
void disp_set_layer(uint8_t layer,
                    uint32_t window_x0,
                    uint32_t window_x1,
                    uint32_t window_y0,
                    uint32_t window_y1,
                    uint32_t buffer_x,
                    uint32_t buffer_y,
                    uint8_t alpha,
                    uint32_t pixel_format);
void disp_activate_layer(uint8_t layer);
void disp_deactivate_layer(uint8_t layer);
void disp_fill_layer(uint8_t layer, uint32_t color);
void disp_move_layer(uint8_t layer, uint32_t x, uint32_t y);
void disp_flip_buffer(uint8_t **done_buffer_pp);
void disp_set_clut_table(uint32_t *clut_p);
void disp_enable_clut(uint8_t layer);
void disp_disable_clut(uint8_t layer);

#endif
