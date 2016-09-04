/*
 * memwa2 fdd component
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
 * This file contains a short cut for the fdd. Since the
 * documentation for 1541 is very limited the code in this file
 * is probably not correct. However, it seems to work with games
 * that are not doing any tricky stuff (like uploading and executing
 * custom code on the disk drive.
 */

#include "fdd.h"
#include "bus.h"
#include "cpu.h"
#include "if.h"

/*
  Bytes: $00-1F: First directory entry
          00-01: Track/Sector location of next directory sector ($00 $00 if
                 not the first entry in the sector)
             02: File type.
                 Typical values for this location are:
                   $00 - Scratched (deleted file entry)
                    80 - DEL
                    81 - SEQ
                    82 - PRG
                    83 - USR
                    84 - REL
                 Bit 0-3: The actual filetype
                          000 (0) - DEL
                          001 (1) - SEQ
                          010 (2) - PRG
                          011 (3) - USR
                          100 (4) - REL
                          Values 5-15 are illegal, but if used will produce
                          very strange results. The 1541 is inconsistent in
                          how it treats these bits. Some routines use all 4
                          bits, others ignore bit 3,  resulting  in  values
                          from 0-7.
                 Bit   4: Not used
                 Bit   5: Used only during SAVE-@ replacement
                 Bit   6: Locked flag (Set produces ">" locked files)
                 Bit   7: Closed flag  (Not  set  produces  "*", or "splat"
                          files)
          03-04: Track/sector location of first sector of file
          05-14: 16 character filename (in PETASCII, padded with $A0)
          15-16: Track/Sector location of first side-sector block (REL file
                 only)
             17: REL file record length (REL file only, max. value 254)
          18-1D: Unused (except with GEOS disks)
          1E-1F: File size in sectors, low/high byte  order  ($1E+$1F*256).
                 The approx. filesize in bytes is <= #sectors * 254
          20-3F: Second dir entry. From now on the first two bytes of  each
                 entry in this sector  should  be  $00  $00,  as  they  are
                 unused.
          40-5F: Third dir entry
          60-7F: Fourth dir entry
          80-9F: Fifth dir entry
          A0-BF: Sixth dir entry
          C0-DF: Seventh dir entry
          E0-FF: Eighth dir entry
*/

#define CBM_COMMAND_READ_SECTOR                         0x80
#define CBM_COMMAND_WRITE_SECTOR                        0x90
#define CBM_COMMAND_VERIFY_SECTOR                       0xA0
#define CBM_COMMAND_FETCH_HEADER_ID                     0xB0
#define CBM_COMMAND_BUMP_HEAD                           0xC0
#define CBM_COMMAND_EXEC_CODE                           0xD0
#define CBM_COMMAND_READ_SECTOR_HEADER_AND_EXEC_CODE    0xE0
#define CBM_COMMAND_READ_SECTOR_HEADER                  0xF0

#define REG_TRACK_BUFFER_0        0x06
#define REG_SECTOR_BUFFER_0       0x07
#define REG_TRACK_BUFFER_1        0x08
#define REG_SECTOR_BUFFER_1       0x09
#define REG_TRACK_BUFFER_2        0x0A
#define REG_SECTOR_BUFFER_2       0x0B
#define REG_TRACK_BUFFER_3        0x0C
#define REG_SECTOR_BUFFER_3       0x0D
#define REG_TRACK_BUFFER_4        0x0E
#define REG_SECTOR_BUFFER_4       0x0F

#define ADDRESS_HIGH_BUFFER_0     0x00
#define ADDRESS_HIGH_BUFFER_1     0x01
#define ADDRESS_HIGH_BUFFER_2     0x02
#define ADDRESS_HIGH_BUFFER_3     0x03
#define ADDRESS_HIGH_BUFFER_4     0x04

#define SECTOR_SIZE               0x100

static void event_write_cbm(uint16_t addr, uint8_t value);

typedef struct
{
  uint32_t offset;
} track_t;

extern memory_dd_t g_memory_dd; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

static track_t g_tracks[41] =
{
  {0x000000}, // track start at 1, not 0
  {0x000000}, // 1
  {0x001500}, // 2
  {0x002A00}, // 3
  {0x003F00}, // 4
  {0x005400}, // 5
  {0x006900}, // 6
  {0x007E00}, // 7
  {0x009300}, // 8
  {0x00A800}, // 9
  {0x00BD00}, // 10
  {0x00D200}, // 11
  {0x00E700}, // 12
  {0x00FC00}, // 13
  {0x011100}, // 14
  {0x012600}, // 15
  {0x013B00}, // 16
  {0x015000}, // 17
  {0x016500}, // 18
  {0x017800}, // 19
  {0x018B00}, // 20
  {0x019E00}, // 21
  {0x01B100}, // 22
  {0x01C400}, // 23
  {0x01D700}, // 24
  {0x01EA00}, // 25
  {0x01FC00}, // 26
  {0x020E00}, // 27
  {0x022000}, // 28
  {0x023200}, // 29
  {0x024400}, // 30
  {0x025600}, // 31
  {0x026700}, // 32
  {0x027800}, // 33
  {0x028900}, // 34
  {0x029A00}, // 35
  {0x02AB00}, // 36
  {0x02BC00}, // 37
  {0x02CD00}, // 38
  {0x02DE00}, // 39
  {0x02EF00}, // 40
};

static uint8_t g_command = 0;
static uint32_t *g_file_p;

