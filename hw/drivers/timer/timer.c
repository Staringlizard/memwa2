/*
 * memwa2 timer driver
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
 * Responsible for all timing related stuff.
 */

#include "timer.h"
#include "if.h"

extern if_emu_cc_t g_if_cc_emu;

static TIM_HandleTypeDef g_tim_handle_type;
static uint32_t g_timer_ms;
static uint32_t g_timer3;
static uint8_t g_timer;

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(htim);
   
    /* NOTE : This function Should not be modified, when the callback is needed,
              the HAL_TIM_Base_MspInit could be implemented in the user file
     */
}

static void InitTimer3()
{
    HAL_StatusTypeDef ret = HAL_OK;

    RCC_ClkInitTypeDef    clkconfig;
    uint32_t              uwTimclock, uwAPB1Prescaler = 0;
    uint32_t              uwPrescalerValue = 0;
    uint32_t              pFLatency;
    
      /*Configure the TIM3 IRQ priority */
    HAL_NVIC_SetPriority(TIM3_IRQn, 2 ,0);
    
    /* Enable the TIM3 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    
    /* Enable TIM3 clock */
    __HAL_RCC_TIM3_CLK_ENABLE();
    
    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
    
    /* Get APB1 prescaler */
    uwAPB1Prescaler = clkconfig.APB1CLKDivider;
    
    /* Compute TIM3 clock */
    if(uwAPB1Prescaler == RCC_HCLK_DIV1)
    {
        uwTimclock = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        uwTimclock = 2*HAL_RCC_GetPCLK1Freq();
    }

    /* Compute the prescaler value to have TIM3 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000) - 1);
    
    /* Initialize TIM3 */
    g_tim_handle_type.Instance = TIM3;

    g_tim_handle_type.Init.Period = (1000000 / 10000) - 1; /* (1/10000) s time base */
    g_tim_handle_type.Init.Prescaler = uwPrescalerValue;
    g_tim_handle_type.Init.ClockDivision = 0;
    g_tim_handle_type.Init.CounterMode = TIM_COUNTERMODE_DOWN;

    ret = HAL_TIM_Base_Init(&g_tim_handle_type);
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize timer 3!", __FILE__, __LINE__, ret);
    }
    
    ret = HAL_TIM_Base_Start_IT(&g_tim_handle_type);
    /* Start the TIM time Base generation in interrupt mode */
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize timer 3!", __FILE__, __LINE__, ret);
    }
}

void timer_init()
{
    InitTimer3();
}

void systimer_tick()
{
    g_timer++;
    g_timer_ms++;
    if(g_timer == 100)
    {
        if(g_if_cc_emu.if_emu_cc_time.time_tenth_second_fp != NULL)
        {
            g_if_cc_emu.if_emu_cc_time.time_tenth_second_fp();
        }
        g_timer = 0;
    }
}

uint32_t timer_get_ms()
{
    return g_timer_ms;
}

void timer3_set(uint32_t value)
{
    HAL_StatusTypeDef ret = HAL_OK;

    if(value == 0)
    {
        return;
    }

    g_timer3 = value;

    ret = HAL_TIM_Base_Start_IT(&g_tim_handle_type);
    /* Start the TIM time Base generation in interrupt mode */
    if(ret != HAL_OK)
    {
        main_error("Failed to start timer 3", __FILE__, __LINE__, ret);
    }
}

void timer3_tick()
{
    HAL_StatusTypeDef ret = HAL_OK;

    TIM3->SR &= ~((uint32_t)0x01);

    if(g_timer3 == 0)
    {
        ret = HAL_TIM_Base_Stop_IT(&g_tim_handle_type);
        /* Start the TIM time Base generation in interrupt mode */
        if(ret != HAL_OK)
        {
            main_error("Failed to stop timer 3", __FILE__, __LINE__, ret);
        }
    }
    else
    {
        g_timer3--;
    }
}

uint32_t timer3_get()
{
    return g_timer3;
}
