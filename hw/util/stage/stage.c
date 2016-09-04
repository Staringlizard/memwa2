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


/**
 * Handles all different stages and is responsible for
 * the graphic being displayed on the screen.
 */


#include "stage.h"
#include "disp.h"
#include "romcc.h"
#include "ff.h"
#include "if.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BITMAP_WIDTH    192
#define BITMAP_HEIGHT   128

#define SCREEN_WIDTH            800
#define SCREEN_HEIGHT           600
#define BACKGROUND_WIDTH        SCREEN_WIDTH
#define BACKGROUND_HEIGHT       SCREEN_HEIGHT
#define UPPER_BORDER            32
#define LOWER_BORDER            50
#define LEFT_BORDER             44
#define RIGHT_BORDER            36
#define FORGROUND_WIDTH         (320 + LEFT_BORDER + RIGHT_BORDER)
#define FORGROUND_HEIGHT        (200 + UPPER_BORDER + LOWER_BORDER)
#define FILENAME_HIGHT          10
#define FILES_IN_COLUMN         (SCREEN_HEIGHT/FILENAME_HIGHT)
#define FILENAME_LENGTH_PIXELS  (20*8)
#define FILE_COLUMNS            (SCREEN_WIDTH/FILENAME_LENGTH_PIXELS)

#define STRING_INFO_FPS               "EFPS: "
#define STRING_INFO_DISK_ACTIVE       "DD.ACTIVE"
#define STRING_INFO_DISK_INACTIVE     "DD.INACTIVE"
#define STRING_INFO_FREQLOCK_ACTIVE   "FREQ.LOCK"
#define STRING_INFO_FREQLOCK_INACTIVE "FREQ.UNLOCK"
#define STRING_INFO_FRAMERATE_HALF    "FRATE.HALF"
#define STRING_INFO_FRAMERATE_FULL    "FRATE.FULL"
#define STRING_INFO_TAPE_PLAY         "TAPE.PLAY"
#define STRING_INFO_TAPE_STOP         "TAPE.STOP"
#define STRING_INFO_TAPE_ON           "TAPE.ON"
#define STRING_INFO_TAPE_OFF          "TAPE.OFF"
 
#define STRING_INFO_PRINT             "MSG: "
#define CHAR_INFO_DISK_LED_ACTIVE     0xB8
#define CHAR_INFO_DISK_LED_INACTIVE   0xA9

#define XPOS_FW_STRING              (100*8)
#define YPOS_FW_STRING              0

#define XPOS_INFO_FPS               0
#define XPOS_INFO_DISK_ACTIVE       13*8
#define XPOS_INFO_DISK_INACTIVE     13*8
#define XPOS_INFO_DISK_LED_ACTIVE   (13*8+10*8)
#define XPOS_INFO_DISK_LED_INACTIVE (13*8+10*8)
#define XPOS_INFO_FREQLOCK_ACTIVE   13*8*2
#define XPOS_INFO_FREQLOCK_INACTIVE 13*8*2
#define XPOS_INFO_FRAMERATE_HALF    13*8*3
#define XPOS_INFO_FRAMERATE_FULL    13*8*3
#define XPOS_INFO_TAPE_PLAY         13*8*4
#define XPOS_INFO_TAPE_STOP         13*8*4
#define XPOS_INFO_TAPE_ON           13*8*5
#define XPOS_INFO_TAPE_OFF          13*8*5
#define XPOS_INFO_PRINT             0

#define YPOS_INFO_FPS               0
#define YPOS_INFO_DISK_ACTIVE       0
#define YPOS_INFO_DISK_INACTIVE     0
#define YPOS_INFO_DISK_LED_ACTIVE   0
#define YPOS_INFO_DISK_LED_INACTIVE 0
#define YPOS_INFO_FREQLOCK_ACTIVE   0
#define YPOS_INFO_FREQLOCK_INACTIVE 0
#define YPOS_INFO_FRAMERATE_HALF    0
#define YPOS_INFO_FRAMERATE_FULL    0
#define YPOS_INFO_TAPE_PLAY         0
#define YPOS_INFO_TAPE_STOP         0
#define YPOS_INFO_TAPE_ON           0
#define YPOS_INFO_TAPE_OFF          0
#define YPOS_INFO_PRINT             8

