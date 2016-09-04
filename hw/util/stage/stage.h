/*
 * memwa2 stage utility
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


#ifndef _STAGE_H
#define _STAGE_H

#include "stm32f7xx_hal.h"
#include "main.h"

typedef enum
{
  FILE_LIST_TYPE_CASETTE,
  FILE_LIST_TYPE_FLOPPY,
  FILE_LIST_TYPE_T64,
  FILE_LIST_TYPE_PRG,
  FILE_LIST_TYPE_MAX
} file_list_type_t;

typedef enum
{
    STAGE_EMULATION,
    STAGE_MENU,
    STAGE_FILE_SELECT,
    STAGE_COLOR_SCREEN
} stage_t;

typedef enum
{
    INFO_FPS,
    INFO_DISK,
    INFO_DISK_LED,
    INFO_FREQLOCK,
    INFO_FRAMERATE,
    INFO_TAPE_BUTTON,
    INFO_TAPE_MOTOR,
    INFO_PRINT
} info_t;

void stage_init(uint32_t memory);
void stage_prepare(stage_t stage);
void stage_select_layer(uint8_t layer);
void stage_clear_last_messsage();
void stage_draw_string(char *string_p, uint32_t xpos, uint32_t ypos);
void stage_draw_char(char character, uint32_t xpos, uint32_t ypos);
void stage_draw_selection(stage_t stage);
void stage_draw_fw();
void stage_input_key(stage_t stage, char keybd_key);
char *stage_get_selected_filename();
char *stage_get_selected_path();
file_list_type_t stage_get_selected_file_list_type();
void stage_draw_info(info_t info, uint8_t value);
void stage_set_message(char *string_p);

#endif
