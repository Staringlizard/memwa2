/*
 * memwa2 via component
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
 * VIA implementation (MOS6522).
 * The functionality is far from complete, but contains
 * the most important features.
 */

#include "bus.h"
#include "via.h"
#include "cpu.h"
#include "if.h"
#include <string.h>

#define VIA1 0
#define VIA2 1

#define A    0
#define B    1

#define MIN_QUEUE_SIZE  0x20

static uint8_t via1_event_read_irq_enable(uint16_t addr);
static void via1_event_write_timer_lb(uint16_t addr, uint8_t value);
static void via1_event_write_timer_hb(uint16_t addr, uint8_t value);
static void via1_event_write_timer_latch_ap_lb(uint16_t addr, uint8_t value);
static void via1_event_write_timer_latch_ap_hb(uint16_t addr, uint8_t value);
static void via1_event_write_irq_enable(uint16_t addr, uint8_t value);
static void via1_event_write_porta(uint16_t addr, uint8_t value);
static uint8_t via2_event_read_irq_enable(uint16_t addr);
static void via2_event_write_timer_lb(uint16_t addr, uint8_t value);
static void via2_event_write_timer_hb(uint16_t addr, uint8_t value);
static void via2_event_write_timer_latch_ap_lb(uint16_t addr, uint8_t value);
static void via2_event_write_timer_latch_ap_hb(uint16_t addr, uint8_t value);
static void via2_event_write_irq_enable(uint16_t addr, uint8_t value);

typedef struct
{
  uint32_t status_control_reg[1];
  uint32_t status_irq_mask_reg;
  uint16_t *timer_ap[1];
  uint16_t *timer_latch_ap[1];
} via_t;

extern memory_dd_t g_memory_dd; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

static via_t g_via_a[2];
static uint32_t g_cycles_in_queue;
static uint8_t g_serial_port_input;
static uint8_t g_serial_port_output;
static uint8_t g_atn_previous;
static uint8_t g_atn_current;

enum timer_ctrl_t {
  ONE_SHOT_NO_PB7     = 0x00, // Timed Interrupt when Timer 1 is loaded, no PB7
  CONTINUEOUS_NO_PB7  = 0x40, // Continuous Interrupts, no PB7
  ONE_SHOT_ON_PB7     = 0x80, // Timed Interrupt when Timer 1 is loaded, one-shot on PB7
  CONTINUEOUS_ON_PB7  = 0xC0  // Continuous Interrupts, square-wave on PB7
};

static uint16_t via_reg_timer_low[2] =
{
  REG_VIA1_TIMER_LOW_BYTE,
  REG_VIA2_TIMER_LOW_BYTE
};

static uint16_t via_reg_timer_latch_ap_low[2] =
{
  REG_VIA1_TIMER_LATCH_LOW_BYTE,
  REG_VIA2_TIMER_LATCH_LOW_BYTE
};

/*static uint16_t via_reg_ctrl[2] =
{
  REG_VIA1_AUXILIARY_CTRL,
  REG_VIA2_AUXILIARY_CTRL
};*/

static uint16_t via_reg_int[2] =
{
  REG_VIA1_IRQ_STATUS,
  REG_VIA2_IRQ_STATUS
};

static uint8_t via_int_reason_and_msb[1] =
{
  MASK_VIA_IRQ_STATUS_IRQ_OCCURED | MASK_VIA_IRQ_STATUS_TIMER
};

static uint8_t via_int_reason[1] =
{
  MASK_VIA_IRQ_STATUS_TIMER
};

static uint8_t via_int_mask[1] =
{
  MASK_VIA_IRQ_ENABLE_TIMER
};