typedef enum
{
  ICON_CASETTE,
  ICON_FLOPPY,
  ICON_T64,
  ICON_PRG,
  ICON_MAX
} icon_t;

#pragma pack(push, 1)

typedef struct
{
    uint16_t type; /* File type */
    uint32_t size; /* Size in bytes of the bitmap file */
    uint16_t reserved1; /* Must be 0 */
    uint16_t reserved2;  /* Must be 0 */
    uint32_t offset;  /* Offset in bytes to the bitmap bits */
} bitmap_file_header_t;

typedef struct
{
    uint32_t size; /* Size of structure */
    uint32_t width; /* Width in pixels */
    uint32_t height; /* Height in pixels */
    uint16_t planes; /* Must be 1 */
    uint16_t bit_cnt; /* Bits per pixel */
    uint32_t compression; /* Type of compression */
    uint32_t size_image;  /* Size of image in bytes */
    uint32_t pixels_per_meter_X; /* Pixels per meter in x axis */
    uint32_t pixels_per_meter_Y; /* Pixels per meter in y axis */
    uint32_t colors_used; /* Colors used by the bitmap */
    uint32_t colors_important; /* Colors that are important */
} bitmap_info_header_t;

#pragma pack(pop)

typedef struct
{
    bitmap_file_header_t bitmap_file_header;
    bitmap_info_header_t bitmap_info_header;
    uint8_t *data_p;
    FIL *fd_p;
} bitmap_t;

typedef struct
{
    uint32_t x1;
    uint32_t x2;
    uint32_t y1;
    uint32_t y2;
    uint32_t color;
    uint8_t alpha;
} icons_t;

typedef struct
{
    char filename_a[20];
    char path_a[10];
    file_list_type_t file_list_type;
    uint8_t loaded;
    uint8_t exist;
} file_t;

static file_list_type_t g_file_list_type_loaded;
static icons_t g_icons_a[ICON_MAX];
static icon_t g_current_icon;
static file_t *g_file_list_p; /* Scanned filenames can be put here */

static uint8_t *g_drawing_area_p;
static uint8_t *g_crom_p;
static uint32_t g_file_list_cnt;
static uint32_t g_current_file;
static uint32_t g_current_page;
static char *g_last_message;

static char *g_icon_path_ap[] =
{
  "0:/res/tap.bmp",
  "0:/res/d64.bmp",
  "0:/res/t64.bmp",
  "0:/res/prg.bmp"
};

static char *g_dir_path_ap[] =
{
  "0:/tap",
  "0:/d64",
  "0:/t64",
  "0:/prg"
};

static void unscan_files()
{
    uint32_t i;

    for(i = 0; i < g_file_list_cnt; i++)
    {
        memset(g_file_list_p + i, 0x00, sizeof(file_t));
    }

    g_file_list_cnt = 0;
    g_current_file = 0;
    g_current_page = 0;
}

static FRESULT scan_files(file_list_type_t file_list_type, char *path_p, uint32_t *items_p)
{
    FRESULT res;
    DIR dir;
    static FILINFO fno; /* There is something fishy about this variable, if not static then bad things can happen ... */

    if(file_list_type == g_file_list_type_loaded)
    {
        /* No need to reload it */
        return FR_OK;
    }
    else
    {
        unscan_files();
    }

    res = f_opendir(&dir, path_p);
    if(res == FR_OK)
    {
        for(;;)
        {
            res = f_readdir(&dir, &fno);
            if(res != FR_OK || fno.fname[0] == 0)
            {
                break;
            }
            else
            {
                memcpy(g_file_list_p[g_file_list_cnt].filename_a, fno.fname, 20);
                memcpy(g_file_list_p[g_file_list_cnt].path_a, path_p, 10);
                g_file_list_p[g_file_list_cnt].file_list_type = file_list_type;
                g_file_list_p[g_file_list_cnt].exist = 1;
                g_file_list_p[g_file_list_cnt].loaded = 0;
                (*items_p)++;
            }
        }
        f_closedir(&dir);
    }

    if(res == FR_OK)
    {
        g_file_list_type_loaded = file_list_type;
    }

    return res;
}

