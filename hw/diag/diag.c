/*
 * memwa2 diagnostic
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
 * This file can run diagnistic for many parts of the hw including ltdc,
 * sdram and sdcard.
 */

#include "diag.h"
#include "ff.h"
#include "adv7511.h"
#include "disp.h"

#define TEST_FILE               "testfile"
#define TEST_PATTERN_REPS       1000
#define TEST_ADV7511_WRITE_REG  0x96
#define TEST_SDRAM_START_ADDR   SDRAM_ADDR
#define TEST_SDRAM_SIZE         0x800000
#define TEST_SCREEN_WIDTH       800
#define TEST_SCREEN_HEIGHT      600

static FATFS fatfs;
static FIL fil;
static char sd_path[] = {"0:/"};
static char sd_path_and_file[] = {"0:/"TEST_FILE};
static char test_pattern[] = {"abcdefghijklmnopqrstuvxyz1234567890"};
static char test_pattern_cmp[sizeof(test_pattern)];

typedef enum
{
    DIAG_SDRAM_STATUS_OK,
    DIAG_SDRAM_STATUS_ERROR,
} sdram_status_t;

typedef enum
{
    DIAG_SDCARD_STATUS_OK,
    DIAG_SDCARD_STATUS_ERROR_MOUNT,
    DIAG_SDCARD_STATUS_ERROR_OPEN,
    DIAG_SDCARD_STATUS_ERROR_READ,
    DIAG_SDCARD_STATUS_ERROR_WRITE,
    DIAG_SDCARD_STATUS_ERROR_CORRUPT,
} sdcard_status_t;

typedef enum
{
    DIAG_ADV7511_STATUS_OK,
    DIAG_ADV7511_STATUS_ERROR_READ,
    DIAG_ADV7511_STATUS_ERROR_WRITE,
    DIAG_ADV7511_STATUS_ERROR_INITVAL
} adv7511_status_t;

static sdram_status_t sdram_run()
{
    sdram_status_t sdram_status = DIAG_SDRAM_STATUS_OK;
    uint32_t i;

    /* Write to sdram */
    for(i = 1; i <= TEST_SDRAM_SIZE; i++)
    {
        *(uint8_t *)(TEST_SDRAM_START_ADDR + i -1) = test_pattern[i % 35];
    }

    for(i = 1; i <= TEST_SDRAM_SIZE; i++)
    {
        if(*(uint8_t *)(TEST_SDRAM_START_ADDR + i -1) != test_pattern[i % 35])
        {
            sdram_status = DIAG_SDRAM_STATUS_ERROR;
            goto exit;
        }
    }

    /* Zero the memory */
    for(i = 1; i <= TEST_SDRAM_SIZE; i++)
    {
        *(uint8_t *)(TEST_SDRAM_START_ADDR + i -1) = 0;
    }

exit:
    return sdram_status;
}

