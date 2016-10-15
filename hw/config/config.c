/*
 * memwa2 configuration
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
 * This file contains the main configuraiton for the hw.
 * Instead of doing all configuration in every single driver, it
 * is being done here by using "weak" keyword.
 * The file also contains the clock setup and cache etc. that is
 * being called by the startup sequence.
 */

#include "config.h"
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_rcc.h"
#include "stm32f7xx_hal_gpio.h"
#include "stm32f7xx_hal_i2c_ex.h"
#include "stm32f7xx_hal_sdram.h"
#include "sdcard.h"
#include "sdram.h"
#include "adv7511.h"
#include "console.h"
#include "keybd.h"
#include "disp.h"
#include "crc.h"
#include "rng.h"
#include "sidbus.h"
#include "timer.h"
#include "joyst.h"
#include <string.h>

void HAL_CRC_MspInit(CRC_HandleTypeDef *hcrc)
{
    __HAL_RCC_CRC_CLK_ENABLE();
}

void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable GPIOs clock */
    __GPIOA_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();

    /* Configure sdcard functionality */
    __HAL_RCC_SDMMC1_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_PP;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_HIGH;
    GPIO_Init.Alternate = GPIO_AF12_SDMMC1;

    GPIO_Init.Pin = GPIO_PIN_8 |    /* SD DAT0 */
                    GPIO_PIN_9 |    /* SD DAT1 */
                    GPIO_PIN_10 |   /* SD DAT2 */
                    GPIO_PIN_11 |   /* SD DAT3 */
                    GPIO_PIN_12;    /* SD CLK */
    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_2;     /* SD CMD */
    HAL_GPIO_Init(GPIOD, &GPIO_Init);

    GPIO_Init.Mode = GPIO_MODE_INPUT;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_LOW;

    GPIO_Init.Pin = GPIO_PIN_7;     /* R3 PA7 SD CD */
    HAL_GPIO_Init(GPIOA, &GPIO_Init);
}

void  HAL_HCD_MspInit(HCD_HandleTypeDef *hhcd)
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_PP;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_HIGH;
    GPIO_Init.Alternate = GPIO_AF12_OTG_HS_FS;

    GPIO_Init.Pin = GPIO_PIN_14 |    /* OTG_HS_DM */
                    GPIO_PIN_15;     /* OTG_HS_DP */
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Enable USB HS Clocks */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();

    /* Set USBHS Interrupt to the lowest priority */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 5, 0);

    /* Enable USBHS Interrupt */
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);
}

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    GPIO_InitTypeDef GPIO_Init;

    /* Configure USB FS GPIOs */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_HIGH;
    GPIO_Init.Alternate = GPIO_AF10_OTG_FS;

    /* Configure DM DP Pins */
    GPIO_Init.Pin = GPIO_PIN_11 | /* OTG_FS_DM */
                    GPIO_PIN_12;  /* OTG_FS_DP */
    HAL_GPIO_Init(GPIOA, &GPIO_Init); 

    /* Enable USB FS Clock */
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    /* Set USBFS Interrupt priority */
    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6, 0);

    /* Enable USBFS Interrupt */
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_Init;
    RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInit;

    /* Configure the I2C clock source */
    RCC_PeriphCLKInit.PeriphClockSelection = RCC_PERIPHCLK_I2C4;
    RCC_PeriphCLKInit.I2c4ClockSelection = RCC_I2C4CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInit);

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Enable I2C clock */
    __HAL_RCC_I2C4_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_OD;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_HIGH;
    GPIO_Init.Alternate = GPIO_AF4_I2C4;

    GPIO_Init.Pin = GPIO_PIN_12 |    /* I2C4_SCL */
                    GPIO_PIN_13;     /* I2C4_SDA */
    HAL_GPIO_Init(GPIOD, &GPIO_Init);
}