static void draw_char(char character, uint32_t xpos, uint32_t ypos)
{
  uint32_t pixel = 0;
  uint32_t row = 0;
  uint8_t temp_color = 0;
  uint8_t bit_counter = 0;
  uint8_t bitmap = 0;

  for(row = 0; row < 8; row++)
  {
    if(character >= 0x20 && character <= 0x2F) /* ! " # % etc. */
    {
      bitmap = *(g_crom_p + character*8 + row);
    }
    else if(character >= 0x30 && character <= 0x3F) /* 0-9 and : ; ( = ) ? */
    {
      bitmap = *(g_crom_p + (character)*8 + row);
    }
    else if(character >= 0x41 && character <= 0x5A) /* Capital letters */
    {
      bitmap = *(g_crom_p + (character - 0x40)*8 + row);
    }
    else if(character >= 0x61 && character <= 0x7A)/* Lowercase letters */
    {
      bitmap = *(g_crom_p + (character - 0x60)*8 + row + 0x800);
    }
    else if(character == '~')
    {
      bitmap = *(g_crom_p + 0x68*8 + row);
    }
    else if(character == 0xB8) /* led ON */
    {
      bitmap = *(g_crom_p + 0x51*8 + row);
    }
    else if(character == 0xA9) /* led OFF */
    {
      bitmap = *(g_crom_p + 0x60*8 + row);
    }
    else if(character == 0x5B) /* [ */
    {
      bitmap = *(g_crom_p + 0x1B*8 + row);
    }
    else if(character == 0x5D) /* ] */
    {
      bitmap = *(g_crom_p + 0x1D*8 + row);
    }    

    bit_counter = 0;
    for(pixel = 0; pixel < 8; pixel++)
    {
      if((bitmap >> (7 - bit_counter)) & 0x1)
      {
        temp_color = DISP_COLOR_TEXT_FG;
      }
      else
      {
        temp_color = DISP_COLOR_TEXT_BG;
      }
      g_drawing_area_p[(ypos + row) * SCREEN_WIDTH + (xpos + pixel)] = temp_color;
      bit_counter++;
    }
  }
}

static void clear_char(uint32_t xpos, uint32_t ypos)
{
  uint32_t pixel = 0;
  uint32_t row = 0;
  uint8_t bit_counter = 0;

  for(row = 0; row < 8; row++)
  {
    for(pixel = 0; pixel < 8; pixel++)
    {
      g_drawing_area_p[(ypos + row) * SCREEN_WIDTH + (xpos + pixel)] = 0x0000;
      bit_counter++;
    }
  }
}

static void clear_string(uint8_t length, uint32_t xpos, uint32_t ypos)
{
    uint32_t i;
    for(i = 0; i < length; i++) /* Just clear 10 characters for now */
    {
        clear_char(xpos + i*8, ypos);
    }
}

static void draw_string(char *string_p, uint32_t xpos, uint32_t ypos)
{
    uint32_t i;

    for(i = 0; i < strlen(string_p); i++)
    {
        draw_char(string_p[i], xpos + i*8, ypos);
    }
}

static void draw_filenames()
{
    uint32_t x;
    uint32_t y;
    for(x = 0; x < SCREEN_WIDTH/FILENAME_LENGTH_PIXELS; x++)
    {
        for(y = 0; y < SCREEN_HEIGHT/FILENAME_HIGHT; y++)
        {
            draw_string(g_file_list_p[x*SCREEN_HEIGHT/FILENAME_HIGHT + y + g_current_page*FILES_IN_COLUMN*FILE_COLUMNS].filename_a, x * FILENAME_LENGTH_PIXELS, 10 * y);
        }
    }
}

static uint8_t more_files_exist()
{
    return g_file_list_p[(g_current_page + 1)*FILES_IN_COLUMN*FILE_COLUMNS].exist;
}

