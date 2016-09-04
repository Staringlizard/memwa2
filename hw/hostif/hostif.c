/*
 * memwa2 emulator-host interface
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
 * Host interface implementation. Contains all functions necessary for
 * the emu libs to work. The interface for this file is dictated by
 * if.h file.
 */

#include "hostif.h"
#include "rng.h"
#include "config.h"
#include "diag.h"
#include "usbh_core.h"
#include "usbh_hid_keybd.h"
#include "console.h"
#include "keybd.h"
#include "if.h"
#include "romcc.h"
#include "disp.h"
#include "ff.h"
#include "stage.h"
#include "crc.h"
#include "sidbus.h"
#include "sm.h"

uint32_t *if_host_filesys_open(char *path_p, uint8_t mode);
void if_host_filesys_close(uint32_t *fd_p);
uint32_t if_host_filesys_read(uint32_t *fd_p, uint8_t *buf_p, uint32_t length);
uint32_t if_host_filesys_write(uint32_t *fd_p, uint8_t *buf_p, uint32_t length);
void if_host_filesys_remove(char *path_p);
void if_host_filesys_flush(uint32_t *fd_p);
void if_host_filesys_seek(uint32_t *fd_p, uint32_t pos);
void if_host_sid_write(uint8_t addr, uint8_t data);
uint8_t if_host_sid_read(uint8_t addr);
uint32_t if_host_rand_get();
uint32_t if_host_time_get_ms();
void if_host_print(char *string_p, print_type_t print_type);
void if_host_stats_fps(uint8_t fps);
void if_host_stats_led(uint8_t led);
uint32_t if_host_calc_checksum(uint8_t *buffer_p, uint32_t length);
uint8_t if_host_ports_read_serial(if_emu_dev_t if_emu_dev);
void if_host_ports_write_serial(if_emu_dev_t if_emu_dev, uint8_t data);
void if_host_disp_flip(uint8_t **done_buffer_pp);
void if_host_ee_tape_play(uint8_t play);
void if_host_ee_tape_motor(uint8_t motor);

if_host_t g_if_host =
{
    {
        if_host_filesys_open,
        if_host_filesys_close,
        if_host_filesys_read,
        if_host_filesys_write,
        if_host_filesys_remove,
        if_host_filesys_flush,
        if_host_filesys_seek
    },
    {
        if_host_sid_read,
        if_host_sid_write
    },
    {
        if_host_rand_get
    },
    {
        if_host_time_get_ms
    },
    {
        if_host_print
    },
    {
        if_host_stats_fps,
        if_host_stats_led
    },
    {
        if_host_calc_checksum
    },
    {
        if_host_ports_read_serial,
        if_host_ports_write_serial,
    },
    {
        if_host_disp_flip
    },
    {
        if_host_ee_tape_play,
        if_host_ee_tape_motor
    }
};

extern if_emu_cc_t g_if_cc_emu;
extern if_emu_dd_t g_if_dd_emu;

uint32_t *if_host_filesys_open(char *path_p, uint8_t mode)
{
    FRESULT res;
    FIL *fd_p;
    fd_p = (FIL *)malloc(sizeof(FIL));
    res = f_open((FIL *)fd_p, path_p, mode); /* Mode from if.h is mapped directly to fatfs */

    if(res == FR_OK)
    {
        return (uint32_t *)fd_p;
    }
    else
    {
        free(fd_p);
        return (uint32_t *)NULL;
    }
}

void if_host_filesys_close(uint32_t *fd_p)
{
    f_close((FIL *)fd_p);
    free(fd_p);
}

uint32_t if_host_filesys_read(uint32_t *fd_p, uint8_t *buf_p, uint32_t length)
{
    FRESULT res;
    uint32_t bytes_read;
    res = f_read((FIL *)fd_p, buf_p, length, (UINT *)&bytes_read);

    if(bytes_read != 0 && res == FR_OK)
    {
        return bytes_read;
    }
    else
    {
        return 0;
    }
}