static void event_write_cbm(uint16_t addr, uint8_t value)
{
  g_memory_dd.all_p[addr] = value;

  if(value >= 0x80 && (g_memory_dd.all_p[0x7F] == 0x00/* || g_memory_dd.all_p[0x7F] == 0x01*/)) // TODO: should drive 1 be allowed?
  {
    uint8_t buffer = 0;
    uint8_t track = 0;
    uint8_t sector = 0;
    uint8_t buffer_hi = 0;
    buffer = addr;
    g_command = value & 0xF0;

    switch(g_command)
    {
      case CBM_COMMAND_FETCH_HEADER_ID:
        g_memory_dd.all_p[buffer] = 0x01;
        break;
      case CBM_COMMAND_READ_SECTOR:
      case CBM_COMMAND_READ_SECTOR_HEADER_AND_EXEC_CODE:
      case CBM_COMMAND_READ_SECTOR_HEADER:
      case CBM_COMMAND_EXEC_CODE:
      {
        uint8_t buffer_low = 0;
        uint16_t buffer_pointer = 0;

        switch(buffer)
        {
          case ADDRESS_HIGH_BUFFER_0:
            buffer_low = 0x00;
            buffer_hi = 0x03;
            break;
          case ADDRESS_HIGH_BUFFER_1:
            buffer_low = 0x00;
            buffer_hi = 0x04;
            break;
          case ADDRESS_HIGH_BUFFER_2:
            buffer_low = 0x00;
            buffer_hi = 0x05;
            break;
          case ADDRESS_HIGH_BUFFER_3:
            buffer_low = 0x00;
            buffer_hi = 0x06;
            break;
          case ADDRESS_HIGH_BUFFER_4:
            buffer_low = 0x00;
            buffer_hi = 0x07;
            break;
        }

        buffer_pointer = buffer_hi << 8;
        buffer_pointer += buffer_low;

        switch(buffer)
        {
          case ADDRESS_HIGH_BUFFER_0:
            track = g_memory_dd.all_p[REG_TRACK_BUFFER_0];
            sector = g_memory_dd.all_p[REG_SECTOR_BUFFER_0];
            break;
          case ADDRESS_HIGH_BUFFER_1:
            track = g_memory_dd.all_p[REG_TRACK_BUFFER_1];
            sector = g_memory_dd.all_p[REG_SECTOR_BUFFER_1];
            break;
          case ADDRESS_HIGH_BUFFER_2:
            track = g_memory_dd.all_p[REG_TRACK_BUFFER_2];
            sector = g_memory_dd.all_p[REG_SECTOR_BUFFER_2];
            break;
          case ADDRESS_HIGH_BUFFER_3:
            track = g_memory_dd.all_p[REG_TRACK_BUFFER_3];
            sector = g_memory_dd.all_p[REG_SECTOR_BUFFER_3];
            break;
          case ADDRESS_HIGH_BUFFER_4:
            track = g_memory_dd.all_p[REG_TRACK_BUFFER_4];
            sector = g_memory_dd.all_p[REG_SECTOR_BUFFER_4];
            break;
        }

        g_if_host.if_host_filesys.filesys_seek_fp(g_file_p, g_tracks[track].offset + sector * SECTOR_SIZE);

        g_memory_dd.all_p[0x3f] = buffer; /* Previous work place in queue (0 - 5) */
        g_memory_dd.all_p[0x4c] = sector; /* Last read sector */

        if(g_command == CBM_COMMAND_READ_SECTOR)
        {
          uint32_t bytes_read;
          g_if_host.if_host_stats.stats_led_fp(0);
          bytes_read = g_if_host.if_host_filesys.filesys_read_fp(g_file_p, &g_memory_dd.all_p[buffer_pointer], 0x100);
          if(bytes_read != 0x100)
          {
            g_if_host.if_host_printer.print_fp("(DD) Error reading disk file!", PRINT_TYPE_ERROR);
          }
          g_if_host.if_host_stats.stats_led_fp(1);
        }

        if(g_command == CBM_COMMAND_READ_SECTOR_HEADER_AND_EXEC_CODE ||
          g_command == CBM_COMMAND_READ_SECTOR_HEADER)
        {
          // Fill in header
          g_memory_dd.all_p[0x18] = track;
          g_memory_dd.all_p[0x19] = sector;
        }

        g_memory_dd.all_p[buffer] = 0x01;

        if(g_command == CBM_COMMAND_READ_SECTOR_HEADER_AND_EXEC_CODE ||
          g_command == CBM_COMMAND_EXEC_CODE)
        {
          // Now jump to code in buffer
          cpu_jump_to_emu_addr(buffer_pointer); // Goto address
        }
      }
      break;
    case CBM_COMMAND_WRITE_SECTOR:
      g_memory_dd.all_p[buffer] = 0x01;
      break;
    case CBM_COMMAND_VERIFY_SECTOR:
      g_memory_dd.all_p[buffer] = 0x01;
      break;
    case CBM_COMMAND_BUMP_HEAD:
      g_memory_dd.all_p[buffer] = 0x01;
      break;
    }
  }
}

void fdd_init()
{
    bus_dd_event_write_subscribe(0x0000, event_write_cbm);
    bus_dd_event_write_subscribe(0x0001, event_write_cbm);
    bus_dd_event_write_subscribe(0x0002, event_write_cbm);
    bus_dd_event_write_subscribe(0x0003, event_write_cbm);
    bus_dd_event_write_subscribe(0x0004, event_write_cbm);
}

void fdd_insert_disk(uint32_t *fd_p)
{
  g_file_p = fd_p;
}
