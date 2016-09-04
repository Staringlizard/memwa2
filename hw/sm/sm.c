/*
 * memwa2 state machine
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
 * This is the main state machine. It handles all different states
 * for the software. It works closely together with stage and keybd
 * utilities.
 */

#include "sm.h"
#include "if.h"
#include "keybd.h"
#include "disp.h"
#include "stage.h"
#include "ff.h"
#include "main.h"
#include "timer.h"

#include <stdlib.h>
#include <string.h>

#define CLOCK_PAL               985248
#define MAX_EXEC_CYCLES         400
#define LONG_PRESS_MS           500
#define LONG_PRESS_DELAY_MS     30

extern if_emu_cc_t g_if_cc_emu;
extern if_emu_dd_t g_if_dd_emu;

static FIL *g_fd_p;
static sm_state_t g_current_state;
static keybd_state_t g_prev_key_state;
static keybd_state_t g_prev_shift_state;
static keybd_state_t g_prev_ctrl_state;
static uint8_t g_prev_keys_active;
static uint8_t g_key_active;
static uint32_t g_key_active_start;
static uint32_t g_key_active_long_press_delay;
static uint8_t g_disk_drive_on;
static uint8_t g_lock_freq_pal;
static uint8_t g_disp_info;
static uint8_t g_limit_frame_rate; /* Emulator can half its emulated frame rate to gain performance */
static uint8_t g_tape_play;

static void show_info_bar(uint8_t show)
{
    if(show)
    {
        disp_activate_layer(0);
        stage_draw_info(INFO_DISK, g_disk_drive_on);
        stage_draw_info(INFO_FPS, 0);
        stage_draw_info(INFO_FRAMERATE, g_limit_frame_rate);
        stage_draw_info(INFO_FREQLOCK, g_lock_freq_pal);
        stage_draw_info(INFO_TAPE_BUTTON, g_tape_play);
        stage_draw_fw(); /* Upper right corner is reserved for this */
    }
    else
    {
        disp_deactivate_layer(0);
    }
}

static void change_state(sm_state_t new_state)
{
    if(g_current_state == new_state)
    {
        return; /* No change */
    }
    else
    {
        g_current_state = new_state;
    }

    /* Do some init stuff before changing state */
    switch(new_state)
    {
        case SM_STATE_EMULATOR:
            stage_prepare(STAGE_EMULATION);
            show_info_bar(g_disp_info);
            break;
        case SM_STATE_MENU:
            stage_prepare(STAGE_MENU);
            break;
        case SM_STATE_FILE_SELECT:
            stage_prepare(STAGE_FILE_SELECT);
            break;
        case SM_STATE_COLOR_SCREEN:
            stage_prepare(STAGE_COLOR_SCREEN);
            break;
        case SM_STATE_ERROR:
            show_info_bar(1);
            break;
    }
}

static uint8_t read_t64_file(FIL *fd_p)
{
    FRESULT res = FR_OK;
    uint8_t return_val = 1;
    uint8_t header_p[0x40];
    uint8_t entry_p[0x20];
    uint16_t start_address;
    uint16_t end_address;
    uint16_t entry_size;
    uint32_t entry_offset;
    uint32_t bytes_read;
    //uint8_t files;
    uint8_t i;

    /* Read header which is not used atm */
    res = f_read(fd_p, header_p, 0x40, (void *)&bytes_read);

    if(res != FR_OK || bytes_read != 0x40)
    {
        return_val = 0;
        goto exit;
    }

    /* Get number of files in container */
    //files = header_p[0x24];

    /* Just step through the first file */
    for(i = 0; i < 1; i++)
    {
        /* Go to the start position for the entries */
        res = f_lseek(fd_p, 0x40 + 0x20 * i);
        if(res != FR_OK)
        {
            return_val = 0;
            goto exit;
        }

        /* Read entry information */
        res = f_read(fd_p, entry_p, 0x20, (void *)&bytes_read);

        if(res != FR_OK || bytes_read != 0x20)
        {
            return_val = 0;
            goto exit;
        }

        /*
         * From T64 specification:
         * In reality any value that is not a $00 is seen as a
         * PRG file.
         */
        if(entry_p[1] == 0x00)
        {
            /* This file is not PRG */
            return_val = 0;
            goto exit;
        }

        /* Get start address */
        start_address = entry_p[0x3] << 8;
        start_address += entry_p[0x2];

        /*
         * From T64 specification:
         * If the file is a snapshot, the address will be 0.
         */
        if(start_address == 0x0)
        {
            main_error("T64 snapshots are not supported!", __FILE__, __LINE__, 0);
        }

        /* Get end address */
        end_address = entry_p[0x5] << 8;
        end_address += entry_p[0x4];

        /*
         * From T64 specification:
         * As mentioned previously, there are faulty T64 files around
         * with the 'end load address' value set to $C3C6.
         */
        if(end_address == 0xC3C6)
        {
            main_error("T64 file is corrupt!", __FILE__, __LINE__, 0);
        }

        /* Calculate size of entry */
        entry_size = end_address - start_address;

        entry_offset = entry_p[0xb] << 24;
        entry_offset += entry_p[0xa] << 16;
        entry_offset += entry_p[0x9] << 8;
        entry_offset += entry_p[0x8];

        /* Go to the start position for the entry in file */
        res = f_lseek(fd_p, entry_offset);
        if(res != FR_OK)
        {
            return_val = 0;
            goto exit;
        }

        /* Start writing into the ram memory, which we gave to emulator at startup */
        res = f_read(fd_p, (uint8_t *)(CC_RAM_BASE_ADDR + start_address), entry_size, (void *)&bytes_read);

        if(res != FR_OK || bytes_read != entry_size)
        {
            return_val = 0;
            goto exit;
        }
    }

exit:
    return return_val;
}

