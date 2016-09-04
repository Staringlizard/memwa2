/*
 * memwa2 cia component
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
 * CIA implementation (MOS6526).
 * The functionality is far from complete, but contains
 * the most important features.
 */

#include "bus.h"
#include "cia.h"
#include "cpu.h"
#include "if.h"
#include "vic.h"
#include "key.h"
#include "joy.h"
#include "tap.h"
#include <string.h>

#define CIA1 0
#define CIA2 1

#define A    0
#define B    1

/* Much higher than this and many tapes stop working */
#define MIN_QUEUE_SIZE  0x20

static uint8_t cia1_event_read_irq_ctrl(uint16_t addr);
static uint8_t cia1_event_read_porta(uint16_t addr);
static uint8_t cia1_event_read_portb(uint16_t addr);
static uint8_t cia1_event_read_ctrla(uint16_t addr);
static void cia1_event_write_porta(uint16_t addr, uint8_t value);
static void cia1_event_write_portb(uint16_t addr, uint8_t value);
static void cia1_event_write_dira(uint16_t addr, uint8_t value);
static void cia1_event_write_dirb(uint16_t addr, uint8_t value);
static void cia1_event_write_ctrla(uint16_t addr, uint8_t value);
static void cia1_event_write_ctrlb(uint16_t addr, uint8_t value);
static void cia1_event_write_irq_ctrl(uint16_t addr, uint8_t value);
static void cia1_event_write_timera_hb(uint16_t addr, uint8_t value);
static void cia1_event_write_timera_lb(uint16_t addr, uint8_t value);
static void cia1_event_write_timerb_hb(uint16_t addr, uint8_t value);
static void cia1_event_write_timerb_lb(uint16_t addr, uint8_t value);
static void cia1_event_write_tod_tenth(uint16_t addr, uint8_t value);
static uint8_t cia2_event_read_irq_ctrl(uint16_t addr);
static void cia2_event_write_porta(uint16_t addr, uint8_t value);
static void cia2_event_write_portb(uint16_t addr, uint8_t value);
static void cia2_event_write_dira(uint16_t addr, uint8_t value);
static void cia2_event_write_dirb(uint16_t addr, uint8_t value);
static void cia2_event_write_ctrla(uint16_t addr, uint8_t value);
static void cia2_event_write_ctrlb(uint16_t addr, uint8_t value);
static void cia2_event_write_irq_ctrl(uint16_t addr, uint8_t value);
static void cia2_event_write_timera_hb(uint16_t addr, uint8_t value);
static void cia2_event_write_timera_lb(uint16_t addr, uint8_t value);
static void cia2_event_write_timerb_hb(uint16_t addr, uint8_t value);
static void cia2_event_write_timerb_lb(uint16_t addr, uint8_t value);
static void cia2_event_write_tod_tenth(uint16_t addr, uint8_t value);

typedef struct
{
  uint32_t timer_latch[2];
  uint32_t status_control_reg[2];
  uint32_t status_irq_mask_reg;
  uint16_t *timer[2];
  uint8_t tod_started;
} cia_t;

/* Contains matrix of pushed keys, key.c is responsible for this memory */
extern uint8_t g_matrix_active_aa[8][8];

/* Contains matrix of joystick status, key.c is responsible for this memory */
extern joy_action_state_t g_joy_action_state_aa[2][5];

extern memory_t g_memory; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

static cia_t g_cia_a[2];
static uint32_t g_cycles_in_queue;
static uint8_t g_tenth_second;
static uint8_t g_seconds;
static uint8_t g_minutes;
static uint8_t g_hours;
static uint8_t g_serial_port_input;
static uint8_t g_serial_port_output;

static uint16_t cia_reg_timer_low[2][2] =
{
  {REG_CIA1_TIMER_A_LOW_BYTE, REG_CIA1_TIMER_B_LOW_BYTE},
  {REG_CIA2_TIMER_A_LOW_BYTE, REG_CIA2_TIMER_B_LOW_BYTE}
};