uint32_t if_host_filesys_write(uint32_t *fd_p, uint8_t *buf_p, uint32_t length)
{
    FRESULT res;
    uint32_t bytes_written;
    res = f_write((FIL *)fd_p, buf_p, length, (UINT *)&bytes_written);

    if(bytes_written != 0 && res == FR_OK)
    {
        return bytes_written;
    }
    else
    {
        return 0;
    }
}

void if_host_filesys_remove(char *path_p)
{
    FRESULT res;
    res = f_unlink(path_p);

    if(res != FR_OK)
    {
        ; /* print something perhaps? */
    }
}

void if_host_filesys_flush(uint32_t *fd_p)
{
    FRESULT res;
    res = f_sync((FIL *)fd_p);

    if(res != FR_OK)
    {
        ; /* print something perhaps? */
    }
}

void if_host_filesys_seek(uint32_t *fd_p, uint32_t pos)
{
    FRESULT res;
    res = f_lseek((FIL *)fd_p, pos);

    if(res != FR_OK)
    {
        ; /* print something perhaps? */
    }
}

void if_host_sid_write(uint8_t addr, uint8_t data)
{
    sidbus_write(addr, data);
}

uint8_t if_host_sid_read(uint8_t addr)
{
    return sidbus_read(addr);
}

uint32_t if_host_rand_get()
{
    return rng_get();
}

uint32_t if_host_time_get_ms()
{
    return HAL_GetTick();
}

void if_host_print(char *string_p, print_type_t print_type)
{
    switch(print_type)
    {
    case PRINT_TYPE_INFO:
        printf("[INFO] %s\n", string_p);
        break;
    case PRINT_TYPE_WARNING:
        printf("[WARN] %s\n", string_p);
        break;
    case PRINT_TYPE_ERROR:
        printf("[ERR] %s\n", string_p);
        stage_set_message(string_p);
        stage_draw_info(INFO_PRINT, 0);
        sm_error_occured();
        break;
    case PRINT_TYPE_DEBUG:
        printf("[DEBUG] %s\n", string_p);
        break;
    }
}

void if_host_stats_fps(uint8_t fps)
{
    if(sm_get_disp_stats_flag())
    {
        stage_draw_info(INFO_FPS, fps);
    }
}

void if_host_stats_led(uint8_t led)
{
    if(sm_get_disp_stats_flag())
    {
        stage_draw_info(INFO_DISK_LED, led);
    }
}

uint32_t if_host_calc_checksum(uint8_t *buffer_p, uint32_t length)
{
#ifdef USE_CRC
    return crc_calculate(buffer_p, length);
#else
    uint32_t res = 0;
    uint32_t i;

    for(i = 0; i < length; i++)
    {
        res ^= buffer_p[i];
    }

    return res;
#endif
}

uint8_t if_host_ports_read_serial(if_emu_dev_t if_emu_dev)
{
    switch(if_emu_dev)
    {
        case IF_EMU_DEV_CC:
          break;
        case IF_EMU_DEV_DD:
          break;
    }

    return 0; /* Not used atm */
}

void if_host_ports_write_serial(if_emu_dev_t if_emu_dev, uint8_t data)
{
    switch(if_emu_dev)
    {
        case IF_EMU_DEV_CC: /* Computer writes to serial port */
            g_if_dd_emu.if_emu_dd_ports.if_emu_dd_ports_write_serial_fp(data);
            break;
        case IF_EMU_DEV_DD: /* Disk drive writes to serial port */
            g_if_cc_emu.if_emu_cc_ports.if_emu_cc_ports_write_serial_fp(data);
            break;
    }
}

void if_host_disp_flip(uint8_t **done_buffer_pp)
{
    disp_flip_buffer(done_buffer_pp);
}

void if_host_ee_tape_play(uint8_t play)
{
    sm_tape_play(play);
}

void if_host_ee_tape_motor(uint8_t motor)
{
    if(sm_get_disp_stats_flag())
    {
        stage_draw_info(INFO_TAPE_MOTOR, motor);
    }
}
