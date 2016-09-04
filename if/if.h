/*
 * memwa2 interface specification
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


#ifndef _IF_H
#define _IF_H

/*********************** EMULATOR CC INTERFACE */

#define IF_MEMORY_CC_RAM_ACTUAL_SIZE        0x10000
#define IF_MEMORY_CC_KROM_ACTUAL_SIZE       0x2000
#define IF_MEMORY_CC_BROM_ACTUAL_SIZE       0x2000
#define IF_MEMORY_CC_CROM_ACTUAL_SIZE       0x1000
#define IF_MEMORY_DD_DOS_ACTUAL_SIZE        0x4000


#define IF_MEMORY_CC_SCREEN_BUFFER1_SIZE    0x100000
#define IF_MEMORY_CC_SCREEN_BUFFER2_SIZE    0x100000
#define IF_MEMORY_CC_SCREEN_BUFFER3_SIZE    0x100000
#define IF_MEMORY_CC_RAM_SIZE               0x10000
#define IF_MEMORY_CC_KROM_SIZE              0x10000
#define IF_MEMORY_CC_BROM_SIZE              0x10000
#define IF_MEMORY_CC_CROM_SIZE              0x10000
#define IF_MEMORY_CC_IO_SIZE                0x10000
#define IF_MEMORY_CC_UTIL1_SIZE             0x40000
#define IF_MEMORY_CC_UTIL2_SIZE             0x40000
#define IF_MEMORY_CC_SPRITE1_SIZE           0x40000
#define IF_MEMORY_CC_SPRITE2_SIZE           0x40000
#define IF_MEMORY_CC_SPRITE3_SIZE           0x40000
#define IF_MEMORY_CC_STAGE_FILES_SIZE       0x100000

#define IF_MEMORY_DD_ALL_SIZE               0x10000
#define IF_MEMORY_DD_UTIL1_SIZE             0x40000
#define IF_MEMORY_DD_UTIL2_SIZE             0x40000

typedef enum
{
    IF_DISPLAY_LAYER_BUFFER1, /* Size = 0x100000 (800x600x2) */
    IF_DISPLAY_LAYER_BUFFER2, /* Size = 0x100000 (800x600x2) */
    IF_DISPLAY_LAYER_BUFFER3 /* Size = 0x100000 (800x600x2) */
} if_display_layer_t;

typedef enum
{
    IF_FILE_STATUS_OK,
    IF_FILE_STATUS_ERROR
} if_file_status_t;

typedef enum
{
    IF_FILE_MODE_OPEN_EXISTING = 0x00,
    IF_FILE_MODE_READ = 0x01,
    IF_FILE_MODE_WRITE = 0x02,
    IF_FILE_MODE_CREATE_NEW = 0x04,
    IF_FILE_MODE_CREATE_ALWAYS = 0x08,
    IF_FILE_MODE_OPEN_ALWAYS = 0x10
} if_file_mode_t;

typedef enum
{
    IF_KEY_STATE_RELEASED,
    IF_KEY_STATE_PRESSED
} if_key_state_t;

typedef enum
{
    IF_JOYST_ACTION_STATE_RELEASED,
    IF_JOYST_ACTION_STATE_PRESSED
} if_joyst_action_state_t;

typedef enum
{
    IF_JOYST_PORT_A,
    IF_JOYST_PORT_B
} if_joyst_port_t;

typedef enum
{
    IF_JOYST_ACTION_UP,
    IF_JOYST_ACTION_DOWN,
    IF_JOYST_ACTION_RIGHT,
    IF_JOYST_ACTION_LEFT,
    IF_JOYST_ACTION_FIRE
} if_joyst_action_t;

typedef enum
{
    IF_MEM_CC_TYPE_RAM,       /* Size = 0x10000 */
    IF_MEM_CC_TYPE_KROM,      /* Size = 0x10000 */
    IF_MEM_CC_TYPE_BROM,      /* Size = 0x10000 */
    IF_MEM_CC_TYPE_CROM,      /* Size = 0x10000 */
    IF_MEM_CC_TYPE_IO,        /* Size = 0x10000 */
    IF_MEM_CC_TYPE_UTIL1,     /* Size = 0x40000 */
    IF_MEM_CC_TYPE_UTIL2,     /* Size = 0x40000 */
    IF_MEM_CC_TYPE_SPRITE1,   /* Size = 0x40000 */
    IF_MEM_CC_TYPE_SPRITE2,   /* Size = 0x40000 */
    IF_MEM_CC_TYPE_SPRITE3    /* Size = 0x20000 */
} if_mem_cc_type_t;

typedef struct
{
    uint8_t scan_code; /* Scan code from usb keyboard */
    uint8_t matrix_x; /* The x coordinate for c64 keyboard matrix */
    uint8_t matrix_y; /* The y coordinate for c64 keyboard matrix */
} if_keybd_map_t;