static uint16_t cia_reg_ctrl[2][2] =
{
  {REG_CIA1_CONTROL_A, REG_CIA1_CONTROL_B},
  {REG_CIA2_CONTROL_A, REG_CIA2_CONTROL_B}
};

static uint16_t cia_reg_int[2] =
{
  REG_CIA1_INTERRUPT_CONTROL_REG,
  REG_CIA2_INTERRUPT_CONTROL_REG
};

static uint8_t cia_int_reason_and_msb[2] =
{
  0x81,
  0x82
};

static uint8_t cia_int_reason[2] =
{
  0x1,
  0x2
};

static uint8_t cia_int_mask[2] =
{
  MASK_CIA_INTERRUPT_CONTROL_REG_TIMER_A,
  MASK_CIA_INTERRUPT_CONTROL_REG_TIMER_B
};

static uint8_t eval_cia2_porta()
{
  uint8_t reg = g_serial_port_output;

  /*
   * The bus (serial port) between computer and disk drive is pretty complicated.
   * Hw schematics are necessary to completely understand this.
   */

  /*
   * The computer (cc) is driving the clock bus with CIA2 PA4. This pin is isolated and is
   * not affected by the input from disk drive. On the serial cable this pin will be inverted.
   * This means that if this pin is set to logical 1, then the electrical state on the serial
   * cable will be driven to low state, regardless what the input is
   * (e.i. output from disk drive VIA1 PB3).
   */
  if(g_serial_port_output & MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_OUTPUT)
  {
    reg &= ~MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_INPUT;
  }
  else
  {
    /*
     * However, if it is set to logical 0, then the input from disk drive will instead decide
     * the electrical state for clock bus. The input pin from disk drive is also inverted, so
     * if this is set to a logical 1, then the bus will get low electrical state, and vice versa.
     */
    if(g_serial_port_input & 0x08)
    {
      reg &= ~MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_INPUT;
    }
    else
    {
      reg |= MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_INPUT;
    }
  }

  /* The exact same reasoning as for the clock bus can be applied for the data bus */
  if(g_serial_port_output & MASK_CIA2_DATA_PORTA_SB_DATA_OUTPUT)
  {
    reg &= ~MASK_CIA2_DATA_PORTA_SB_DATA_INPUT;
  }
  else
  {
    if(g_serial_port_input & 0x02)
    {
      reg &= ~MASK_CIA2_DATA_PORTA_SB_DATA_INPUT;
    }
    else
    {
      reg |= MASK_CIA2_DATA_PORTA_SB_DATA_INPUT;
    }
  }

  /*
   * Now the input pins from computer (CIA2 PA6 and PA7) and the disk drive (VIA1 PB0 and PB2)
   * have been evaluated. Just add the vic port pins to the result and the final result can be
   * returned.
   */
  return reg;
}

static uint8_t eval_cia1_portb()
{
  /*
   * What is coming out from port B has to do with what key is pressed
   * and what is set on port A.
   */

  /* First save the current value for port b */
  uint8_t reg_result = g_memory.io_p[REG_CIA1_DATA_PORT_B];

  uint32_t i; /* col */
  uint32_t j; /* row */

  /* Go through the matrix to see if button is pressed */
  for(i = 0; i < 8; i++)
  {
    for(j = 0; j < 8; j++)
    {
      /* If button pressed and the corresponding bit in port a is unset ... */
      if(g_matrix_active_aa[i][j] &&
         !(g_memory.io_p[REG_CIA1_DATA_PORT_A] & (1 << j)))
      {
        /* ... then also unset resulting register bit in portb */
        reg_result &= ~(1 << i);
      }
    }
  }

  if(g_joy_action_state_aa[JOY_PORT_A][JOY_ACTION_UP] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_A_UP_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_A][JOY_ACTION_DOWN] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_A_DOWN_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_A][JOY_ACTION_RIGHT] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_A_RIGHT_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_A][JOY_ACTION_LEFT] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_A_LEFT_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_A][JOY_ACTION_FIRE] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_A_FIRE_PORT_VAL;
  }

  return reg_result;
}

