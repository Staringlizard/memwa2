/*
 * memwa2 tape (datasette) component
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
 * Contains functions to handle tapes (datasette).
 */

#include "tap.h"
#include "cpu.h"
#include "if.h"
#include "cia.h"
#include "bus.h"
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE     128
#define LONG_PULSE      0x1000

typedef struct
{
  uint32_t *file_p; /* File descriptor */
  uint8_t *buffer_p; /* Buffer containing a porion of file data */
  uint16_t offset; /* File offset */
  uint32_t fsize; /* File size as derived from header */
  uint32_t cycles_passed; /* Cycle counter */
  uint32_t cycles_bit_limit; /* Cycles until interrupt */
  uint8_t play;
  uint8_t version; /* Tap version (0x0, 0x1) */
  uint8_t motor;
}tape_t;

extern if_host_t g_if_host; /* Main interface */
extern cpu_on_chip_port_t g_cpu_on_chip_port; /* Cpu on chip port (tap owns one bit) */

static tape_t g_tape;

void tap_set_motor(uint8_t status)
{
  g_tape.motor = status & MASK_DATASETTE_MOTOR_CTRL;
  g_if_host.if_host_ee.ee_tape_motor_fp(!g_tape.motor); /* motor on = false */
}

void tap_insert_tape(uint32_t *fd_p)
{
    g_tape.file_p = fd_p;
}

void tap_init()
{
  g_cpu_on_chip_port.addr_one |= MASK_DATASETTE_BUTTON_STATUS; /* Unpress play button */
  g_tape.play = 0;

  memset(&g_tape, 0, sizeof(tape_t));
  g_tape.buffer_p = (uint8_t *)calloc(1, BUFFER_SIZE);
}

void tap_play()
{
  uint32_t bytes_read = 0;
  bytes_read = g_if_host.if_host_filesys.filesys_read_fp(g_tape.file_p, g_tape.buffer_p, BUFFER_SIZE);
  if(bytes_read != BUFFER_SIZE)
  {
    g_if_host.if_host_printer.print_fp("(CC) No tape file loaded!", PRINT_TYPE_ERROR);
    return;
  }

  g_cpu_on_chip_port.addr_one &= ~MASK_DATASETTE_BUTTON_STATUS; /* Press play button */
  g_tape.play = 1;
  g_if_host.if_host_ee.ee_tape_play_fp(g_tape.play);

  g_tape.fsize = *(g_tape.buffer_p + 0x10);
  g_tape.fsize += *(g_tape.buffer_p + 0x11) << 8;
  g_tape.fsize += *(g_tape.buffer_p + 0x12) << 16;
  g_tape.fsize += *(g_tape.buffer_p + 0x13) << 24;

  g_tape.version = *(g_tape.buffer_p + 0x0C);

  g_tape.offset = 0x14; // skip header

  g_tape.cycles_bit_limit = (*(g_tape.buffer_p + g_tape.offset) * 8); /* Read bit limit */
}

void tap_stop()
{
  memset(g_tape.buffer_p, 0, BUFFER_SIZE);
  memset(&g_tape, 0, sizeof(tape_t));

  g_cpu_on_chip_port.addr_one |= MASK_DATASETTE_BUTTON_STATUS; /* Unpress play button */
  g_tape.play = 0;
  g_if_host.if_host_ee.ee_tape_play_fp(g_tape.play);
}

static void load_buffer()
{
  uint32_t bytes_read = 0;
  if(g_tape.offset >= BUFFER_SIZE) /* Limit reached for buffer? */
  {
    if(g_tape.fsize < BUFFER_SIZE)
    {
      bytes_read = g_if_host.if_host_filesys.filesys_read_fp(g_tape.file_p, g_tape.buffer_p, g_tape.fsize);
      if(bytes_read != g_tape.fsize)
      {
        g_if_host.if_host_printer.print_fp("(CC) Error reading tap file!", PRINT_TYPE_ERROR);
      }
    }
    else
    {
      bytes_read = g_if_host.if_host_filesys.filesys_read_fp(g_tape.file_p, g_tape.buffer_p, BUFFER_SIZE);
      if(bytes_read != BUFFER_SIZE)
      {
        g_if_host.if_host_printer.print_fp("(CC) Error reading tap file!", PRINT_TYPE_ERROR);
      }
    }

    g_tape.offset = 0;
  }
}

void tap_step(uint8_t cc)
{
  if(g_tape.play == 1 && g_tape.motor == 0)  /* Tape and motor should be on if loading */
  {
    /* Check if exceeding limit */
    if((g_tape.cycles_passed + cc) < g_tape.cycles_bit_limit)
    {
      /* Nope, then just add cycles to counter and return */
      g_tape.cycles_passed += cc;
      return;
    }

    for(;cc != 0; cc--)
    {
      g_tape.cycles_passed += 1;

      if(g_tape.cycles_passed >= g_tape.cycles_bit_limit)
      {
        /* Tell cia about this */
        cia_request_irq_tape();

        /* Is this end of file? */
        if(g_tape.fsize == 0)
        {
          tap_stop();
          return;
        }

        load_buffer();

        g_tape.cycles_bit_limit = (*(g_tape.buffer_p + g_tape.offset) * 8); // read new bit limit
        g_tape.fsize--;
        g_tape.offset++;
        g_tape.cycles_passed = 0;

        if(g_tape.cycles_bit_limit == 0x0)  /* Special case! */
        {
          if(g_tape.version == 0x1)
          {
            /* Is this end of file? */
            if(g_tape.fsize == 0)
            {
              tap_stop();
              return;
            }

            load_buffer();
            /* Read 1 of three bytes that represents the new limit */
            g_tape.cycles_bit_limit = *(g_tape.buffer_p + g_tape.offset);
            g_tape.fsize--;
            g_tape.offset++;

            if(g_tape.fsize == 0)
            {
              tap_stop();
              return;
            }

            load_buffer();
            /* Read 1 of three bytes that represents the new limit */
            g_tape.cycles_bit_limit += (*(g_tape.buffer_p + g_tape.offset)) << 8;
            g_tape.fsize--;
            g_tape.offset++;
            
            if(g_tape.fsize == 0)
            {
              tap_stop();
              return;
            }
            
            load_buffer();
            /* Read 1 of three bytes that represents the new limit */
            g_tape.cycles_bit_limit += (*(g_tape.buffer_p + g_tape.offset) << 16);
            g_tape.fsize--;
            g_tape.offset++;
          }
          else
          {
            /* For this case the pulselength should be over 255*8 cycles */
            g_tape.cycles_bit_limit = LONG_PULSE;
          }
        }
      }
    }
  }
}

uint8_t tap_get_play()
{
  return g_tape.play;
}
