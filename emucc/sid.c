/*
 * memwa2 sid component
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
 * SID implementation (MOS6581)
 */

#include "sid.h"
#include "bus.h"
#include "if.h"

extern memory_t g_memory; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

static void event_write(uint16_t addr, uint8_t value)
{
  if(addr >= SID_READ_ONLY_LOW && addr <= SID_READ_ONLY_HIGH) /* Read only */
  {
    ;
  }
  else /* Write only */
  {
    g_if_host.if_host_sid.sid_write_fp(addr & 0x1F, value);
  }
}

static uint8_t event_read(uint16_t addr)
{
  if(addr >= SID_READ_ONLY_LOW && addr <= SID_READ_ONLY_HIGH) /* Read only */
  {
    return g_if_host.if_host_sid.sid_read_fp(addr & 0x1F);
  }
  else /* Write only */
  {
    return 0; 
  }
}

void sid_init()
{
  uint32_t i;

  /* Clear */
  for(i = 0; i < 0x20; i++)
  {
    event_write(0xD400 + i, 0x00);
  }

  /* Subscribe to sid write address range */
  for(i = 0; i < 0x20; i++)
  {
    bus_event_write_subscribe(0xD400 + i, event_write);
  }

  /* Subscribe to sid read address range */
  for(i = 0; i < 0x20; i++)
  {
    bus_event_read_subscribe(0xD400 + i, event_read);
  }
}