typedef void (*if_emu_cc_ue_joyst_t)(if_joyst_port_t if_joyst_port, if_joyst_action_t if_joyst_action, if_joyst_action_state_t if_action_state);
typedef void (*if_emu_cc_ue_keybd_t)(uint8_t *keybd_keys_p, uint8_t max_keys, if_key_state_t if_key_shift, if_key_state_t if_key_ctrl);
typedef void (*if_emu_cc_ue_keybd_map_set_t)(if_keybd_map_t *if_keybd_map_p);
typedef void (*if_emu_cc_display_layer_set_t)(uint8_t *layer_p);
typedef void (*if_emu_cc_mem_set_t)(uint8_t *mem_p, if_mem_cc_type_t if_mem_type);
typedef void (*if_emu_cc_op_init_t)();
typedef void (*if_emu_cc_op_run_t)(int32_t cycles);
typedef void (*if_emu_cc_op_reset_t)();
typedef void (*if_emu_cc_tape_drive_load_t)(uint32_t *fd_p);
typedef void (*if_emu_cc_tape_drive_play_t)();
typedef void (*if_emu_cc_tape_drive_stop_t)();
typedef void (*if_emu_cc_time_tenth_second_t)();
typedef void (*if_emu_cc_ports_write_serial_t)(uint8_t data);
typedef void (*if_emu_cc_display_limit_frame_rate_t)(uint8_t active);
typedef void (*if_emu_cc_display_lock_frame_rate_t)(uint8_t active);

typedef struct
{
    if_emu_cc_ue_joyst_t ue_joyst_fp;
    if_emu_cc_ue_keybd_t ue_keybd_fp;
    if_emu_cc_ue_keybd_map_set_t ue_keybd_map_set_fp;
} if_emu_cc_ue_t;

typedef struct
{
    if_emu_cc_display_layer_set_t display_layer_set_fp;
    if_emu_cc_display_limit_frame_rate_t display_limit_frame_rate_fp;
    if_emu_cc_display_lock_frame_rate_t display_lock_frame_rate_fp;
} if_emu_cc_display_t;

typedef struct
{
    if_emu_cc_mem_set_t mem_set_fp;
} if_emu_cc_mem_t;

typedef struct
{
    if_emu_cc_op_init_t op_init_fp;
    if_emu_cc_op_run_t op_run_fp;
    if_emu_cc_op_reset_t op_reset_fp;
} if_emu_cc_op_t;

typedef struct
{
    if_emu_cc_tape_drive_load_t tape_drive_load_fp;
    if_emu_cc_tape_drive_play_t tape_drive_play_fp;
    if_emu_cc_tape_drive_stop_t tape_drive_stop_fp;
} if_emu_cc_tape_drive_t;

typedef struct
{
    if_emu_cc_time_tenth_second_t time_tenth_second_fp;
} if_emu_cc_time_t;

typedef struct
{
    if_emu_cc_ports_write_serial_t if_emu_cc_ports_write_serial_fp;
} if_emu_cc_ports_t;

/*
 * Emulator cc main interface. These
 * functions should be implemented
 * by cc emulator.
 */
typedef struct
{
    if_emu_cc_ue_t if_emu_cc_ue;
    if_emu_cc_display_t if_emu_cc_display;
    if_emu_cc_mem_t if_emu_cc_mem;
    if_emu_cc_op_t if_emu_cc_op;
    if_emu_cc_tape_drive_t if_emu_cc_tape_drive;
    if_emu_cc_time_t if_emu_cc_time;
    if_emu_cc_ports_t if_emu_cc_ports;
} if_emu_cc_t;


/*********************** EMULATOR DD INTERFACE */

#define IF_MEMORY_DD_ALL_ACTUAL_SIZE           0x10000

typedef enum
{
    IF_MEM_DD_TYPE_ALL,       /* Size = 0x10000 */
    IF_MEM_DD_TYPE_UTIL1,     /* Size = 0x50000 */
    IF_MEM_DD_TYPE_UTIL2,     /* Size = 0x50000 */
} if_mem_dd_type_t;

typedef void (*if_emu_dd_mem_set_t)(uint8_t *mem_p, if_mem_dd_type_t if_mem_type);
typedef void (*if_emu_dd_op_init_t)();
typedef void (*if_emu_dd_op_run_t)(int32_t cycles);
typedef void (*if_emu_dd_op_reset_t)();
typedef void (*if_emu_dd_disk_drive_load_t)(uint32_t *fd_p);
typedef void (*if_emu_dd_ports_write_serial_t)(uint8_t data);

typedef struct
{
    if_emu_dd_mem_set_t mem_set_fp;
} if_emu_dd_mem_t;

typedef struct
{
    if_emu_dd_op_init_t op_init_fp;
    if_emu_dd_op_run_t op_run_fp;
    if_emu_dd_op_reset_t op_reset_fp;
} if_emu_dd_op_t;

typedef struct
{
    if_emu_dd_disk_drive_load_t disk_drive_load_fp;
} if_emu_dd_disk_drive_t;