static uint8_t eval_cia1_porta()
{
  /*
   * What is coming out from port A has to do with what key is pressed
   * and what is set on port B.
   */

  /* First save the current value for port b */
  uint8_t reg_result = g_memory.io_p[REG_CIA1_DATA_PORT_A];

  uint32_t i; /* col */
  uint32_t j; /* row */

  /* Go through the matrix to see if button is pressed */
  for(i = 0; i < 8; i++)
  {
    for(j = 0; j < 8; j++)
    {
      /* If button pressed and the corresponding bit in port a is unset ... */
      if(g_matrix_active_aa[i][j] &&
         !(g_memory.io_p[REG_CIA1_DATA_PORT_B] & (1 << j)))
      {
        /* ... then also unset resulting register bit in portb */
        reg_result &= ~(1 << i);
      }
    }
  }

  if(g_joy_action_state_aa[JOY_PORT_B][JOY_ACTION_UP] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_B_UP_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_B][JOY_ACTION_DOWN] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_B_DOWN_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_B][JOY_ACTION_RIGHT] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_B_RIGHT_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_B][JOY_ACTION_LEFT] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_B_LEFT_PORT_VAL;
  }
  if(g_joy_action_state_aa[JOY_PORT_B][JOY_ACTION_FIRE] == JOY_ACTION_STATE_PRESSED)
  {
    reg_result &= JOY_B_FIRE_PORT_VAL;
  }

  return reg_result;
}


static uint8_t cia1_event_read_irq_ctrl(uint16_t addr)
{
  uint8_t reg = g_memory.io_p[addr];
  /* This reg is cleared when read */
  g_memory.io_p[addr] = 0;
  return reg;
}

static uint8_t cia1_event_read_porta(uint16_t addr)
{
  return eval_cia1_porta();
}

static uint8_t cia1_event_read_portb(uint16_t addr)
{
  return eval_cia1_portb();
}

static uint8_t cia1_event_read_ctrla(uint16_t addr)
{
  return g_memory.io_p[addr] & ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
}

static void cia1_event_write_porta(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory.io_p[REG_CIA1_DATA_DIRECTION_A];
  g_memory.io_p[addr] = ~mask; /* Dont forget the internal pull ups for input pins */
  g_memory.io_p[addr] |= value & mask; /* Set the bits allowed to write to */
}

static void cia1_event_write_portb(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory.io_p[REG_CIA1_DATA_DIRECTION_B];
  g_memory.io_p[addr] = ~mask; /* Dont forget the internal pull ups for input pins */
  g_memory.io_p[addr] |= value & mask; /* Set the bits allowed to write to */
}

static void cia1_event_write_dira(uint16_t addr, uint8_t value)
{
  g_memory.io_p[addr] = value;

  /* This will affect the port value due to internal pull up */
  g_memory.io_p[REG_CIA1_DATA_PORT_A] |= ~value;
}

static void cia1_event_write_dirb(uint16_t addr, uint8_t value)
{
  g_memory.io_p[addr] = value;

  /* This will affect the port value due to internal pull up */
  g_memory.io_p[REG_CIA1_DATA_PORT_B] |= ~value;
}

static void cia1_event_write_ctrla(uint16_t addr, uint8_t value)
{
  /*
   * A strobe bit allows the timer latch to be loaded
   * into the timer counter at any time, whetherthe timer
   * is running or not.
   */
  if(value & MASK_CIA_CONTROL_FORCE_LOAD_TIMER)
  {
    *g_cia_a[CIA1].timer[A] = g_cia_a[CIA1].timer_latch[A];
    value &= ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
  }
  g_cia_a[CIA1].status_control_reg[A] = value;

  g_memory.io_p[addr] = value;
}