static bitmap_t *load_bitmap(icon_t icon)
{
    FRESULT res;
    uint32_t bytes_read;
    bitmap_t *bitmap_p;

    bitmap_p = malloc(sizeof(bitmap_t));
    bitmap_p->fd_p = malloc(sizeof(FIL));

    res = f_open(bitmap_p->fd_p, g_icon_path_ap[icon], FA_READ);

    if(res != FR_OK)
    {
        main_error("Cannot open bitmap file!", __FILE__, __LINE__, 0);
    }

    /* Get file header */
    f_read(bitmap_p->fd_p, &bitmap_p->bitmap_file_header, sizeof(bitmap_file_header_t), (UINT *)&bytes_read);

    if(bytes_read != sizeof(bitmap_file_header_t))
    {
        main_error("Cannot read bitmap file!", __FILE__, __LINE__, 0);
    }

    /* Check so that file is bitmap */
    if(bitmap_p->bitmap_file_header.type !=0x4D42)
    {
        main_error("Only bitmap files are supported!", __FILE__, __LINE__, 0);
    }

    /* Get the info header */
    f_read(bitmap_p->fd_p, &bitmap_p->bitmap_info_header, sizeof(bitmap_info_header_t), (UINT *)&bytes_read);

    if(bytes_read != sizeof(bitmap_info_header_t))
    {
        main_error("Cannot read bitmap file!", __FILE__, __LINE__, 0);
    }

    /* Go to bitmap data */
    f_lseek(bitmap_p->fd_p, bitmap_p->bitmap_file_header.offset);

    bitmap_p->data_p = (uint8_t *)malloc(bitmap_p->bitmap_info_header.size_image);

    if(bitmap_p->data_p == NULL)
    {
        main_error("Malloc problems!", __FILE__, __LINE__, 0);
    }

    /* Get bitmap data */
    f_read(bitmap_p->fd_p, bitmap_p->data_p, bitmap_p->bitmap_info_header.size_image, (UINT *)&bytes_read);

    if(bytes_read != bitmap_p->bitmap_info_header.size_image)
    {
        main_error("Cannot read bitmap file!", __FILE__, __LINE__, 0);
    }

    return bitmap_p;
}

static void free_bitmap(bitmap_t *bitmap_p)
{
    f_close(bitmap_p->fd_p);
    free(bitmap_p->data_p);
    free(bitmap_p->fd_p);
    free(bitmap_p);
}

static void draw_icons()
{
    uint32_t read_cnt = 0;
    uint32_t i;
    uint32_t y;
    uint32_t x;
    bitmap_t *bitmap_p;

    for(i = 0; i < ICON_MAX; i++)
    {
        bitmap_p = load_bitmap((icon_t)i);

        read_cnt = bitmap_p->bitmap_info_header.size_image;

        for(y = 0; y < bitmap_p->bitmap_info_header.height; y++)
        {
            for(x = 0; x < bitmap_p->bitmap_info_header.width; x++)
            {
                uint32_t color = 0;

                color = (bitmap_p->data_p[read_cnt--] >> 3) << 11;
                color |= (bitmap_p->data_p[read_cnt--] >> 2) << 5;
                color |= (bitmap_p->data_p[read_cnt--] >> 3);

                ((uint16_t *)g_drawing_area_p)[(y + g_icons_a[i].y1) * SCREEN_WIDTH + bitmap_p->bitmap_info_header.width - x + g_icons_a[i].x1] = color;
            }
        }

        free_bitmap(bitmap_p);
    }
}

static void prepare_emulation()
{
    disp_fill_layer(0, 0);
    disp_fill_layer(1, 0);
    disp_deactivate_layer(0);
    disp_deactivate_layer(1);

#ifdef SCREEN_X2
    disp_set_layer(1,
        (SCREEN_WIDTH-FORGROUND_WIDTH*2)/2,
        (SCREEN_WIDTH-FORGROUND_WIDTH*2)/2 + FORGROUND_WIDTH*2,
        (SCREEN_HEIGHT-FORGROUND_HEIGHT*2)/2,
        (SCREEN_HEIGHT-FORGROUND_HEIGHT*2)/2 + FORGROUND_HEIGHT*2,
        FORGROUND_WIDTH*2,
        FORGROUND_HEIGHT*2,
        255,
        LTDC_PIXEL_FORMAT_L8);
#else
    disp_set_layer(1,
        (SCREEN_WIDTH-FORGROUND_WIDTH)/2,
        (SCREEN_WIDTH-FORGROUND_WIDTH)/2 + FORGROUND_WIDTH,
        (SCREEN_HEIGHT-FORGROUND_HEIGHT)/2,
        (SCREEN_HEIGHT-FORGROUND_HEIGHT)/2 + FORGROUND_HEIGHT,
        FORGROUND_WIDTH,
        FORGROUND_HEIGHT,
        255,
        LTDC_PIXEL_FORMAT_L8);
#endif

    disp_enable_clut(0);
    disp_enable_clut(1);
    disp_activate_layer(1);
}