static sdcard_status_t sdcard_run()
{
    uint32_t bytes_written;
    uint32_t bytes_read;
    uint32_t i;
    uint32_t j;
    sdcard_status_t sdcard_status = DIAG_SDCARD_STATUS_OK;
    FRESULT fresult = FR_OK;

    if(f_mount(&fatfs, (TCHAR const*)sd_path, 1) != FR_OK)
    {
        sdcard_status = DIAG_SDCARD_STATUS_ERROR_MOUNT;
        goto exit;
    }

    if(f_open(&fil, TEST_FILE, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
    {
        sdcard_status = DIAG_SDCARD_STATUS_ERROR_OPEN;
        goto exit;
    }

    for(i = 0; i < TEST_PATTERN_REPS; i++)
    {
        fresult = f_write(&fil, test_pattern, sizeof(test_pattern), (void *)&bytes_written);
          
        if((bytes_written == 0) || (fresult != FR_OK))
        {
            sdcard_status = DIAG_SDCARD_STATUS_ERROR_WRITE;
            goto close;
        }
    }

    f_close(&fil);

    if(f_open(&fil, TEST_FILE, FA_READ) != FR_OK)
    {
        sdcard_status = DIAG_SDCARD_STATUS_ERROR_OPEN;
        goto remove;
    }

    for(i = 0; i < TEST_PATTERN_REPS; i++)
    {
        fresult = f_read(&fil, test_pattern_cmp, sizeof(test_pattern), (void *)&bytes_read);
          
        if((bytes_read == 0) || (fresult != FR_OK))
        {
            sdcard_status = DIAG_SDCARD_STATUS_ERROR_READ;
            goto close;
        }

        for(j = 0; j < sizeof(test_pattern); j++)
        {
            if(test_pattern_cmp[j] != test_pattern[j])
            {
                sdcard_status = DIAG_SDCARD_STATUS_ERROR_CORRUPT;
                goto close;
            }
        }
    }
close:
    f_close(&fil);
remove:
    f_unlink(sd_path_and_file);
    f_mount(NULL, "", 0);
exit:
    return sdcard_status;
}

static adv7511_status_t adv7511_run()
{
    adv7511_status_t adv7511_status = DIAG_ADV7511_STATUS_OK;

    /* Chip revision for AD7511w should be 0x13 according to datasheet */
    if(adv7511_rd_reg(0x00) != 0x13)
    {
        adv7511_status = DIAG_ADV7511_STATUS_ERROR_READ;
        goto exit;
    }

    if(adv7511_rd_reg(0x01) != 0x00)
    {
        adv7511_status = DIAG_ADV7511_STATUS_ERROR_READ;
        goto exit;
    }

    /* Registers can only be written if HDMI cable is inserted */

exit:
    return adv7511_status;
}

static void disp_run()
{
    uint32_t *disp_p = (uint32_t *)TEST_SDRAM_START_ADDR;
    uint32_t i;
    uint32_t k;
    uint32_t j;
    uint32_t color;

    disp_disable_clut(0);
    disp_disable_clut(1);

    disp_set_memory(0, TEST_SDRAM_START_ADDR);

    disp_set_layer(0,
                   0,
                   TEST_SCREEN_WIDTH,
                   0,
                   TEST_SCREEN_HEIGHT,
                   TEST_SCREEN_WIDTH,
                   TEST_SCREEN_HEIGHT,
                   255,
                   LTDC_PIXEL_FORMAT_ARGB8888);

    for(i = 0; i < TEST_SCREEN_HEIGHT; i++)
    {
        color = 0x800000;
        for(k = 0; k < 24; k++)
        {
            for(j = 0; j < TEST_SCREEN_WIDTH/24; j++)
            {
                *(disp_p + i * TEST_SCREEN_WIDTH + k * TEST_SCREEN_WIDTH/24 + j) = color | 0xFF000000;
            }
            color >>= 1;
        }
    }

    disp_activate_layer(0);
}   

diag_status_t diag_run()
{
    diag_status_t diag_status = DIAG_STATUS_OK;
    sdram_status_t sdram_status = DIAG_SDRAM_STATUS_OK;
    adv7511_status_t adv7511_status = DIAG_ADV7511_STATUS_OK;

    /* SRAM functionality */
    sdram_status = sdram_run();

    if(sdram_status != DIAG_SDRAM_STATUS_OK)
    {
        diag_status = DIAG_STATUS_ERROR_SDRAM;
        goto exit;
    }

    /* ADV7511 (I2C) functionality */
    adv7511_status = adv7511_run();

    if(adv7511_status != DIAG_ADV7511_STATUS_OK)
    {
        diag_status = DIAG_STATUS_ERROR_I2C;
        goto exit;
    }

    /* Display test screen for optical inspection */
    disp_run();

exit:
    return diag_status;
}

diag_status_t diag_sdcard_run()
{
    diag_status_t diag_status = DIAG_STATUS_OK;
    sdcard_status_t sdcard_status = DIAG_SDCARD_STATUS_OK;

    /* SD functionality */
    sdcard_status = sdcard_run();

    if(sdcard_status != DIAG_SDCARD_STATUS_OK)
    {
        diag_status = DIAG_STATUS_ERROR_SDCARD;
        goto exit;
    }

exit:
    return diag_status;
}