static uint8_t eval_via1_portb()
{
  uint8_t reg = g_serial_port_output;

  /*
   * The bus (serial port) between computer and disk drive is pretty complicated.
   * Hw schematics are necessary to completely understand this.
   */


  /*
   * The disk drive (dd) is driving the clock bus with VIA1 PB3. This pin is isolated and is
   * not affected by the input from computer. On the serial cable this pin will be inverted.
   * This means that if this pin is set to logical 1, then the electrical state on the serial
   * cable will be driven to low state, regardless what the input is
   * (e.i. output from disk drive VIA1 PB3).
   */

  if(g_serial_port_output & MASK_VIA1_PORTB_CLK_OUT)
  {
    reg |= MASK_VIA1_PORTB_CLK_IN;
  }
  else
  {
    /*
     * However, if it is set to logical 0, then the input from computer will instead decide
     * the electrical state for clock bus. The input pin from computer is also inverted, so
     * if this is set to a logical 1, then the bus will get low electrical state, and vice versa.
     * Unlike the computer, both the clock input pin (VIA1 PB2) and data input pin on disk drive
     * are inverted.
     */
    if(g_serial_port_input & 0x10)
    {
      reg |= MASK_VIA1_PORTB_CLK_IN;
    }
    else
    {
      reg &= ~MASK_VIA1_PORTB_CLK_IN;
    }
  }

  /* The exact same reasoning as for the clock bus can be applied for the data bus */
  if(g_serial_port_output & MASK_VIA1_PORTB_DATA_OUT)
  {
    reg |= MASK_VIA1_PORTB_DATA_IN;
  }
  else
  {
    if(g_serial_port_input & 0x20)
    {
      reg |= MASK_VIA1_PORTB_DATA_IN;
    }
    else
    {
      reg &= ~MASK_VIA1_PORTB_DATA_IN;
    }
  }


  /* This line is inveted on both cpu and disk drive! */
  if(g_serial_port_input & 0x08)
  {
    reg |= MASK_VIA1_PORTB_ATN_IN;
  }
  else
  {
    reg &= ~MASK_VIA1_PORTB_ATN_IN;
  }

  return reg;
}

static void check_atn_line()
{
  /* This line is inveted on both cpu and disk drive! */
  if(g_serial_port_input & 0x08)
  {
    g_atn_current = 1;
  }
  else
  {
    g_atn_current = 0;
  }

  /* Check if this is a change */
  if(g_atn_previous == 0 && g_atn_current == 1)
  {
    if(g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE] & MASK_VIA_IRQ_ENABLE_ATN)
    {
      g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] |= MASK_VIA_IRQ_STATUS_IRQ_OCCURED;
      g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] |= MASK_VIA_IRQ_STATUS_ATN;
    }

    /* If auto response is on, then data line should go low */
    if((g_serial_port_output & MASK_VIA1_PORTB_ATNA_OUT) == 0x00)
    {
      g_serial_port_output |= MASK_VIA1_PORTB_DATA_OUT;
      g_if_host.if_host_ports.ports_write_serial_fp(IF_EMU_DEV_DD, g_serial_port_output);
    }
  }

  g_atn_previous = g_atn_current;
}

static uint8_t via1_event_read_irq_enable(uint16_t addr)
{
  return g_memory_dd.all_p[addr] | 0x80; /* Bit 7 always set ? */
}

static uint8_t via1_event_read_porta(uint16_t addr)
{
  /* Reading this register will clear ATN irq flag */
  g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_ATN;

  if(((g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  return g_memory_dd.all_p[addr];
}

static uint8_t via1_event_read_timer_lb(uint16_t addr)
{

  g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_TIMER;

  /* Reading this register will clear timer irq flag */
  if(((g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  return g_memory_dd.all_p[addr];
}

static uint8_t via1_event_read_portb(uint16_t addr)
{
  /* This port is the serial port ! */
  return eval_via1_portb();
}

static void via1_event_write_timer_lb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA1].timer_latch_ap[A] &= 0xFF00;
  *g_via_a[VIA1].timer_latch_ap[A] |= value;
}

static void via1_event_write_timer_hb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA1].timer_latch_ap[A] &= 0x00FF;
  *g_via_a[VIA1].timer_latch_ap[A] |= value << 8;

  /* At this time both latches are transfered to counter */
  *g_via_a[VIA1].timer_ap[A] = *g_via_a[VIA1].timer_latch_ap[A];

  /* Writing this register will clear timer irq flag */
  g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_TIMER;

  if(((g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }
}

static void via1_event_write_timer_latch_ap_lb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA1].timer_latch_ap[A] &= 0xFF00;
  *g_via_a[VIA1].timer_latch_ap[A] |= value;
}

static void via1_event_write_timer_latch_ap_hb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA1].timer_latch_ap[A] &= 0x00FF;
  *g_via_a[VIA1].timer_latch_ap[A] |= value << 8;
}