static uint8_t read_prg_file(FIL *fd_p)
{
    uint32_t bytes_read;
    FRESULT res = FR_OK;
    uint8_t return_val = 1;
    uint16_t start_address;
    uint8_t entry_p[0x4];

    /* Read entry information */
    res = f_read(fd_p, entry_p, 2, (void *)&bytes_read);

    if(res != FR_OK || bytes_read != 2)
    {
        return_val = 0;
        goto exit;
    }

    start_address = entry_p[0] + (entry_p[1] << 8);

    /* These kind of file is just loaded right into emu memory */
    res = f_read(fd_p, (uint8_t *)(CC_RAM_BASE_ADDR + start_address), 0x10000, (void *)&bytes_read);

    if(res != FR_OK || bytes_read == 0)
    {
        main_error("Failed to read PRG file!", __FILE__, __LINE__, 0);
        return_val = 0;
        goto exit;
    }

exit:
    return return_val;
}



static void read_keybd()
{
    uint8_t keys_active = keybd_get_active_keys_hash();
    keybd_state_t key_state = keybd_key_state();
    keybd_state_t shift_state = keybd_get_shift_state();
    keybd_state_t ctrl_state = keybd_get_ctrl_state();

    /* Any change ? */
    if(keys_active == g_prev_keys_active &&
       key_state == g_prev_key_state &&
       shift_state == g_prev_shift_state &&
       ctrl_state == g_prev_ctrl_state)
    {
        return;
    }
    else
    {
        g_prev_keys_active = keys_active;
        g_prev_key_state = key_state;
        g_prev_shift_state = shift_state;
        g_prev_ctrl_state = ctrl_state;
    }

    g_key_active = keybd_get_active_key();
    if(g_key_active != 0)
    {
        g_key_active_start = timer_get_ms();
    }
    else
    {
        g_key_active_start = 0;
    }

    /* Keycodes regardless of state */
    if(ctrl_state == KEYBD_STATE_PRESSED)
    {
        switch(g_key_active)
        {
            case 0x29: /* CTRL + ESC */
                if(g_current_state == SM_STATE_EMULATOR)
                {
                    change_state(SM_STATE_MENU);
                }
                else
                {
                    change_state(SM_STATE_EMULATOR);
                }
                break;
            case 0x3A: /* CTRL + F1 */
                if(g_current_state == SM_STATE_EMULATOR)
                {
                    g_disp_info = !g_disp_info;
                    show_info_bar(g_disp_info);
                }
                break;
            case 0x3B: /* CTRL + F2 */
                g_disk_drive_on = !g_disk_drive_on;
                if(g_disp_info)
                {
                    stage_draw_info(INFO_DISK, g_disk_drive_on);
                }
                break;
            case 0x3C: /* CTRL + F3 */
                g_lock_freq_pal = !g_lock_freq_pal;
                if(g_disp_info)
                {
                    stage_draw_info(INFO_FREQLOCK, g_lock_freq_pal);
                }
                g_if_cc_emu.if_emu_cc_display.display_lock_frame_rate_fp(g_lock_freq_pal);
                break;
            case 0x3D: /* CTRL + F4 */
                g_limit_frame_rate = !g_limit_frame_rate;
                if(g_disp_info)
                {
                    stage_draw_info(INFO_FRAMERATE, g_limit_frame_rate);
                }
                g_if_cc_emu.if_emu_cc_display.display_limit_frame_rate_fp(g_limit_frame_rate);
                break;
            case 0x3E: /* CTRL + F5 */
                g_tape_play = !g_tape_play;
                if(g_tape_play)
                {
                    g_if_cc_emu.if_emu_cc_tape_drive.tape_drive_play_fp();
                }
                else
                {
                    g_if_cc_emu.if_emu_cc_tape_drive.tape_drive_stop_fp();
                }
                break;
            case 0x3F: /* CTRL + F6 */
                stage_clear_last_messsage();
                /* Make sure to leave any error state */
                change_state(SM_STATE_EMULATOR);
                break;
            case 0x42: /* CTRL + F9 */
                if(g_current_state == SM_STATE_EMULATOR)
                {
                    change_state(SM_STATE_COLOR_SCREEN);
                }
                else
                {
                    change_state(SM_STATE_EMULATOR);
                }
                break;
            case 0x43: /* CTRL + F10 */
                g_if_cc_emu.if_emu_cc_op.op_reset_fp();
                g_if_dd_emu.if_emu_dd_op.op_reset_fp();
                break;
            case 0x44: /* CTRL + F11 */
                {
                    change_state(SM_STATE_EMULATOR);

                    g_if_cc_emu.if_emu_cc_op.op_init_fp();
                    g_if_dd_emu.if_emu_dd_op.op_init_fp();

                    /* Reset disk drive setting */
                    g_disk_drive_on = 0;
                    if(g_disp_info)
                    {
                        stage_draw_info(INFO_DISK, g_disk_drive_on);
                    }

                    /* Reset freq lock setting */
                    g_lock_freq_pal = 1;
                    if(g_disp_info)
                    {
                        stage_draw_info(INFO_FREQLOCK, g_lock_freq_pal);
                    }
                    g_if_cc_emu.if_emu_cc_display.display_lock_frame_rate_fp(g_lock_freq_pal);

                    /* Reset frame rate setting */
                    g_limit_frame_rate = 1;
                    if(g_disp_info)
                    {
                        stage_draw_info(INFO_FRAMERATE, g_limit_frame_rate);
                    }
                    g_if_cc_emu.if_emu_cc_display.display_limit_frame_rate_fp(g_limit_frame_rate);

                    /* Reset all emulator vic pointers */
                    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE1_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE1);
                    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE2_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE2);
                    g_if_cc_emu.if_emu_cc_mem.mem_set_fp((uint8_t *)CC_SPRITE3_BASE_ADDR, IF_MEM_CC_TYPE_SPRITE3);
                    g_if_cc_emu.if_emu_cc_display.display_layer_set_fp((uint8_t *)CC_DISP_BUFFER2_ADDR);
                    stage_clear_last_messsage();
                }
                break;
            case 0x45: /* CTRL + F12 */
                NVIC_SystemReset();
                break;
            case 79: /* Right Arrow */
                break;
            case 80: /* Left Arrow */
                break;
            case 82: /* Up Arrow */
                break;
            case 81: /* Down Arrow */
                break;
        }
        return; /* No need to go further and possibly feed emulator with button press */
    }

    switch(g_current_state)
    {
        case SM_STATE_EMULATOR:
            g_if_cc_emu.if_emu_cc_ue.ue_keybd_fp(keybd_get_active_keys(), 6, shift_state, ctrl_state);
            break;
        case SM_STATE_MENU:
            if(g_key_active == 40) /* Return key */
            {
                change_state(SM_STATE_FILE_SELECT);
            }

            if(key_state == KEYBD_STATE_PRESSED)
            {
                stage_input_key(STAGE_MENU, g_key_active);
            }
            break;
        case SM_STATE_FILE_SELECT:
            if(g_key_active == 40) /* Return key */
            {
                FRESULT res = FR_OK;
                char *path_p = stage_get_selected_path();
                char *filename_p = stage_get_selected_filename();
                char *path_and_filename_p = (char *)calloc(1, strlen(path_p) + strlen(filename_p) + 2);
                sprintf(path_and_filename_p, "%s/%s", path_p, filename_p);

                /* If file was previously opened, then free fd before allocating new one */
                if(g_fd_p != NULL)
                {
                    f_close(g_fd_p);
                    free(g_fd_p);
                    g_fd_p = NULL;
                }

                /* Create new fd and try and open file */
                g_fd_p = (FIL *)calloc(1, sizeof(FIL));
                res = f_open(g_fd_p, path_and_filename_p, FA_READ);
                free(path_and_filename_p);
                if(res != FR_OK)
                {
                    free(g_fd_p);
                    g_fd_p = NULL;
                    break;
                }

                file_list_type_t file_list_type = stage_get_selected_file_list_type();
                switch(file_list_type)
                {
                    case FILE_LIST_TYPE_CASETTE:
                        g_if_cc_emu.if_emu_cc_tape_drive.tape_drive_load_fp((uint32_t *)g_fd_p);
                        change_state(SM_STATE_EMULATOR);
                        break;
                    case FILE_LIST_TYPE_FLOPPY:
                        g_if_dd_emu.if_emu_dd_disk_drive.disk_drive_load_fp((uint32_t *)g_fd_p);
                        change_state(SM_STATE_EMULATOR);
                        break;
                    case FILE_LIST_TYPE_T64:
                        {
                            if(read_t64_file(g_fd_p))
                            {
                                change_state(SM_STATE_EMULATOR);
                            }
                        }
                        break;
                    case FILE_LIST_TYPE_PRG:
                        {
                            if(read_prg_file(g_fd_p))
                            {
                                change_state(SM_STATE_EMULATOR);
                            }
                        }
                        break;
                    default:
                        ;
                }
            }

            if(key_state == KEYBD_STATE_PRESSED)
            {
                stage_input_key(STAGE_FILE_SELECT, g_key_active);
            }

            break;
        case SM_STATE_COLOR_SCREEN:
            if(key_state == KEYBD_STATE_PRESSED)
            {
                stage_input_key(STAGE_COLOR_SCREEN, g_key_active);
            }
            break;
        case SM_STATE_ERROR:
            ;
            break;
    }
}