static void cia1_event_write_ctrlb(uint16_t addr, uint8_t value)
{
  /*
   * A strobe bit allows the timer latch to be loaded
   * into the timer counter at any time, whetherthe timer
   * is running or not.
   */
  if(value & MASK_CIA_CONTROL_FORCE_LOAD_TIMER)
  {
    *g_cia_a[CIA1].timer[B] = g_cia_a[CIA1].timer_latch[B];
    value &= ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
  }
  g_cia_a[CIA1].status_control_reg[B] = value;

  g_memory.io_p[addr] = value;
}

static void cia1_event_write_irq_ctrl(uint16_t addr, uint8_t value)
{
  if((value & 0x80) == 0x80)  /* If MSB set then bits written will be set as normal */
  {
    g_cia_a[CIA1].status_irq_mask_reg |= (value & 0x7F);
  }
  else  /* If MSB not set then bits written will be reset */
  {
    g_cia_a[CIA1].status_irq_mask_reg &= ~(value & 0x7F);
  }

  /* This register is in hw, no memory involved */
}

static void cia1_event_write_timera_hb(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA1].timer_latch[A] &= 0x00FF;
  g_cia_a[CIA1].timer_latch[A] |= value << 8;
  if((g_cia_a[CIA1].status_control_reg[A] & MASK_CIA_CONTROL_START_STOP_TIMER) == 0x00)
  {
    *g_cia_a[CIA1].timer[A] = g_cia_a[CIA1].timer_latch[A];
  }
}

static void cia1_event_write_timera_lb(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA1].timer_latch[A] &= 0xFF00;
  g_cia_a[CIA1].timer_latch[A] |= value;
}

static void cia1_event_write_timerb_hb(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA1].timer_latch[B] &= 0x00FF;
  g_cia_a[CIA1].timer_latch[B] |= value << 8;
  if((g_cia_a[CIA1].status_control_reg[B] & MASK_CIA_CONTROL_START_STOP_TIMER) == 0x00)
  {
    *g_cia_a[CIA1].timer[B] = g_cia_a[CIA1].timer_latch[B];
  }
}

static void cia1_event_write_timerb_lb(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA1].timer_latch[B] &= 0xFF00;
  g_cia_a[CIA1].timer_latch[B] |= value;
}

static void cia1_event_write_tod_tenth(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA1].tod_started = 1;
}

static uint8_t cia2_event_read_porta(uint16_t addr)
{
  /* This port is the serial port ! */
  return eval_cia2_porta();
}

static uint8_t cia2_event_read_irq_ctrl(uint16_t addr)
{
  uint8_t reg = g_memory.io_p[addr];
  /* This reg is cleared when read */
  g_memory.io_p[addr] = 0;
  cpu_clear_nmi(); /* Tell cpu that the nmi line is high again */
  return reg;
}

static uint8_t cia2_event_read_ctrla(uint16_t addr)
{
  return g_memory.io_p[addr] & ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
}

static void cia2_event_write_porta(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory.io_p[REG_CIA2_DATA_DIRECTION_A];
  g_serial_port_output = ~mask; /* Dont forget the internal pull ups for input pins */
  g_serial_port_output |= value & mask; /* Set the bits allowed to write to */

  /* Inform vic as well! */
  vic_set_bank(g_serial_port_output & MASK_CIA2_DATA_PORTA_VIC_CHIP_BANK);

  /* This port is the serial port, so better inform dd about any change */
  g_if_host.if_host_ports.ports_write_serial_fp(IF_EMU_DEV_CC, g_serial_port_output &
                                                  (MASK_CIA2_DATA_PORTA_SB_DATA_OUTPUT |
                                                  MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_OUTPUT |
                                                  MASK_CIA2_DATA_PORTA_SB_ATN_SIGNAL_OUTPUT));

  /* This register is not connected to any memory */
}