static void prepare_limit_bg()
{
    disp_deactivate_layer(0);
    disp_deactivate_layer(1);

    /* Careful here so that the bus does not overload */
    disp_set_layer(0,
        0,
        BACKGROUND_WIDTH,
        0,
        16, /* Bus overloads if larger window? */
        BACKGROUND_WIDTH,
        BACKGROUND_HEIGHT,
        255,
        LTDC_PIXEL_FORMAT_L8);

    disp_enable_clut(0);
    disp_enable_clut(1);
}

static void prepare_full_bg()
{
    disp_fill_layer(0, 0);
    disp_fill_layer(1, 0);
    disp_deactivate_layer(0);

    disp_set_layer(0,
        0,
        BACKGROUND_WIDTH,
        0,
        BACKGROUND_HEIGHT,
        BACKGROUND_WIDTH,
        BACKGROUND_HEIGHT,
        255,
        LTDC_PIXEL_FORMAT_L8);

    disp_enable_clut(0);
    disp_enable_clut(1);

    disp_activate_layer(0);
}

static void prepare_menu()
{
    disp_fill_layer(0, 0);
    disp_fill_layer(1, 0);
    disp_deactivate_layer(0);
    disp_deactivate_layer(1);

    disp_set_layer(0,
        0,
        BACKGROUND_WIDTH,
        0,
        BACKGROUND_HEIGHT,
        BACKGROUND_WIDTH,
        BACKGROUND_HEIGHT,
        255,
        LTDC_PIXEL_FORMAT_RGB565);

    disp_fill_layer(0, 0);

    disp_set_layer(1, /* This will be for selection */
        g_icons_a[g_current_icon].x1,
        g_icons_a[g_current_icon].x2,
        g_icons_a[g_current_icon].y1,
        g_icons_a[g_current_icon].y2,
        BITMAP_WIDTH,
        BITMAP_HEIGHT,
        g_icons_a[g_current_icon].alpha,
        LTDC_PIXEL_FORMAT_L8);

    disp_disable_clut(0);

    disp_fill_layer(1, DISP_COLOR_MARKER);

    draw_icons();

    disp_activate_layer(0);
    disp_activate_layer(1);
}

static void prepare_file_select()
{
    disp_fill_layer(0, 0);
    disp_fill_layer(1, 0);
    disp_deactivate_layer(1);
    switch(g_current_icon)
    {
        case ICON_CASETTE:
            scan_files(FILE_LIST_TYPE_CASETTE, g_dir_path_ap[g_current_icon], &g_file_list_cnt);
            break;
        case ICON_FLOPPY:
            scan_files(FILE_LIST_TYPE_FLOPPY, g_dir_path_ap[g_current_icon], &g_file_list_cnt);
            break;
        case ICON_T64:
            scan_files(FILE_LIST_TYPE_T64, g_dir_path_ap[g_current_icon], &g_file_list_cnt);
            break;
        case ICON_PRG:
            scan_files(FILE_LIST_TYPE_PRG, g_dir_path_ap[g_current_icon], &g_file_list_cnt);
            break;
        default:
            scan_files(FILE_LIST_TYPE_CASETTE, g_dir_path_ap[g_current_icon], &g_file_list_cnt);
    }
    disp_set_layer(1, /* This will be for selection */
                   (g_current_file/FILES_IN_COLUMN) * FILENAME_LENGTH_PIXELS,
                   (g_current_file/FILES_IN_COLUMN + 1) * FILENAME_LENGTH_PIXELS,
                   (g_current_file) * FILENAME_HIGHT,
                   (g_current_file + 1) * FILENAME_HIGHT,
                   FILENAME_LENGTH_PIXELS,
                   FILENAME_HIGHT,
                   100,
                   LTDC_PIXEL_FORMAT_L8);

    draw_filenames();
    disp_fill_layer(1, DISP_COLOR_MARKER);
    disp_activate_layer(1);
}