void sm_init()
{
    g_fd_p = NULL;
    change_state(SM_STATE_EMULATOR);

    g_if_cc_emu.if_emu_cc_op.op_init_fp();
    g_if_dd_emu.if_emu_dd_op.op_init_fp();

    g_disk_drive_on = 0;
    g_lock_freq_pal = 1;
    g_disp_info = 0;
    g_limit_frame_rate = 1;
    g_tape_play = 0;
    g_if_cc_emu.if_emu_cc_display.display_lock_frame_rate_fp(g_lock_freq_pal);
    g_if_cc_emu.if_emu_cc_display.display_limit_frame_rate_fp(g_limit_frame_rate);
}

void sm_run()
{
    while(1)
    {
        switch(g_current_state)
        {
            case SM_STATE_EMULATOR:
                if(g_disk_drive_on)
                {
                    /* If disk drive is turned on, then run both emulators */
                    g_if_cc_emu.if_emu_cc_op.op_run_fp(25);
                    g_if_dd_emu.if_emu_dd_op.op_run_fp(25);
                }
                else
                {
                    g_if_cc_emu.if_emu_cc_op.op_run_fp(MAX_EXEC_CYCLES);
                }

                break;
            case SM_STATE_MENU:
                stage_draw_selection(STAGE_MENU);
                break;
            case SM_STATE_FILE_SELECT:
                /* Handle long press */
                if(g_key_active_start != 0 &&
                   (timer_get_ms() - g_key_active_start) > LONG_PRESS_MS)
                {
                    if((timer_get_ms() - g_key_active_long_press_delay) > LONG_PRESS_DELAY_MS)
                    {
                        stage_input_key(STAGE_FILE_SELECT, g_key_active);
                        g_key_active_long_press_delay = timer_get_ms();
                    }
                }
                stage_draw_selection(STAGE_FILE_SELECT);
                break;
            case SM_STATE_COLOR_SCREEN:
                ;
                break;
            case SM_STATE_ERROR:
                ;
                break;
        }
        keybd_poll();
        read_keybd();
    }
}

uint8_t sm_get_disp_stats_flag()
{
    return g_disp_info;
}

void sm_error_occured()
{
    change_state(SM_STATE_ERROR);
}

sm_state_t sm_get_state()
{
    return g_current_state;
}

void sm_tape_play(uint8_t play)
{
    g_tape_play = play;
    if(g_disp_info)
    {
        stage_draw_info(INFO_TAPE_BUTTON, g_tape_play);
    }
}