static void cia2_event_write_portb(uint16_t addr, uint8_t value)
{
  uint8_t mask = g_memory.io_p[REG_CIA2_DATA_DIRECTION_B];
  g_memory.io_p[addr] = ~mask; /* Dont forget the internal pull ups for input pins */
  g_memory.io_p[addr] |= value & mask; /* Set the bits allowed to write to */
}

static void cia2_event_write_dira(uint16_t addr, uint8_t value)
{
  g_memory.io_p[addr] = value;

  /* This will affect the port value due to internal pull up */
  g_memory.io_p[REG_CIA2_DATA_PORT_A] |= ~value;
}

static void cia2_event_write_dirb(uint16_t addr, uint8_t value)
{
  g_memory.io_p[addr] = value;

  /* This will affect the port value due to internal pull up */
  g_memory.io_p[REG_CIA2_DATA_PORT_B] |= ~value;
}

static void cia2_event_write_ctrla(uint16_t addr, uint8_t value)
{
  /*
   * A strobe bit allows the timer latch to be loaded
   * into the timer counter at any time, whetherthe timer
   * is running or not.
   */
  if(value & MASK_CIA_CONTROL_FORCE_LOAD_TIMER)
  {
    *g_cia_a[CIA2].timer[A] = g_cia_a[CIA2].timer_latch[A];
    value &= ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
  }
  g_cia_a[CIA2].status_control_reg[A] = value;

  g_memory.io_p[addr] = value;
}

static void cia2_event_write_ctrlb(uint16_t addr, uint8_t value)
{
  /*
   * A strobe bit allows the timer latch to be loaded
   * into the timer counter at any time, whetherthe timer
   * is running or not.
   */
  if(value & MASK_CIA_CONTROL_FORCE_LOAD_TIMER)
  {
    *g_cia_a[CIA2].timer[B] = g_cia_a[CIA2].timer_latch[B];
    value &= ~MASK_CIA_CONTROL_FORCE_LOAD_TIMER;
  }
  g_cia_a[CIA2].status_control_reg[B] = value;

  g_memory.io_p[addr] = value;
}

static void cia2_event_write_irq_ctrl(uint16_t addr, uint8_t value)
{
  if((value & 0x80) == 0x80) /* 1=bits written with 1 will be set */
  {
    g_cia_a[CIA2].status_irq_mask_reg |= (value & 0x7F);
  }
  else  /* 0=bits written with 1 will be cleared */
  {
    g_cia_a[CIA2].status_irq_mask_reg &= ~(value & 0x7F);
  }

  /* This register is in hw, no memory involved */
}

static void cia2_event_write_timera_hb(uint16_t addr, uint8_t value)
{
  /* Latch the value into the timer latch */
  g_cia_a[CIA2].timer_latch[A] &= 0x00FF;
  g_cia_a[CIA2].timer_latch[A] |= value << 8;
  if((g_cia_a[CIA2].status_control_reg[A] & MASK_CIA_CONTROL_START_STOP_TIMER) == 0x00)
  {
    *g_cia_a[CIA2].timer[A] = g_cia_a[CIA2].timer_latch[A];
  }
}

static void cia2_event_write_timera_lb(uint16_t addr, uint8_t value)
{
  /* Latch the value into the timer latch */
  g_cia_a[CIA2].timer_latch[A] &= 0xFF00;
  g_cia_a[CIA2].timer_latch[A] |= value;
}

static void cia2_event_write_timerb_hb(uint16_t addr, uint8_t value)
{
  /* Latch the value into the timer latch */
  g_cia_a[CIA2].timer_latch[B] &= 0x00FF;
  g_cia_a[CIA2].timer_latch[B] |= value << 8;
  if((g_cia_a[CIA2].status_control_reg[B] & MASK_CIA_CONTROL_START_STOP_TIMER) == 0x00)
  {
    *g_cia_a[CIA2].timer[B] = g_cia_a[CIA2].timer_latch[B];
  }
}

