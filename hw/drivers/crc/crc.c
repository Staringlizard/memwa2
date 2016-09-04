/*
 * memwa2 crc driver
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
 * Responsible for hw crc routines.
 */

#include "crc.h"

static CRC_HandleTypeDef g_crc_handle_type;

void crc_init()
{
    HAL_StatusTypeDef ret = HAL_OK;

    g_crc_handle_type.Instance = CRC;

    /* The default polynomial is used */
    g_crc_handle_type.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_ENABLE;

    /* The default init value is used */
    g_crc_handle_type.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_ENABLE;

    /* The input data are not inverted */
    g_crc_handle_type.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;

    /* The output data are not inverted */
    g_crc_handle_type.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;

    /* The input data are 32 bits lenght */
    g_crc_handle_type.InputDataFormat              = CRC_INPUTDATA_FORMAT_BYTES;

    ret = HAL_CRC_Init(&g_crc_handle_type);
    if(ret != HAL_OK)
    {
        main_error("Failed to initialize CRC!", __FILE__, __LINE__, ret);
    }
}

uint32_t crc_calculate(uint8_t *buffer_p, uint32_t length)
{
    return HAL_CRC_Calculate(&g_crc_handle_type, (uint32_t *)buffer_p, length);
}
