/*
 * memwa2 diagnostic
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
 


#ifndef _DIAG_H
#define _DIAG_H

#include "stm32f7xx_hal.h"
#include "main.h"

typedef enum
{
	DIAG_STATUS_OK,
	DIAG_STATUS_ERROR_SDRAM,
	DIAG_STATUS_ERROR_SDCARD,
	DIAG_STATUS_ERROR_I2C,
	DIAG_STATUS_ERROR_GRAPHIC,
	DIAG_STATUS_ERROR_SID,
	DIAG_STATUS_ERROR_KEYBOARD,
} diag_status_t;

diag_status_t diag_run();
diag_status_t diag_sdcard_run();

#endif