static void cia2_event_write_timerb_lb(uint16_t addr, uint8_t value)
{
  /* Latch the value into the timer latch */
  g_cia_a[CIA2].timer_latch[B] &= 0xFF00;
  g_cia_a[CIA2].timer_latch[B] |= value;
}

static void cia2_event_write_tod_tenth(uint16_t addr, uint8_t value)
{
  g_cia_a[CIA2].tod_started = 1;
}

static uint32_t dec2bcd(uint32_t num)
{
    uint32_t ones = 0;
    uint32_t tens = 0;
    uint32_t temp = 0;

    ones = num%10;
    temp = num/10;
    tens = temp<<4;
    return (tens + ones);
}

void cia_init()
{
  uint32_t i; /* cia 1 and 2 */
  uint32_t j; /* timer A and B */

  memset(g_cia_a, 0, sizeof(cia_t) * 2);

  g_cycles_in_queue = 0;

  for(i = 0; i < 2; i++)
  {
    for(j = 0; j < 2; j++)
    {
      g_cia_a[i].timer[j] = (uint16_t *)(g_memory.io_p + cia_reg_timer_low[i][j]);
    }
  }

  /* Default values */
  g_memory.io_p[REG_CIA1_DATA_DIRECTION_A] = 0xFF;
  g_memory.io_p[REG_CIA1_DATA_DIRECTION_B] = 0x00;
  g_memory.io_p[REG_CIA2_DATA_DIRECTION_A] = 0x3F;
  g_memory.io_p[REG_CIA2_DATA_DIRECTION_B] = 0x00;

  /* Default port value */
  g_memory.io_p[REG_CIA2_DATA_PORT_A] = 0x03;
  g_memory.io_p[REG_CIA1_TIME_OF_DAY_HOURS] = 0x01;
  vic_set_bank(0x03);

  /*
   * Unclear what these registers will be if read before written
   * to, set them to random value.
   */
  
  *g_cia_a[CIA1].timer[A] = g_if_host.if_host_rand.rand_get_fp();
  *g_cia_a[CIA1].timer[B] = g_if_host.if_host_rand.rand_get_fp();
  *g_cia_a[CIA2].timer[A] = g_if_host.if_host_rand.rand_get_fp();
  *g_cia_a[CIA2].timer[B] = g_if_host.if_host_rand.rand_get_fp();

  bus_event_read_subscribe(REG_CIA1_INTERRUPT_CONTROL_REG, cia1_event_read_irq_ctrl);
  bus_event_read_subscribe(REG_CIA1_DATA_PORT_A, cia1_event_read_porta);
  bus_event_read_subscribe(REG_CIA1_DATA_PORT_B, cia1_event_read_portb);
  bus_event_read_subscribe(REG_CIA1_CONTROL_A, cia1_event_read_ctrla);

  bus_event_write_subscribe(REG_CIA1_DATA_PORT_A, cia1_event_write_porta);
  bus_event_write_subscribe(REG_CIA1_DATA_PORT_B, cia1_event_write_portb);
  bus_event_write_subscribe(REG_CIA1_DATA_DIRECTION_A, cia1_event_write_dira);
  bus_event_write_subscribe(REG_CIA1_DATA_DIRECTION_B, cia1_event_write_dirb);
  bus_event_write_subscribe(REG_CIA1_CONTROL_A, cia1_event_write_ctrla);
  bus_event_write_subscribe(REG_CIA1_CONTROL_B, cia1_event_write_ctrlb);
  bus_event_write_subscribe(REG_CIA1_INTERRUPT_CONTROL_REG, cia1_event_write_irq_ctrl);
  bus_event_write_subscribe(REG_CIA1_TIMER_A_HIGH_BYTE, cia1_event_write_timera_hb);
  bus_event_write_subscribe(REG_CIA1_TIMER_A_LOW_BYTE, cia1_event_write_timera_lb);
  bus_event_write_subscribe(REG_CIA1_TIMER_B_HIGH_BYTE, cia1_event_write_timerb_hb);
  bus_event_write_subscribe(REG_CIA1_TIMER_B_LOW_BYTE, cia1_event_write_timerb_lb);
  bus_event_write_subscribe(REG_CIA1_TIME_OF_DAY_TENTH_SEC, cia1_event_write_tod_tenth);

  bus_event_read_subscribe(REG_CIA2_DATA_PORT_A, cia2_event_read_porta);
  bus_event_read_subscribe(REG_CIA2_INTERRUPT_CONTROL_REG, cia2_event_read_irq_ctrl);
  bus_event_read_subscribe(REG_CIA2_CONTROL_A, cia2_event_read_ctrla);

  bus_event_write_subscribe(REG_CIA2_DATA_PORT_A, cia2_event_write_porta);
  bus_event_write_subscribe(REG_CIA2_DATA_PORT_B, cia2_event_write_portb);
  bus_event_write_subscribe(REG_CIA2_DATA_DIRECTION_A, cia2_event_write_dira);
  bus_event_write_subscribe(REG_CIA2_DATA_DIRECTION_B, cia2_event_write_dirb);
  bus_event_write_subscribe(REG_CIA2_CONTROL_A, cia2_event_write_ctrla);
  bus_event_write_subscribe(REG_CIA2_CONTROL_B, cia2_event_write_ctrlb);
  bus_event_write_subscribe(REG_CIA2_INTERRUPT_CONTROL_REG, cia2_event_write_irq_ctrl);
  bus_event_write_subscribe(REG_CIA2_TIMER_A_HIGH_BYTE, cia2_event_write_timera_hb);
  bus_event_write_subscribe(REG_CIA2_TIMER_A_LOW_BYTE, cia2_event_write_timera_lb);
  bus_event_write_subscribe(REG_CIA2_TIMER_B_HIGH_BYTE, cia2_event_write_timerb_hb);
  bus_event_write_subscribe(REG_CIA2_TIMER_B_LOW_BYTE, cia2_event_write_timerb_lb);
  bus_event_write_subscribe(REG_CIA2_TIME_OF_DAY_TENTH_SEC, cia2_event_write_tod_tenth);
}