static void prepare_color_screen()
{
    uint32_t i;
    uint32_t k;
    uint32_t j;
    uint32_t color;

    disp_deactivate_layer(1);
    disp_fill_layer(0, 0);

    for(i = 0; i < SCREEN_HEIGHT; i++)
    {
        for(k = 0; k < 16; k++)
        {
            color = k;
            for(j = 0; j < SCREEN_WIDTH/16; j++)
            {
                *(g_drawing_area_p + i * SCREEN_WIDTH + k * SCREEN_WIDTH/16 + j) = color;
            }
        }
    }
}
    
void stage_init(uint32_t memory)
{
    uint8_t i;
    uint32_t icon_spacing = (SCREEN_WIDTH - ICON_MAX * BITMAP_WIDTH)/(ICON_MAX + 1);

    g_file_list_p = (file_t *)memory;
    g_file_list_cnt = 0;

    prepare_limit_bg();
    prepare_emulation();

    g_crom_p = rom_cc_get_memory(ROM_CC_SECTION_CROM);
    for(i = 0; i < ICON_MAX; i++)
    {
        g_icons_a[i].x1 = icon_spacing + icon_spacing * i + BITMAP_WIDTH * i;
        g_icons_a[i].x2 = icon_spacing + icon_spacing * i + BITMAP_WIDTH * (i + 1);
        g_icons_a[i].y1 = 100;
        g_icons_a[i].y2 = 100 + BITMAP_HEIGHT;
        g_icons_a[i].color = 11000; /* Not used atm */
        g_icons_a[i].alpha = 200;
    }

    g_current_icon = ICON_CASETTE;
    g_current_file = 0;
    g_current_page = 0;
    g_file_list_type_loaded = FILE_LIST_TYPE_MAX; /* Set to none, so filelist will be reloaded */
}

void stage_prepare(stage_t stage)
{
    switch(stage)
    {
        case STAGE_EMULATION:
            prepare_limit_bg();
            prepare_emulation();
            break;
        case STAGE_MENU:    
            prepare_menu();
            break;
        case STAGE_FILE_SELECT:
            prepare_full_bg();
            prepare_file_select();
            break;
        case STAGE_COLOR_SCREEN:
            prepare_full_bg();
            prepare_color_screen();
            break;
    }
}

void stage_select_layer(uint8_t layer)
{
    g_drawing_area_p = (uint8_t *)disp_get_layer(layer);
}

void stage_draw_selection(stage_t stage)
{
    switch(stage)
    {
        case STAGE_EMULATION:
            ;
            break;
        case STAGE_MENU:
            disp_move_layer(1, g_icons_a[g_current_icon].x1, g_icons_a[g_current_icon].y1);
            break;
        case STAGE_FILE_SELECT:
            disp_move_layer(1, (g_current_file/FILES_IN_COLUMN) * FILENAME_LENGTH_PIXELS, (g_current_file % FILES_IN_COLUMN) * FILENAME_HIGHT);
            break;
        case STAGE_COLOR_SCREEN:
            ;
            break;
    }
}

void stage_input_key(stage_t stage, char keybd_key)
{
    switch(stage)
    {
        case STAGE_EMULATION:
            ;
            break;
        case STAGE_MENU:
            switch(keybd_key)
            {
                case 79: /* Right arrow key */
                    if(g_current_icon < ICON_MAX - 1)
                    {
                        g_current_icon++;
                    }
                    else
                    {
                        g_current_icon = 0;
                    }
                    break;
                case 80: /* Left arrow key */
                    if(g_current_icon > 0)
                    {
                        g_current_icon--;
                    }
                    else
                    {
                        g_current_icon = ICON_MAX - 1;
                    }
                    break;
            }
            break;
        case STAGE_FILE_SELECT:
            switch(keybd_key)
            {
                case 75: /* Pg-up */
                    if(g_current_page > 0)
                    {
                        g_current_page--;
                        disp_fill_layer(0, 0);
                        draw_filenames();
                    }
                    break;
                case 78: /* Pg-dn */
                    if(more_files_exist())
                    {
                        g_current_page++;
                        disp_fill_layer(0, 0);
                        draw_filenames();
                    }
                    break;
                case 79: /* Right arrow key */
                    g_current_file += FILES_IN_COLUMN;
                    if(g_current_file >= FILES_IN_COLUMN * FILE_COLUMNS)
                    {
                        g_current_file -= FILES_IN_COLUMN * FILE_COLUMNS;
                    }
                    break;
                case 80: /* Left arrow key */
                    if(g_current_file < FILES_IN_COLUMN)
                    {
                        g_current_file += FILES_IN_COLUMN * (FILE_COLUMNS - 1);
                    }
                    else
                    {
                        g_current_file -= FILES_IN_COLUMN;
                    }
                    break;
                case 81: /* Down arrow key */
                    g_current_file += 1;
                    if(g_current_file == FILES_IN_COLUMN * FILE_COLUMNS)
                    {
                        g_current_file = 0;
                    }
                    break;
                case 82: /* Up arrow key */
                    if(g_current_file == 0)
                    {
                        g_current_file = FILES_IN_COLUMN * FILE_COLUMNS - 1;
                    }
                    else
                    {
                        g_current_file -= 1;
                    }
                    break;
            }
            break;
        case STAGE_COLOR_SCREEN:
            break;

    }
}