void HAL_ADV7511_MspInit() /* Memwa2 specific */
{
    GPIO_InitTypeDef GPIO_Init;

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_FAST;

    GPIO_Init.Pin = GPIO_PIN_15; /* A13 PA15 ADV7511 IRQ */
    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    /* Enable and set interrupt for adv7511 */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Enable FMC clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_PP;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_FAST;
    GPIO_Init.Alternate = GPIO_AF12_FMC;
  
    GPIO_Init.Pin = GPIO_PIN_2 | /* M4 PC2 FMC_SDNE0 */
                    GPIO_PIN_3; /* M5 PC3 FMC_SDCKE0 */
    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_0 | /* B12 PD0 FMC_D2 */
                    GPIO_PIN_1 | /* C12 PD1 FMC_D3 */
                    GPIO_PIN_8 | /* P15 PD8 FMC_D13 */
                    GPIO_PIN_9 | /* P14 PD9 FMC_D14 */
                    GPIO_PIN_10 | /* N15 PD10 FMC_D15 */
                    GPIO_PIN_14 | /* M14 PD14 FMC_D0 */
                    GPIO_PIN_15; /* L14 PD15 FMC_D1 */
    HAL_GPIO_Init(GPIOD, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_0 | /* A4 PE0 FMC_NBL0 */
                    GPIO_PIN_1 | /* A3 PE1 FMC_NBL1 */
                    GPIO_PIN_7 | /* R8 PE7 FMC_D4 */
                    GPIO_PIN_8 | /* P8 PE8 FMC_D5 */
                    GPIO_PIN_9 | /* P9 PE9 FMC_D6 */
                    GPIO_PIN_10 | /* R9 PE10 FMC_D7 */
                    GPIO_PIN_11 | /* P10 PE11 FMC_D8 */
                    GPIO_PIN_12 | /* R10 PE12 FMC_D9 */
                    GPIO_PIN_13 | /* N11 PE13 FMC_D10 */
                    GPIO_PIN_14 | /* P11 PE14 FMC_D11 */
                    GPIO_PIN_15; /* R11 PE15 FMC_D12 */
    HAL_GPIO_Init(GPIOE, &GPIO_Init);
    
    GPIO_Init.Pin = GPIO_PIN_0 | /* E2 PF0 FMC_A0 */
                    GPIO_PIN_1 | /* H3 PF1 FMC_A1 */
                    GPIO_PIN_2 | /* H2 PF2 FMC_A2 */
                    GPIO_PIN_3 | /* J2 PF3 FMC_A3 */
                    GPIO_PIN_4 | /* J3 PF4 FMC_A4 */
                    GPIO_PIN_5 | /* K3 PF5 FMC_A5 */
                    GPIO_PIN_11 | /* R6 PF11 FMC_SDNRAS */
                    GPIO_PIN_12 | /* P6 PF12 FMC_A6 */
                    GPIO_PIN_13 | /* N6 PF13 FMC_A7 */
                    GPIO_PIN_14 | /* R7 PF14 FMC_A8 */
                    GPIO_PIN_15; /* P7 PF15 FMC_A9 */
    HAL_GPIO_Init(GPIOF, &GPIO_Init);   

    GPIO_Init.Pin = GPIO_PIN_0 | /* N7 PG0 FMC_A10 */
                    GPIO_PIN_1 | /* M7 PG1 FMC_A11 */
                    GPIO_PIN_4 | /* K14 PG4 FMC_A14 */
                    GPIO_PIN_5 | /* K13 PG5 FMC_A15 */
                    GPIO_PIN_8 | /* H14 PG8 FMC_SDCLK */
                    GPIO_PIN_15; /* B7 PG15 FMC_SDNCAS */
    HAL_GPIO_Init(GPIOG, &GPIO_Init);  
    
    GPIO_Init.Pin = GPIO_PIN_5; /* J4 PH5 FMC_SDNWE */
    HAL_GPIO_Init(GPIOH, &GPIO_Init);  
}

void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable the LTDC and DMA2D clocks */
    __HAL_RCC_LTDC_CLK_ENABLE();
    //__HAL_RCC_DMA2D_CLK_ENABLE();

    /* No worries, it will be enabled by hal again */
    //__HAL_LTDC_DISABLE(hltdc);

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_AF_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FAST;

    /* Watch out, LTDC has 2 alternative func mappings */
    GPIO_Init.Alternate = GPIO_AF14_LTDC;

    GPIO_Init.Pin = GPIO_PIN_1 | /* N2 PA1 LCD_R2 */
                    GPIO_PIN_2 | /* P2 PA2 LCD_R1 */
                    GPIO_PIN_3 | /* R2 PA3 LCD_B5 */
                    GPIO_PIN_4 | /* N4 PA4 LCD_VSYNC */
                    GPIO_PIN_5 | /* P4 PA5 LCD_R4 */
                    GPIO_PIN_6; /* P3 PA6 LCD_G2 */
    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_8 | /* A5 PB8 LCD_B6 */
                    GPIO_PIN_9 | /* B4 PB9 LCD_B7 */
                    GPIO_PIN_10 | /* R12 PB10 LCD_G4 */
                    GPIO_PIN_11; /* R13 PB11 LCD_G5 */
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_0 | /* M2 PC0 LCD_R5 */
                    GPIO_PIN_6 | /* H15 PC6 LCD_HSYNC */
                    GPIO_PIN_7; /* G15 PC7 LCD_G6 */
    HAL_GPIO_Init(GPIOC, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_6; /* B11 PD6 LCD_B2 */
    HAL_GPIO_Init(GPIOD, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_4 | /* B1 PE4 LCD_B0 */
                    GPIO_PIN_5 | /* B2 PE5 LCD_G0 */
                    GPIO_PIN_6;/* B3 PE6 LCD_G1 */
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_10; /* L1 PF10 LCD_DE */
    HAL_GPIO_Init(GPIOF, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_6 | /* J15 PG6 LCD_R7 */
                    GPIO_PIN_7 | /* J14 PG7 LCD_CLK */
                    GPIO_PIN_11 | /* B9 PG11 LCD_B3 */
                    GPIO_PIN_12; /* B8 PG12 LCD_B1 */ 
    HAL_GPIO_Init(GPIOG, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_2; /* F4 PH2 LCD_R0 */
    HAL_GPIO_Init(GPIOH, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_2 | /* C14 PI2 LCD_G7 */
                    GPIO_PIN_4; /* D4 PI4 LCD_B4 */
    HAL_GPIO_Init(GPIOI, &GPIO_Init);


    GPIO_Init.Alternate = GPIO_AF9_LTDC;

    GPIO_Init.Pin = GPIO_PIN_10; /* B10 PG10 LCD_G3 */

    HAL_GPIO_Init(GPIOG, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_0 | /* R5 PB0 LCD_R3 */
                    GPIO_PIN_1; /* R4 PB1 LCD_R6 */
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Set LTDC Interrupt priority */
    HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);
    HAL_NVIC_SetPriority(LTDC_ER_IRQn, 0xD, 0);
    
    /* Enable LTDC Interrupt */
    HAL_NVIC_EnableIRQ(LTDC_IRQn);
    HAL_NVIC_EnableIRQ(LTDC_ER_IRQn);
}

void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng)
{
  /* RNG Peripheral clock enable */
  __HAL_RCC_RNG_CLK_ENABLE();
}

void HAL_SIDBUS_MspInit() /* Memwa2 specific */
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FAST;

    /* Sidbus data lines */
    GPIO_Init.Pin = GPIO_PIN_8 | /* M12 PH8 SIDBUS D0 */
                    GPIO_PIN_9 | /* M13 PH9 SIDBUS D1 */
                    GPIO_PIN_10 | /* L13 PH10 SIDBUS D2 */
                    GPIO_PIN_11 | /* L12 PH11 SIDBUS D3 */
                    GPIO_PIN_12 | /* K12 PH12 SIDBUS D4 */
                    GPIO_PIN_13 | /* E12 PH13 SIDBUS D5 */
                    GPIO_PIN_14 | /* E13 PH14 SIDBUS D6 */
                    GPIO_PIN_15; /* D13 PH15 SIDBUS D7 */
    HAL_GPIO_Init(GPIOH, &GPIO_Init);

    /* Sidbus address and ctrl lines */
    GPIO_Init.Pin = GPIO_PIN_5 | /* C4 PI5 SIDBUS RW */
                    GPIO_PIN_6 | /* C3 PI6 SIDBUS CS */
                    GPIO_PIN_7 | /* C2 PI7 SIDBUS A0 */
                    GPIO_PIN_8 | /* D2 PI8 SIDBUS A1 */ 
                    GPIO_PIN_9 | /* D3 PI9 SIDBUS A2 */
                    GPIO_PIN_10 | /* E3 PI10 SIDBUS A3 */
                    GPIO_PIN_11; /* E4 PI11 SIDBUS A4 */
    HAL_GPIO_Init(GPIOI, &GPIO_Init);

    /* Sidbus clock external interrupt */
    GPIO_Init.Mode = GPIO_MODE_IT_RISING;
    GPIO_Init.Pull = GPIO_NOPULL;
    GPIO_Init.Speed = GPIO_SPEED_FAST;
    GPIO_Init.Pin = GPIO_PIN_13; /* A8 PG13 SIDBUS CLK */
    HAL_GPIO_Init(GPIOG, &GPIO_Init);

    EXTI->RTSR &= ~GPIO_PIN_13; /* Disable sidbus irq directly */

    /*
     * Enable and set interrupt.
     * This irq line is connected directly to sid clk.
     * This is needed since it is only possible to write when
     * clk is high.
     */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 2);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void HAL_JOYST_MspInit() /* Memwa2 specific */
{
    GPIO_InitTypeDef GPIO_Init;

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_Init.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_Init.Pull = GPIO_PULLUP;
    GPIO_Init.Speed = GPIO_SPEED_LOW;

    GPIO_Init.Pin = GPIO_PIN_10; /* D15 PA10 JOY_A5 FIRE */
    HAL_GPIO_Init(GPIOA, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_5; /* A6 PB5 JOY_A3 LEFT */
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_2 | /* A2 PE2 JOY_A1 UP */
                    GPIO_PIN_3; /* A1 PE3 JOY_A2 DOWN */
    HAL_GPIO_Init(GPIOE, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_6 | /* K2 PF6 JOY_B3 LEFT */
                    GPIO_PIN_7 | /* K1 PF7 JOY_B4 RIGHT */
                    GPIO_PIN_8 | /* L3 PF8 JOY_B1 UP */
                    GPIO_PIN_9; /* L2 PF9 JOY_B2 DOWN */
    HAL_GPIO_Init(GPIOF, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_14; /* A7 PG14 JOY_A4 RIGHT */
    HAL_GPIO_Init(GPIOG, &GPIO_Init);

    GPIO_Init.Pin = GPIO_PIN_4; /* H4 PH4 JOY_B5 FIRE */
    HAL_GPIO_Init(GPIOH, &GPIO_Init);


    /* Enable and set interrupt for joystick A and B */
    HAL_NVIC_SetPriority(EXTI2_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
    HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  /*Configure the SysTick to have interrupt in ms time basis*/
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  /*Configure the SysTick IRQ priority */
  HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority ,0);

  /* Return function status */
  return HAL_OK;
}

static void mpu_config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WB for SDRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = SDRAM_ADDR;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

static void config_clks()
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

#ifdef USE_HSE
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
#endif  
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLN = 432;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 9;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    HAL_PWREx_EnableOverDrive();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC |
                                               RCC_PERIPHCLK_I2C4 |
                                               RCC_PERIPHCLK_SDMMC1 |
                                               RCC_PERIPHCLK_CLK48;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 320;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
    PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
    PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
    PeriphClkInitStruct.PLLSAIDivQ = 1;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    PeriphClkInitStruct.I2c4ClockSelection = RCC_I2C4CLKSOURCE_HSI;
    PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
    PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 1, 1);
}

static void config_crc()
{
    crc_init();
}

static void config_sdcard()
{
    sdcard_init();
}

static void config_rng()
{
    rng_init();
}

static void config_sidbus()
{
    sidbus_init();
}

static void config_timer()
{
    timer_init();
}

static void config_joyst()
{
    joyst_init();
}

static void config_adv7511()
{
    adv7511_init();
    adv7511_configure();
}

static void config_sdram()
{
    mpu_config();
    sdram_init();
}

static void config_console()
{
    console_init();
}

static void config_keybd()
{
    keybd_init();
}

static void config_disp()
{
    disp_init(DISP_MODE_SVGA);
    disp_deactivate_layer(0);
    disp_deactivate_layer(1);
}

static void config_enable_cpu_cache()
{
    SCB_EnableICache();
    SCB_EnableDCache();
}

void config_init()
{
    HAL_EnableFMCMemorySwapping();
    config_enable_cpu_cache();
    config_clks();
    config_sdram();
    config_sdcard();
    config_crc();
    config_rng();
    config_sidbus();
    config_timer();
    config_joyst();
    config_adv7511();
    config_console();
    config_keybd();
    config_disp();
}