void cia_step(uint32_t cc)
{
  uint32_t i; /* cia */
  uint32_t j; /* a or b */
  uint8_t cycles;

  if(tap_get_play()) /* If tape is active then do not use queue */
  {
    cycles = cc;
    g_cycles_in_queue = 0;
  }
  else
  {
    g_cycles_in_queue += cc;

    if(g_cycles_in_queue >= MIN_QUEUE_SIZE)
    {
      g_cycles_in_queue -= MIN_QUEUE_SIZE;
      cycles = MIN_QUEUE_SIZE;
    }
    else
    {
      return;
    }
  }

  for(i = 0; i < 2; i++)
  {
    for(j = 0; j < 2; j++)
    {
      /* First check if the timer is on or not */
      if(g_cia_a[i].status_control_reg[j] & MASK_CIA_CONTROL_START_STOP_TIMER)
      {
        /* Will this timer underflow? */
        if(*g_cia_a[i].timer[j] <= cycles)
        {
          *g_cia_a[i].timer[j] = 0;

          /* Is the interrupt enabled? */
          if(g_cia_a[i].status_irq_mask_reg & cia_int_mask[j]) /* Interrupt enable? */
          {
            if(i == CIA1)
            {
              /* Set flag about what caused IRQ and raise IRQ */
              g_memory.io_p[cia_reg_int[CIA1]] = cia_int_reason_and_msb[j]; /* Set IRQ line low*/
            }
            else
            {
              /* NMI must have been cleared before entering again */
              if(!cpu_get_nmi())
              {
                /* Set flag about what caused IRQ and raise IRQ */
                g_memory.io_p[cia_reg_int[CIA2]] = cia_int_reason_and_msb[j]; /* Set NMI line low*/
              }
            }
          }
          else
          {
            /* Report underrun, since the interrupt is not enabled bit 7 should not be set */
            g_memory.io_p[cia_reg_int[i]] = cia_int_reason[j];
          }

          if(g_cia_a[i].status_control_reg[j] & MASK_CIA_CONTROL_RUN_MODE) /* Single shot */
          {
            /*
             * In one-shot mode, the timer will count down from the latched value
             * to zero, generate an interrupt, reload the latched value,
             * then stop.
             */
            *g_cia_a[i].timer[j] = g_cia_a[i].timer_latch[j];
            g_memory.io_p[cia_reg_ctrl[i][j]] &= ~MASK_CIA_CONTROL_START_STOP_TIMER;
            g_cia_a[i].status_control_reg[j] &= ~MASK_CIA_CONTROL_START_STOP_TIMER;

          }
          else /* Contineuous */
          {
            /* In continous mode just reload the latch and continue */
            *g_cia_a[i].timer[j] = g_cia_a[i].timer_latch[j];
          }
        }
        else
        {
          *g_cia_a[i].timer[j] -= cycles;
        }
      }
    }
  }
}