void stage_draw_info(info_t info, uint8_t value)
{
    switch(info)
    {
      case INFO_FPS:
        {
            char buf[10];
            clear_string(10, 0, 0);
            itoa(value, buf, 10);
            draw_string(STRING_INFO_FPS, XPOS_INFO_FPS, YPOS_INFO_FPS);
            draw_string(buf, strlen(STRING_INFO_FPS)*8, YPOS_INFO_FPS);
        }
        break;
      case INFO_DISK:
          if(value)
          {
              clear_string(strlen(STRING_INFO_DISK_INACTIVE),
                           XPOS_INFO_DISK_ACTIVE,
                           YPOS_INFO_DISK_ACTIVE);
              draw_string(STRING_INFO_DISK_ACTIVE,
                          XPOS_INFO_DISK_ACTIVE,
                          YPOS_INFO_DISK_ACTIVE);
          }
          else
          {
              clear_string(strlen(STRING_INFO_DISK_ACTIVE),
                           XPOS_INFO_DISK_INACTIVE,
                           YPOS_INFO_DISK_INACTIVE);
              draw_string(STRING_INFO_DISK_INACTIVE,
                          XPOS_INFO_DISK_INACTIVE,
                          YPOS_INFO_DISK_INACTIVE);
          }
          break;
      case INFO_DISK_LED:
          if(value)
          {
              draw_char(CHAR_INFO_DISK_LED_ACTIVE,
                        XPOS_INFO_DISK_LED_ACTIVE,
                        YPOS_INFO_DISK_LED_ACTIVE);
          }
          else
          {
              draw_char(CHAR_INFO_DISK_LED_INACTIVE,
                        XPOS_INFO_DISK_LED_INACTIVE,
                        YPOS_INFO_DISK_LED_INACTIVE);
          }
          break;
      case INFO_FREQLOCK:
          if(value)
          {
              clear_string(strlen(STRING_INFO_FREQLOCK_INACTIVE),
                           XPOS_INFO_FREQLOCK_ACTIVE,
                           YPOS_INFO_FREQLOCK_ACTIVE);
              draw_string(STRING_INFO_FREQLOCK_ACTIVE,
                          XPOS_INFO_FREQLOCK_ACTIVE,
                          YPOS_INFO_FREQLOCK_ACTIVE);
          }
          else
          {
              clear_string(strlen(STRING_INFO_FREQLOCK_ACTIVE),
                           XPOS_INFO_FREQLOCK_INACTIVE,
                           YPOS_INFO_FREQLOCK_INACTIVE);
              draw_string(STRING_INFO_FREQLOCK_INACTIVE,
                          XPOS_INFO_FREQLOCK_INACTIVE,
                          YPOS_INFO_FREQLOCK_INACTIVE);
          }
          break;
      case INFO_FRAMERATE:
          if(value)
          {
              clear_string(strlen(STRING_INFO_FRAMERATE_FULL),
                           XPOS_INFO_FRAMERATE_HALF,
                           YPOS_INFO_FRAMERATE_HALF);
              draw_string(STRING_INFO_FRAMERATE_HALF,
                          XPOS_INFO_FRAMERATE_HALF,
                          YPOS_INFO_FRAMERATE_HALF);
          }
          else
          {
              clear_string(strlen(STRING_INFO_FRAMERATE_HALF),
                           XPOS_INFO_FRAMERATE_FULL,
                           YPOS_INFO_FRAMERATE_FULL);
              draw_string(STRING_INFO_FRAMERATE_FULL,
                          XPOS_INFO_FRAMERATE_FULL,
                          YPOS_INFO_FRAMERATE_FULL);
          }
          break;
      case INFO_TAPE_BUTTON:
          if(value)
          {
              clear_string(strlen(STRING_INFO_TAPE_STOP),
                           XPOS_INFO_TAPE_PLAY,
                           YPOS_INFO_TAPE_PLAY);
              draw_string(STRING_INFO_TAPE_PLAY,
                          XPOS_INFO_TAPE_PLAY,
                          YPOS_INFO_TAPE_PLAY);
          }
          else
          {
              clear_string(strlen(STRING_INFO_TAPE_PLAY),
                           XPOS_INFO_TAPE_STOP,
                           YPOS_INFO_TAPE_STOP);
              draw_string(STRING_INFO_TAPE_STOP,
                          XPOS_INFO_TAPE_STOP,
                          YPOS_INFO_TAPE_STOP);
          }
          break;
      case INFO_TAPE_MOTOR:
          if(value)
          {
              clear_string(strlen(STRING_INFO_TAPE_OFF),
                           XPOS_INFO_TAPE_ON,
                           YPOS_INFO_TAPE_ON);
              draw_string(STRING_INFO_TAPE_ON,
                          XPOS_INFO_TAPE_ON,
                          YPOS_INFO_TAPE_ON);
          }
          else
          {
              clear_string(strlen(STRING_INFO_TAPE_ON),
                           XPOS_INFO_TAPE_OFF,
                           YPOS_INFO_TAPE_OFF);
              draw_string(STRING_INFO_TAPE_OFF,
                          XPOS_INFO_TAPE_OFF,
                          YPOS_INFO_TAPE_OFF);
          }
          break;
      case INFO_PRINT:
          clear_string(strlen(STRING_INFO_PRINT) + 25,
                       XPOS_INFO_PRINT,
                       YPOS_INFO_PRINT);
          draw_string(STRING_INFO_PRINT,
                      XPOS_INFO_PRINT,
                      YPOS_INFO_PRINT);


        if(g_last_message != NULL)
        {
            draw_string(g_last_message, XPOS_INFO_PRINT + strlen(STRING_INFO_PRINT)*8, YPOS_INFO_PRINT);
        }
        break;
    }
}

