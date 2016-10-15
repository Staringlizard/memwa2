/*
 * memwa2 sidbus driver
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
 * Responsible for the connection with sid hw.
 */

#include "sidbus.h"
#include "stm32f7xx_hal_gpio.h"
#include "config.h"
#include "rng.h"

sidbus_state_t g_sidbus_state;

__weak void HAL_SIDBUS_MspInit()
{
  ;
}

void sidbus_irq()
{
    switch(g_sidbus_state)
    {
      case SIDBUS_STATE_ACTIVATE_CHIP:
        SID_SET_CS_LOW();
        g_sidbus_state = SIDBUS_STATE_DATA_SENT;
        break;
      case SIDBUS_STATE_DATA_SENT:
        SID_SET_CS_HIGH();

        /* Disable sidbus IRQ */
        EXTI->FTSR &= ~GPIO_PIN_13;
        g_sidbus_state = SIDBUS_STATE_ACTIVATE_CHIP;
        break;
    }
}

void sidbus_init()
{
    HAL_SIDBUS_MspInit();
    SID_SET_CS_HIGH();

    g_sidbus_state = SIDBUS_STATE_ACTIVATE_CHIP;
}

void sidbus_write(uint8_t addr, uint8_t value)
{
    SID_ASSERT_WRITE();
    SID_SET_ADDR(addr);
    SID_SET_DATA(value);

    /* Enable sidbus IRQ */
    EXTI->FTSR |= GPIO_PIN_13;
}

uint8_t sidbus_read(uint8_t addr)
{
  /* Just return random value for now */
  return rng_get();
}