static void via1_event_write_irq_enable(uint16_t addr, uint8_t value)
{
  if((value & 0x80) == 0x80)  // 1=bits written with 1 will be set
  {
    g_via_a[VIA1].status_irq_mask_reg |= (value & 0x7F);
  }
  else  // 0=bits written with 1 will be cleared
  {
    g_via_a[VIA1].status_irq_mask_reg &= ~(value & 0x7F);
  }

  /* Check if this change will also affect irq status */
  if(((g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  g_memory_dd.all_p[addr] = g_via_a[VIA1].status_irq_mask_reg;
}

static void via1_event_write_porta(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory_dd.all_p[REG_VIA1_DIRECTION_PORTA];
  g_memory_dd.all_p[addr] &= ~mask; /* Clear the bits allowed to write to */
  g_memory_dd.all_p[addr] |= value & mask; /* Write the bits allowed to write to */
}

static void via1_event_write_portb(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory_dd.all_p[REG_VIA1_DIRECTION_PORTB];
  g_serial_port_output &= ~mask; /* Clear the bits allowed to write to */
  g_serial_port_output |= value & mask; /* Write the bits allowed to write to */

  /* This port is the serial port, so better inform cc about any change */
  g_if_host.if_host_ports.ports_write_serial_fp(IF_EMU_DEV_DD, g_serial_port_output &
                                                             (MASK_VIA1_PORTB_DATA_OUT |
                                                              MASK_VIA1_PORTB_CLK_OUT));
}

static void via1_event_write_irq_status(uint16_t addr, uint8_t value)
{
  g_memory_dd.all_p[addr] &= ~(value & 0x7F);

  if(((g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA1_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }
}

static uint8_t via2_event_read_irq_enable(uint16_t addr)
{
  return g_memory_dd.all_p[addr] | 0x80; /* Bit 7 always set ? */
}

static uint8_t via2_event_read_porta(uint16_t addr)
{
  /* Reading this register will clear ATN irq flag */
  g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_ATN;

  if(((g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA2_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  return g_memory_dd.all_p[addr];
}

static uint8_t via2_event_read_timer_lb(uint16_t addr)
{
  /* Reading this register will clear timer irq flag */
  g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_TIMER;

  if(((g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA2_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  return g_memory_dd.all_p[addr];
}

static void via2_event_write_timer_lb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA2].timer_latch_ap[A] &= 0xFF00;
  *g_via_a[VIA2].timer_latch_ap[A] |= value;
}

static void via2_event_write_timer_hb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA2].timer_latch_ap[A] &= 0x00FF;
  *g_via_a[VIA2].timer_latch_ap[A] |= value << 8;

  /* At this time both latches are transfered to counter */
  *g_via_a[VIA2].timer_ap[A] = *g_via_a[VIA2].timer_latch_ap[A];

  /* Writing this register will clear timer irq flag */
  g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_TIMER;

  if(((g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA2_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }
}

static void via2_event_write_timer_latch_ap_lb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA2].timer_latch_ap[A] &= 0xFF00;
  *g_via_a[VIA2].timer_latch_ap[A] |= value;
}

static void via2_event_write_timer_latch_ap_hb(uint16_t addr, uint8_t value)
{
  *g_via_a[VIA2].timer_latch_ap[A] &= 0x00FF;
  *g_via_a[VIA2].timer_latch_ap[A] |= value << 8;
}

static void via2_event_write_irq_enable(uint16_t addr, uint8_t value)
{
  if((value & 0x80) == 0x80)  // 1=bits written with 1 will be set
  {
    g_via_a[VIA2].status_irq_mask_reg |= (value & 0x7F);
  }
  else  // 0=bits written with 1 will be cleared
  {
    g_via_a[VIA2].status_irq_mask_reg &= ~(value & 0x7F);
  }

  /* Check if this change will also affect irq status */
  if(((g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA2_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }

  g_memory_dd.all_p[addr] = g_via_a[VIA2].status_irq_mask_reg;
}

static void via2_event_write_porta(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory_dd.all_p[REG_VIA2_DIRECTION_PORTA];
  g_memory_dd.all_p[addr] &= ~mask; /* Clear the bits allowed to write to */
  g_memory_dd.all_p[addr] |= value & mask; /* Write the bits allowed to write to */
}

static void via2_event_write_portb(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory_dd.all_p[REG_VIA2_DIRECTION_PORTB];

  /* Update if led is changed */
  if((value & 0x08) != (g_memory_dd.all_p[addr] & 0x08))
  {
    g_if_host.if_host_stats.stats_led_fp(value & 0x08);
  }

  g_memory_dd.all_p[addr] &= ~mask; /* Clear the bits allowed to write to */
  g_memory_dd.all_p[addr] |= value & mask; /* Write the bits allowed to write to */
}

static void via2_event_write_irq_status(uint16_t addr, uint8_t value)
{
  g_memory_dd.all_p[addr] &= ~(value & 0x7F);

  if(((g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] & g_memory_dd.all_p[REG_VIA2_IRQ_ENABLE]) & 0x7F) == 0x00)
  {
    g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] &= ~MASK_VIA_IRQ_STATUS_IRQ_OCCURED; /* Reset bit, i.e no irq */
  }
}

void via_init()
{
  uint32_t i; /* via -> 1 and 2 */
  uint32_t j; /* port, timer etc -> A and B */

  memset(g_via_a, 0, sizeof(via_t) * 2);

  g_cycles_in_queue = 0;

  for(i = 0; i < 2; i++)
  {
    for(j = 0; j < 1; j++)
    {
      g_via_a[i].timer_ap[j] = (uint16_t *)(g_memory_dd.all_p + via_reg_timer_low[i]);
      g_via_a[i].timer_latch_ap[j] = (uint16_t *)(g_memory_dd.all_p + via_reg_timer_latch_ap_low[i]);
    }
  }

  g_atn_previous = 1;
  g_atn_current = 1;

  /* Default irq values */
  g_memory_dd.all_p[REG_VIA1_IRQ_STATUS] = 0x00;
  g_memory_dd.all_p[REG_VIA2_IRQ_STATUS] = 0x00;

  /* Default port direction values */
  g_memory_dd.all_p[REG_VIA1_DIRECTION_PORTA] = 0xFF;
  g_memory_dd.all_p[REG_VIA1_DIRECTION_PORTB] = 0x1A;
  g_memory_dd.all_p[REG_VIA2_DIRECTION_PORTA] = 0xFF;
  g_memory_dd.all_p[REG_VIA2_DIRECTION_PORTB] = 0x6F;

  /* Default port value */
  g_memory_dd.all_p[REG_VIA1_DATA_PORTA] = 0x00;
  g_memory_dd.all_p[REG_VIA1_DATA_PORTB] = 0x00;
  g_memory_dd.all_p[REG_VIA2_DATA_PORTA] = 0x00;
  g_memory_dd.all_p[REG_VIA2_DATA_PORTB] = 0x00;

  g_memory_dd.all_p[REG_VIA1_DATA_PORTB] &= ~0x60; /* Set to device number 8 */
  g_memory_dd.all_p[REG_VIA2_DATA_PORTB] |= 0x10; /* Set not write protected */
  g_memory_dd.all_p[REG_VIA2_DATA_PORTB] &= ~0x80; /* Data bytes being currently read */
  g_memory_dd.all_p[REG_VIA2_PERIPHERAL_CTRL] |= 0x01; /* Data bytes being currently read */

  /*
   * Unclear what these registers will be if read before written
   * to, set them to random for now.
   */

  *g_via_a[VIA1].timer_ap[A] = g_if_host.if_host_rand.rand_get_fp();
  *g_via_a[VIA2].timer_ap[A] = g_if_host.if_host_rand.rand_get_fp();

  bus_dd_event_write_subscribe(REG_VIA1_TIMER_LOW_BYTE, via1_event_write_timer_lb);
  bus_dd_event_write_subscribe(REG_VIA1_TIMER_HIGH_BYTE, via1_event_write_timer_hb);
  bus_dd_event_write_subscribe(REG_VIA1_TIMER_LATCH_LOW_BYTE, via1_event_write_timer_latch_ap_lb);
  bus_dd_event_write_subscribe(REG_VIA1_TIMER_LATCH_HIGH_BYTE, via1_event_write_timer_latch_ap_hb);
  bus_dd_event_write_subscribe(REG_VIA1_IRQ_ENABLE, via1_event_write_irq_enable);
  bus_dd_event_write_subscribe(REG_VIA1_DATA_PORTA, via1_event_write_porta);
  bus_dd_event_write_subscribe(REG_VIA1_DATA_PORTB, via1_event_write_portb);
  bus_dd_event_write_subscribe(REG_VIA1_IRQ_STATUS, via1_event_write_irq_status);
  
  bus_dd_event_read_subscribe(REG_VIA1_IRQ_ENABLE, via1_event_read_irq_enable);
  bus_dd_event_read_subscribe(REG_VIA1_DATA_PORTA, via1_event_read_porta);
  bus_dd_event_read_subscribe(REG_VIA1_TIMER_LOW_BYTE, via1_event_read_timer_lb);
  bus_dd_event_read_subscribe(REG_VIA1_DATA_PORTB, via1_event_read_portb);

  bus_dd_event_write_subscribe(REG_VIA2_TIMER_LOW_BYTE, via2_event_write_timer_lb);
  bus_dd_event_write_subscribe(REG_VIA2_TIMER_HIGH_BYTE, via2_event_write_timer_hb);
  bus_dd_event_write_subscribe(REG_VIA2_TIMER_LATCH_LOW_BYTE, via2_event_write_timer_latch_ap_lb);
  bus_dd_event_write_subscribe(REG_VIA2_TIMER_LATCH_HIGH_BYTE, via2_event_write_timer_latch_ap_hb);
  bus_dd_event_write_subscribe(REG_VIA2_IRQ_ENABLE, via2_event_write_irq_enable);
  bus_dd_event_write_subscribe(REG_VIA2_DATA_PORTA, via2_event_write_porta);
  bus_dd_event_write_subscribe(REG_VIA2_DATA_PORTB, via2_event_write_portb);
  bus_dd_event_write_subscribe(REG_VIA2_IRQ_STATUS, via2_event_write_irq_status);

  bus_dd_event_read_subscribe(REG_VIA2_IRQ_ENABLE, via2_event_read_irq_enable);
  bus_dd_event_read_subscribe(REG_VIA2_DATA_PORTA, via2_event_read_porta);
  bus_dd_event_read_subscribe(REG_VIA2_TIMER_LOW_BYTE, via2_event_read_timer_lb);
}

void via_step(uint32_t cc)
{
  g_cycles_in_queue += cc;
  uint32_t i; /* via */
  uint32_t j; /* a or b, b not needed atm */

  if(g_cycles_in_queue >= MIN_QUEUE_SIZE)
  {
    g_cycles_in_queue -= MIN_QUEUE_SIZE;

    for(i = 0; i < 2; i++)
    {
      for(j = 0; j < 1; j++)
      {
        /* Will this timer underflow? */
        if(*g_via_a[i].timer_ap[j] <= MIN_QUEUE_SIZE)
        {
          *g_via_a[i].timer_ap[j] = 0;

          if((g_via_a[i].status_control_reg[j] & MASK_VIA_AUXILIARY_TIMER_CTRL) == ONE_SHOT_NO_PB7) /* Single shot */
          {
            /* In one shot mode the interrupt needs to be cleared in order to get another one */
            if((g_memory_dd.all_p[via_reg_int[i]] & MASK_VIA_IRQ_STATUS_TIMER) == 0x00)
            {
              /* Is the interrupt enabled? */
              if(g_via_a[i].status_irq_mask_reg & via_int_mask[j]) /* Interrupt enable? */
              {
                /* Set flag about what caused IRQ and raise IRQ */
                g_memory_dd.all_p[via_reg_int[i]] = via_int_reason_and_msb[j];
              }
              else
              {
                /* Report underrun, since the interrupt is not enabled bit 7 should not be set */
                g_memory_dd.all_p[via_reg_int[i]] = via_int_reason[j];
              }
            }

            /* Timer will continue to decrement in this mode */
            ;//*g_via_a[i].timer_ap[j] = 0xFFFF;
          }
          else /* Contineuous */
          {
            /* Is the interrupt enabled? */
            if(g_via_a[i].status_irq_mask_reg & via_int_mask[j]) /* Interrupt enable? */
            {
              /* Set flag about what caused IRQ and raise IRQ */
              g_memory_dd.all_p[via_reg_int[i]] = via_int_reason_and_msb[j];
            }
            else
            {
              /* Report underrun, since the interrupt is not enabled bit 7 should not be set */
              g_memory_dd.all_p[via_reg_int[i]] = via_int_reason[j];
            }

            *g_via_a[i].timer_ap[j] = *g_via_a[i].timer_latch_ap[j];
          }
        }
        else
        {
          /* Counter will continue to decrement. No start/stop exist */
          *g_via_a[i].timer_ap[j] -= MIN_QUEUE_SIZE;
        }
      }
    }
  }
}

void via_serial_port_activity(uint8_t data)
{
  /*
  * Everytime the serial port is written by other end, this function will
  * be called containing the changed value.
  */

  g_serial_port_input = data;

  check_atn_line();
}