void stage_set_message(char *string_p)
{
    if(g_last_message != NULL)
    {
      free(g_last_message);
    }
    g_last_message = calloc(1, strlen(string_p) + 1);
    memcpy(g_last_message, string_p, strlen(string_p));
}

void stage_clear_last_messsage()
{
    if(g_last_message != NULL)
    {
        free(g_last_message);
        g_last_message = NULL;
        clear_string(strlen(STRING_INFO_PRINT) + 25,
                   XPOS_INFO_PRINT,
                   YPOS_INFO_PRINT);
    }
}

void stage_draw_string(char *string_p, uint32_t xpos, uint32_t ypos)
{
    draw_string(string_p, xpos, ypos);
}

void stage_draw_char(char character, uint32_t xpos, uint32_t ypos)
{
    draw_char(character, xpos, ypos);
}

void stage_draw_fw()
{
    draw_string(main_get_fw_revision(), XPOS_FW_STRING - strlen(main_get_fw_revision())*8, YPOS_FW_STRING);
}

char *stage_get_selected_filename()
{
    return &g_file_list_p[g_current_page * FILES_IN_COLUMN * FILE_COLUMNS + g_current_file].filename_a[0];
}

char *stage_get_selected_path()
{
    return &g_file_list_p[g_current_page * FILES_IN_COLUMN * FILE_COLUMNS + g_current_file].path_a[0];
}

file_list_type_t stage_get_selected_file_list_type()
{
    return g_file_list_p[g_current_page * FILES_IN_COLUMN * FILE_COLUMNS + g_current_file].file_list_type;
}
