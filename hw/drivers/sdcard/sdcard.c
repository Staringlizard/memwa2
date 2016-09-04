/*
 * memwa2 sdcard driver
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
 * Responsible for SDRAM.
 */

#include "sdcard.h"
#include "config.h"

SD_HandleTypeDef g_sd_handle;
static HAL_SD_CardInfoTypedef g_hal_sd_card_info;

void sdcard_init()
{
    g_sd_handle.Instance = SDMMC1;
    g_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    g_sd_handle.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
    g_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    g_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_1B;
    g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    g_sd_handle.Init.ClockDiv = 0;
    HAL_SD_Init(&g_sd_handle, &g_hal_sd_card_info);
}

uint8_t sdcard_inserted()
{
    if((SDCARD_CD_PORT->IDR & SDCARD_CD_PIN) != (uint32_t)GPIO_PIN_RESET)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
