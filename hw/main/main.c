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


/**
 * Main entry point. Handles main configuration of memory and setup.
 * It Aaso handles error codes and fw revision.
 */

#include "main.h"
#include "rng.h"
#include "config.h"
#include "diag.h"
#include "usbh_core.h"
#include "usbh_hid_keybd.h"
#include "console.h"
#include "keybd.h"
#include "if.h"
#include "romcc.h"
#include "romdd.h"
#include "stage.h"
#include "disp.h"
#include "ff.h"
#include "sm.h"
#include "sdcard.h"

#define BUFFER_SIZE        0x1000
#define DEFAULT_KEY_MAX    71

extern if_emu_cc_t g_if_cc_emu;
extern if_emu_dd_t g_if_dd_emu;

static char *g_rom_path_pp[4] =
{
  "0:/rom/cc_brom.bin",
  "0:/rom/cc_crom.bin",
  "0:/rom/cc_krom.bin",
  "0:/rom/dd_dos.bin"
};

static char *g_conf_path_pp[2] =
{
  "0:/conf/palette.cfg",
  "0:/conf/key.cfg"
};

static uint32_t g_clut_a[16];
static if_keybd_map_t g_keybd_map_a[DEFAULT_KEY_MAX + 1];

static FATFS g_fatfs;
static char g_sd_path_p[] = {"0:/"};

