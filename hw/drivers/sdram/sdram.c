/*
 * memwa2 sdram driver
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
 * Responsible for extended SDRAM.
 */

#include "sdram.h"
#include "config.h"

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0003)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define SDRAM_CMD_TIMEOUT 0x1000

static SDRAM_HandleTypeDef g_sdram_handle;
static FMC_SDRAM_TimingTypeDef g_fmc_sdram_timing;

static void sdram_start_sequence()
{
	FMC_SDRAM_CommandTypeDef FMC_SDRAM_Command;
	__IO uint32_t tmpmrd = 0;

	/* Step 1:  Configure a clock configuration enable command */
	FMC_SDRAM_Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	FMC_SDRAM_Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	FMC_SDRAM_Command.AutoRefreshNumber = 1;
	FMC_SDRAM_Command.ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(&g_sdram_handle, &FMC_SDRAM_Command, SDRAM_CMD_TIMEOUT);

	/* Step 2: Insert 100 us minimum delay */
	/* Inserted delay is equal to 1 ms due to systick time base unit (ms) */
	HAL_Delay(1);

	/* Step 3: Configure a PALL (precharge all) command */
	FMC_SDRAM_Command.CommandMode = FMC_SDRAM_CMD_PALL;
	FMC_SDRAM_Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	FMC_SDRAM_Command.AutoRefreshNumber = 1;
	FMC_SDRAM_Command.ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(&g_sdram_handle, &FMC_SDRAM_Command, SDRAM_CMD_TIMEOUT);

	/* Step 4 : Configure a Auto-Refresh command */
	FMC_SDRAM_Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	FMC_SDRAM_Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	FMC_SDRAM_Command.AutoRefreshNumber = 8;
	FMC_SDRAM_Command.ModeRegisterDefinition = 0;

	/* Send the command */
	HAL_SDRAM_SendCommand(&g_sdram_handle, &FMC_SDRAM_Command, SDRAM_CMD_TIMEOUT);

	/* Step 5: Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1        |
	                 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
	                 SDRAM_MODEREG_CAS_LATENCY_3           |
	                 SDRAM_MODEREG_OPERATING_MODE_STANDARD |
	                 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	FMC_SDRAM_Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	FMC_SDRAM_Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	FMC_SDRAM_Command.AutoRefreshNumber = 1;
	FMC_SDRAM_Command.ModeRegisterDefinition = tmpmrd;

	/* Send the command */
	HAL_SDRAM_SendCommand(&g_sdram_handle, &FMC_SDRAM_Command, SDRAM_CMD_TIMEOUT);

	/* Step 6: Set the refresh rate counter */
	/* (15.62 us x Freq) - 20 */
	/* Set the device refresh counter */
	g_sdram_handle.Instance->SDRTR |= ((uint32_t)((1292)<< 1));
}

void sdram_init()
{
    HAL_StatusTypeDef ret = HAL_OK;

    /* SDRAM device configuration */
    g_sdram_handle.Instance = FMC_SDRAM_DEVICE;

    g_fmc_sdram_timing.LoadToActiveDelay    = 1;
    g_fmc_sdram_timing.ExitSelfRefreshDelay = 1;
    g_fmc_sdram_timing.SelfRefreshTime      = 1;
    g_fmc_sdram_timing.RowCycleDelay        = 5;
    g_fmc_sdram_timing.WriteRecoveryTime    = 1;
    g_fmc_sdram_timing.RPDelay              = 2;
    g_fmc_sdram_timing.RCDDelay             = 2;

    g_sdram_handle.Init.SDBank             = SDRAM_BANK;
    g_sdram_handle.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_8;
    g_sdram_handle.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_12;
    g_sdram_handle.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_16;
    g_sdram_handle.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
    g_sdram_handle.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
    g_sdram_handle.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
    g_sdram_handle.Init.SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2;
    g_sdram_handle.Init.ReadBurst          = FMC_SDRAM_RBURST_ENABLE;
    g_sdram_handle.Init.ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_0;

    /* Initialize the SDRAM controller */
    ret = HAL_SDRAM_Init(&g_sdram_handle, &g_fmc_sdram_timing);
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize sdram!", __FILE__, __LINE__, ret);
    }

    sdram_start_sequence();
}
