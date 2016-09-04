/*
 * memwa2 random generator driver
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
 * Responsible for random generator.
 */

#include "rng.h"

static RNG_HandleTypeDef g_rng_handle;

void rng_init()
{
    HAL_StatusTypeDef ret = HAL_OK;

    g_rng_handle.Instance = RNG;

    ret = HAL_RNG_Init(&g_rng_handle);
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize RNG!", __FILE__, __LINE__, 0);
    }
}

uint32_t rng_get()
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint32_t rnd_number;

    while(status != HAL_OK)
    {
        status = HAL_RNG_GenerateRandomNumber(&g_rng_handle, &rnd_number);
    }
    return rnd_number;
}