int main()
{
    FIL fd;
    FRESULT res;
    diag_status_t diag_status = DIAG_STATUS_OK;
    uint32_t bytes_read = 0;
    uint32_t read_cnt = 0;
    uint32_t i;

    __set_PRIMASK(0); /* Enable IRQ */

    HAL_Init();

    HAL_Delay(200);

    config_init();

    /* Clean sdram */
    for(i = 0; i < SDRAM_SIZE; i++)
    {
        *(uint8_t *)(SDRAM_ADDR + i) = 0;
    }

    /* Card inserted ? */
    if(sdcard_inserted() == 0)
    {
        /* If not insterted, then run full diagnostic */
        diag_status = diag_run();

        if(diag_status != DIAG_STATUS_OK)
        {
            main_error("Diagnostic did not pass!", __FILE__, __LINE__, (uint32_t)diag_status);
        }

        while(1)
        {
            /* If not, then just try again */
            if(sdcard_inserted() == 1)
            {
                sdcard_init();
                HAL_Delay(1000);
                diag_status = diag_sdcard_run();
                if(diag_status != DIAG_STATUS_OK)
                {
                    main_error("Diagnostic did not pass!", __FILE__, __LINE__, (uint32_t)diag_status);
                }
                break;
            }
        }
    }

    disp_set_memory(MEM_ADDR_BUFFER0, CC_DISP_BUFFER1_ADDR);
    disp_set_memory(MEM_ADDR_BUFFER1, CC_DISP_BUFFER2_ADDR);
    disp_set_memory(MEM_ADDR_BUFFER2, CC_DISP_BUFFER3_ADDR);

    if(f_mount(&g_fatfs, (TCHAR const*)g_sd_path_p, 1) != FR_OK)
    {
        main_error("Failed to mount filesystem!", __FILE__, __LINE__, 0);
    }

    /* Try and load basic rom from sd card */
    res = f_open(&fd, g_rom_path_pp[0], FA_READ);

    if(res == FR_OK)
    {
        read_cnt = 0;
        while(read_cnt != IF_MEMORY_CC_BROM_ACTUAL_SIZE)
        {
            f_read(&fd, (uint8_t *)(CC_BROM_BASE_ADDR + CC_BROM_LOAD_ADDR + read_cnt), BUFFER_SIZE, (UINT *)&bytes_read);
            if(res != FR_OK || bytes_read == 0)
            {
                main_warning("Error reading rom file (brom)!", __FILE__, __LINE__, read_cnt);
                break;
            }
            read_cnt += bytes_read;
        }
        f_close(&fd);
    }
    else
    {
        /* No file found, load the default one */
        memcpy((uint8_t *)(CC_BROM_BASE_ADDR + CC_BROM_LOAD_ADDR), rom_cc_get_memory(ROM_CC_SECTION_BROM), IF_MEMORY_CC_BROM_ACTUAL_SIZE);
    }
 
     /* Try and load character rom from sd card */
    res = f_open(&fd, g_rom_path_pp[1], FA_READ);

    if(res == FR_OK)
    {
        read_cnt = 0;
        while(read_cnt != IF_MEMORY_CC_CROM_ACTUAL_SIZE)
        {
            f_read(&fd, (uint8_t *)(CC_CROM_BASE_ADDR + CC_CROM_LOAD_ADDR + read_cnt), BUFFER_SIZE, (UINT *)&bytes_read);
            if(res != FR_OK || bytes_read == 0)
            {
                main_warning("Error reading rom file (crom)!", __FILE__, __LINE__, read_cnt);
                break;
            }
            read_cnt += bytes_read;
        }
        f_close(&fd);
    }
    else
    {
        /* No file found, load the default one */
        memcpy((uint8_t *)(CC_CROM_BASE_ADDR + CC_CROM_LOAD_ADDR), rom_cc_get_memory(ROM_CC_SECTION_CROM), IF_MEMORY_CC_CROM_ACTUAL_SIZE);
    }

     /* Try and load kernal rom from sd card */
    res = f_open(&fd, g_rom_path_pp[2], FA_READ);

    if(res == FR_OK)
    {
        read_cnt = 0;
        while(read_cnt != IF_MEMORY_CC_KROM_ACTUAL_SIZE)
        {
            f_read(&fd, (uint8_t *)(CC_KROM_BASE_ADDR + CC_KROM_LOAD_ADDR + read_cnt), BUFFER_SIZE, (UINT *)&bytes_read);
            if(res != FR_OK || bytes_read == 0)
            {
                main_warning("Error reading rom file (krom)!", __FILE__, __LINE__, read_cnt);
                break;
            }
            read_cnt += bytes_read;
        }
        f_close(&fd);
    }
    else
    {
        /* No file found, load the default one */
        memcpy((uint8_t *)(CC_KROM_BASE_ADDR + CC_KROM_LOAD_ADDR), rom_cc_get_memory(ROM_CC_SECTION_KROM), IF_MEMORY_CC_KROM_ACTUAL_SIZE);
    }

     /* Try and load dos rom from sd card */
    res = f_open(&fd, g_rom_path_pp[3], FA_READ);

    if(res == FR_OK)
    {
        read_cnt = 0;
        while(read_cnt != IF_MEMORY_DD_DOS_ACTUAL_SIZE)
        {
            res = f_read(&fd, (uint8_t *)(DD_ALL_BASE_ADDR + DD_DOS_LOAD_ADDR + read_cnt), BUFFER_SIZE, (UINT *)&bytes_read);
            if(res != FR_OK || bytes_read == 0)
            {
                main_warning("Error reading rom file (dos)!", __FILE__, __LINE__, read_cnt);
                break;
            }
            read_cnt += bytes_read;
        }
        f_close(&fd);
    }
    else
    {
        /* No file found, load the default one */
        memcpy((uint8_t *)(DD_ALL_BASE_ADDR + DD_DOS_LOAD_ADDR), rom_dd_get_memory(ROM_DD_SECTION_DOS), IF_MEMORY_DD_DOS_ACTUAL_SIZE);
    }

     /* Try and load palette conf from sd card */
    res = f_open(&fd, g_conf_path_pp[0], FA_READ);

    if(res == FR_OK)
    {
        uint8_t palette_string_p[256];
        char *colors_pp[28];
        uint8_t colors_cnt = 0;
        char delimiter_p[2] = "\n";
        uint32_t color;

        memset(colors_pp, 0x00, 16 * sizeof(char *));

        f_read(&fd, (uint8_t *)palette_string_p, BUFFER_SIZE, (UINT *)&bytes_read);

        if(res != FR_OK || bytes_read == 0)
        {
            main_warning("Error reading file (palette)!", __FILE__, __LINE__, 0);
        }

        colors_pp[colors_cnt] = strtok((char *)palette_string_p, delimiter_p);
        colors_cnt++;
        while(colors_pp[colors_cnt-1] != NULL)
        {
            /* Get the color asciihex string */
            colors_pp[colors_cnt] = strtok(NULL, delimiter_p);

            /* Get the 32 bit value */
            color = strtoul(colors_pp[colors_cnt], NULL, 16);

            /* Add to collection */
            g_clut_a[colors_cnt] = color;

            colors_cnt++;
        }

        f_close(&fd);

        disp_set_clut_table(g_clut_a);
    }
    else
    {
        ; /* No file found, the default clut table will be used in disp.c */
    }

     /* Try and load key conf from sd card */
    res = f_open(&fd, g_conf_path_pp[1], FA_READ);

    if(res == FR_OK)
    {
        uint8_t key_string_p[1024]; /* 16 chars max on one row */

        f_read(&fd, (uint8_t *)key_string_p, 1024, (UINT *)&bytes_read);

        if(res != FR_OK || bytes_read == 0)
        {
            main_warning("Error reading file (palette)!", __FILE__, __LINE__, 0);
        }

        keybd_populate_map(key_string_p, g_keybd_map_a);

        f_close(&fd);
    }
    else
    {
        /* No file found, load the default configuration */
        memcpy((uint8_t *)g_keybd_map_a,
               keybd_get_default_map(),
               DEFAULT_KEY_MAX * sizeof(if_keybd_map_t));
    }

    stage_init(CC_STAGE_FILES_ADDR);
    stage_select_layer(0);

    /* Give commodore computer (cc) some memory to work with */
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_RAM_BASE_ADDR, IF_MEM_CC_TYPE_RAM);
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_BROM_BASE_ADDR, IF_MEM_CC_TYPE_BROM);
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_CROM_BASE_ADDR, IF_MEM_CC_TYPE_CROM);
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_KROM_BASE_ADDR, IF_MEM_CC_TYPE_KROM);
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_IO_BASE_ADDR, IF_MEM_CC_TYPE_IO);
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_UTIL1_BASE_ADDR, IF_MEM_CC_TYPE_UTIL1); /* subscription read storage by emu */
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_UTIL2_BASE_ADDR, IF_MEM_CC_TYPE_UTIL2); /* subscription write storage by emu */
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE1_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE1); /* sprite virtual layer (background) by emu */
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE2_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE2); /* sprite virtual layer (forground) by emu */
    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE3_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE3); /* sprite mapping by emu */

    /* Give disk drive (dd) some memory to work with */
    g_if_dd_emu.if_emu_dd_mem.mem_set_fp((uint8_t *)DD_ALL_BASE_ADDR, IF_MEM_DD_TYPE_ALL);
    g_if_dd_emu.if_emu_dd_mem.mem_set_fp((uint8_t *)DD_UTIL1_BASE_ADDR, IF_MEM_DD_TYPE_UTIL1);
    g_if_dd_emu.if_emu_dd_mem.mem_set_fp((uint8_t *)DD_UTIL2_BASE_ADDR, IF_MEM_DD_TYPE_UTIL2);

    g_if_cc_emu.if_emu_cc_display.display_layer_set_fp((uint8_t *)CC_DISP_BUFFER2_ADDR);
    g_if_cc_emu.if_emu_cc_ue.ue_keybd_map_set_fp(g_keybd_map_a);

    sm_init();
    sm_run();
}


void main_error(char *string_p, char *file_p, uint32_t line, uint32_t extra)
{
    printf("[ERR] %s (%lu)\n", string_p, extra);
    stage_set_message(string_p);
    if(sm_get_state() == SM_STATE_EMULATOR && sm_get_disp_stats_flag())
    {
        stage_draw_info(INFO_PRINT, 0   );
    }
    //printf("[%s:%lu] %s\n", file_p, line, string_p);
}

void main_warning(char *string_p, char *file_p, uint32_t line, uint32_t extra)
{
    printf("[ERR] %s (%lu)\n", string_p, extra);
    //printf("[%s:%lu] %s\n", file_p, line, string_p);
}

char *main_get_fw_revision()
{
    return FW_REVISION;
}
