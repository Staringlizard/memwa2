/*
 * memwa2 adv7511w driver
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
 * Handles all communication with the HDMI transmitter.
 */

#include "adv7511.h"
#include "config.h"
#include "stm32f7xx_hal_i2c.h"

#define I2C_ADDRESS      0x72
#define I2C_OWN_ADDRESS  0x00
#define I2C_TIMING       0x40912732

#define ADV7511_REG_IRQ    0x96
#define ADV7511_REG_STATUS 0xC8
#define ADV7511_REG_DETECT 0x42

static I2C_HandleTypeDef g_i2c_handle;

__weak void HAL_ADV7511_MspInit()
{
    ;
}

void adv7511_init()
{
    HAL_StatusTypeDef ret = HAL_OK;

    g_i2c_handle.Instance = I2C4;
    g_i2c_handle.Init.Timing = I2C_TIMING;
    g_i2c_handle.Init.OwnAddress1 = I2C_OWN_ADDRESS;
    g_i2c_handle.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    g_i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    g_i2c_handle.Init.OwnAddress2 = I2C_OWN_ADDRESS;
    g_i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    g_i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    ret = HAL_I2C_Init(&g_i2c_handle);
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize I2C!", __FILE__, __LINE__, ret);
    }

    HAL_I2CEx_ConfigAnalogFilter(&g_i2c_handle, I2C_ANALOGFILTER_ENABLE);

    adv7511_wr_reg(ADV7511_REG_IRQ, 0xFF); /* Clear interrupts */

    HAL_ADV7511_MspInit(); /* Configure IRQ */
}

void adv7511_configure()
{
    /* startup stuff */
    adv7511_wr_reg(0x01, 0x00);
    adv7511_wr_reg(0x02, 0x18);
    adv7511_wr_reg(0x03, 0x00);
    adv7511_wr_reg(0x15, 0x00);
    adv7511_wr_reg(0x16, 0x00);
    adv7511_wr_reg(0x18, 0x08);

    /* configuration */
    adv7511_wr_reg(0x40, 0x80);
    adv7511_wr_reg(0x41, 0x10);
    adv7511_wr_reg(0x49, 0xA8);
    adv7511_wr_reg(0x55, 0x00);
    adv7511_wr_reg(0x56, 0x08);
    adv7511_wr_reg(0x96, 0x20);
    adv7511_wr_reg(0x98, 0x03);
    adv7511_wr_reg(0x99, 0x02);
    adv7511_wr_reg(0x9C, 0x30);
    adv7511_wr_reg(0x9D, 0x61);
    adv7511_wr_reg(0xA2, 0xA4);
    adv7511_wr_reg(0xA3, 0xA4);
    adv7511_wr_reg(0xA5, 0x44);
    adv7511_wr_reg(0xAB, 0x40);
    adv7511_wr_reg(0xAF, 0x14);
    adv7511_wr_reg(0xBA, 0xA0);
    adv7511_wr_reg(0xDE, 0x9C);
    adv7511_wr_reg(0xE4, 0x60);
    adv7511_wr_reg(0xFA, 0x7D);
    adv7511_wr_reg(0x48, 0x00);
    adv7511_wr_reg(0x55, 0x10);
    adv7511_wr_reg(0x56, 0x00);

    adv7511_set_bits(0x16, 0x10); /* Color Depth = 10 bit */
    adv7511_set_bits(0x17, 0x00);  /* DE Generator Enable = Disable */
}

void adv7511_wr_reg(uint8_t reg, uint8_t val)
{
    if(HAL_I2C_Master_WriteReg(&g_i2c_handle, (uint16_t)I2C_ADDRESS, &reg, &val, 2000) != HAL_OK)
    {
        main_error("Failed to write I2C register!", __FILE__, __LINE__, reg);
    }
}

uint8_t adv7511_rd_reg(uint8_t reg)
{
    uint8_t val;

    if(HAL_I2C_Master_ReadReg(&g_i2c_handle, (uint16_t)I2C_ADDRESS, &reg, &val, 2000) != HAL_OK)
    {
        main_error("Failed to read I2C register!", __FILE__, __LINE__, reg);
    }

    return val;
}

void adv7511_set_bits(uint8_t reg, uint8_t bits_to_set)
{
    adv7511_wr_reg(reg, adv7511_rd_reg(reg) | bits_to_set);
}

void adv7511_clear_bits(uint8_t reg, uint8_t bits_to_clear)
{
    adv7511_wr_reg(reg, adv7511_rd_reg(reg) & ~bits_to_clear);
}

void adv7511_irq()
{
    uint8_t val;

    val = adv7511_rd_reg(ADV7511_REG_IRQ);

    /* is this interrupt about HPD? */
    if(val & 0x80)
    {
        /* is HPD asserted ? */
        if(adv7511_rd_reg(ADV7511_REG_DETECT) & 0x40)
        {
            adv7511_configure();
        }
    }

    adv7511_wr_reg(ADV7511_REG_IRQ, 0xFF); /* Clear interrupt */
}