void cia_request_irq_tape()
{
  /* Set reason for interrupt */
  g_memory.io_p[REG_CIA1_INTERRUPT_CONTROL_REG] |= MASK_CIA_INTERRUPT_CONTROL_REG_FLAG1;
  if(g_cia_a[CIA1].status_irq_mask_reg & MASK_CIA_INTERRUPT_CONTROL_REG_FLAG1) /* Interrupt enable? */
  {
    /* Set IRQ line low*/
    g_memory.io_p[REG_CIA1_INTERRUPT_CONTROL_REG] |= MASK_CIA_INTERRUPT_CONTROL_REG_MULTI;
  }
}

void cia_tenth_second()
{
  /*
   * The Time of Day Clock on CIA #1 (see
   * 56328-56331 ($DC08-$DC0B)) does not start running until you write to
   * the tenth of second register. The Operating System never starts this
   * clock, and therefore the two registers used as part of the floating
   * point RND(0) value always have a value of 0
   */
  g_tenth_second++;

  if(g_tenth_second == 10)
  {
    g_seconds++;
    g_tenth_second = 0;
  }

  if(g_seconds == 60)
  {
    g_minutes++;
    g_seconds = 0;
  }

  if(g_minutes == 60)
  {
    g_hours++;
    g_minutes = 0;
  }

  if(g_hours == 24)
  {
    g_hours = 0;
  }

  /*
   * Unlike most host-emu interface functionality, this function can be called
   * before emulator has been initialized. This is why NULL check is needed.
   */
  if(g_memory.io_p != NULL)
  {
    if(g_cia_a[CIA1].tod_started)
    {
      g_memory.io_p[REG_CIA1_TIME_OF_DAY_TENTH_SEC] = g_tenth_second;
      g_memory.io_p[REG_CIA1_TIME_OF_DAY_SEC] = dec2bcd(g_seconds);
      g_memory.io_p[REG_CIA1_TIME_OF_DAY_MIN] = dec2bcd(g_minutes);
      g_memory.io_p[REG_CIA1_TIME_OF_DAY_HOURS] = dec2bcd(g_hours);
    }

    if(g_cia_a[CIA2].tod_started)
    {
      g_memory.io_p[REG_CIA2_TIME_OF_DAY_TENTH_SEC] = g_tenth_second;
      g_memory.io_p[REG_CIA2_TIME_OF_DAY_SEC] = dec2bcd(g_seconds);
      g_memory.io_p[REG_CIA2_TIME_OF_DAY_MIN] = dec2bcd(g_minutes);
      g_memory.io_p[REG_CIA2_TIME_OF_DAY_HOURS] = dec2bcd(g_hours);
    }
  }
}

void cia_serial_port_activity(uint8_t data)
{
  /*
   * Everytime the serial port is written by other end, this function will
   * be called containing the changed value.
   */
  g_serial_port_input = data;
}