typedef struct
{
    if_emu_dd_ports_write_serial_t if_emu_dd_ports_write_serial_fp;
} if_emu_dd_ports_t;

/*
 * Emulator dd main interface. These
 * functions should be implemented
 * by dd emulator.
 */
typedef struct
{
    if_emu_dd_mem_t if_emu_dd_mem;
    if_emu_dd_op_t if_emu_dd_op;
    if_emu_dd_disk_drive_t if_emu_dd_disk_drive;
    if_emu_dd_ports_t if_emu_dd_ports;
} if_emu_dd_t;


/*********************** HOST INTERFACE */

typedef enum
{
    IF_EMU_DEV_CC, /* Commodore Computer (c64) */
    IF_EMU_DEV_DD, /* Disk Drive (1541) */
} if_emu_dev_t;

typedef enum
{
    PRINT_TYPE_INFO,
    PRINT_TYPE_WARNING,
    PRINT_TYPE_ERROR,
    PRINT_TYPE_DEBUG
} print_type_t;

typedef uint32_t *(*if_host_filesys_open_t)(char *path_p, uint8_t mode);
typedef void (*if_host_filesys_close_t)(uint32_t *fd_p);
typedef uint32_t (*if_host_filesys_read_t)(uint32_t *fd_p, uint8_t *buf_p, uint32_t length);
typedef uint32_t (*if_host_filesys_write_t)(uint32_t *fd_p, uint8_t *buf_p, uint32_t length);
typedef void (*if_host_filesys_remove_t)(char *path_p);
typedef void (*if_host_filesys_flush_t)(uint32_t *fd_p);
typedef void (*if_host_filesys_seek_t)(uint32_t *fd_p, uint32_t pos);
typedef uint8_t (*if_host_sid_read_t)(uint8_t addr);
typedef void (*if_host_sid_write_t)(uint8_t addr, uint8_t data);
typedef uint32_t (*if_host_rand_get_t)();
typedef uint32_t (*if_host_time_get_ms_t)();
typedef void (*if_host_print_t)(char *string_p, print_type_t print_type);
typedef void (*if_host_stats_fps_t)(uint8_t fps);
typedef void (*if_host_stats_led_t)(uint8_t led);
typedef uint32_t (*if_host_calc_checksum_t)(uint8_t *buffer_p, uint32_t length);
typedef uint8_t (*if_host_ports_read_serial_t)(if_emu_dev_t if_emu_dev);
typedef void (*if_host_ports_write_serial_t)(if_emu_dev_t if_emu_dev, uint8_t data);
typedef void (*if_host_disp_flip_t)(uint8_t **done_buffer_pp);
typedef void (*if_host_ee_tape_play_t)(uint8_t play);
typedef void (*if_host_ee_tape_motor_t)(uint8_t motor);

typedef struct
{
    if_host_filesys_open_t filesys_open_fp;
    if_host_filesys_close_t filesys_close_fp;
    if_host_filesys_read_t filesys_read_fp;
    if_host_filesys_write_t filesys_write_fp;
    if_host_filesys_remove_t filesys_remove_fp;
    if_host_filesys_flush_t filesys_flush_fp;
    if_host_filesys_seek_t filesys_seek_fp;
} if_host_filesys_t;

typedef struct
{
    if_host_sid_read_t sid_read_fp;
    if_host_sid_write_t sid_write_fp;
} if_host_sid_t;

typedef struct
{
    if_host_rand_get_t rand_get_fp;
} if_host_rand_t;

typedef struct
{
    if_host_time_get_ms_t time_get_ms_fp;
} if_host_time_t;

typedef struct
{
    if_host_print_t print_fp;
} if_host_printer_t;

typedef struct
{
    if_host_stats_fps_t stats_fps_fp; /* from computer */
    if_host_stats_led_t stats_led_fp; /* from disk drive */
} if_host_stats_t;

typedef struct
{
    if_host_calc_checksum_t calc_checksum_fp;
} if_host_calc_t;

typedef struct
{
    if_host_ports_read_serial_t ports_read_serial_fp;
    if_host_ports_write_serial_t ports_write_serial_fp;
} if_host_ports_t;

typedef struct
{
    if_host_disp_flip_t disp_flip_fp;
} if_host_disp_t;

typedef struct
{
    if_host_ee_tape_play_t ee_tape_play_fp;
    if_host_ee_tape_motor_t ee_tape_motor_fp;
} if_host_ee_t;

/*
 * Host main interface. These
 * functions should be implemented
 * by host.
 */
typedef struct
{
    if_host_filesys_t if_host_filesys;
    if_host_sid_t if_host_sid;
    if_host_rand_t if_host_rand;
    if_host_time_t if_host_time;
    if_host_printer_t if_host_printer;
    if_host_stats_t if_host_stats;
    if_host_calc_t if_host_calc;
    if_host_ports_t if_host_ports;
    if_host_disp_t if_host_disp;
    if_host_ee_t if_host_ee;
} if_host_t;

#endif
