/*
 * memwa2 keyboard utility
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
 * Responsible for keyboard and that correct keyboard information
 * can be read at any time.
 */

#include "keybd.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbd_cdc_if.h"
#include "if.h"
#include "sm.h"

#define MAX_KEYS            6
#define DEFAULT_KEY_MAX     71

USBH_HandleTypeDef g_usbh_host;
static uint8_t g_current_id;
static uint8_t g_keys_pressed;
static keybd_state_t g_keybd_state;
static keybd_state_t g_shift_state;
static keybd_state_t g_ctrl_state;
static uint8_t g_keys_active_a[MAX_KEYS + 1];

static if_keybd_map_t g_default_keybd_map_p[DEFAULT_KEY_MAX] =
{
  {0x2A,0,0}, {0x28,1,0}, {0x43,2,0}, {0x40,3,0}, {0x3A,4,0}, {0x3C,5,0}, {0x3E,6,0}, {0x42,7,0},
  {0x20,0,1}, {0x1A,1,1}, {0x04,2,1}, {0x21,3,1}, {0x1D,4,1}, {0x16,5,1}, {0x08,6,1}  /*LSHIFT*/,
  {0x22,0,2}, {0x15,1,2}, {0x07,2,2}, {0x23,3,2}, {0x06,4,2}, {0x09,5,2}, {0x17,6,2}, {0x1B,7,2},
  {0x24,0,3}, {0x1C,1,3}, {0x0A,2,3}, {0x25,3,3}, {0x05,4,3}, {0x0B,5,3}, {0x18,6,3}, {0x19,7,3},
  {0x26,0,4}, {0x0C,1,4}, {0x0D,2,4}, {0x27,3,4}, {0x10,4,4}, {0x0E,5,4}, {0x12,6,4}, {0x11,7,4},
  {0x57,0,5}, {0x13,1,5}, {0x0F,2,5}, {0x56,3,5}, {0x37,4,5}, {0x34,5,5}, {0x2F,6,5}, {0x36,7,5},
  {0x2D,0,6}, {0x55,1,6}, {0x33,2,6}, {0x2E,3,6}  /*RSHIFT*/, {0x38,5,6}, {0x30,6,6}, {0x54,7,6},
  {0x1E,0,7}, {0x35,1,7}  /* CTRL */, {0x1F,3,7}, {0x2C,4,7}, {0x2B,5,7}, {0x14,6,7}, {0x29,7,7},
  {0x52,0,8}, {0x51,1,8}, {0x4F,2,8}, {0x50,3,8}, {0x4C,4,8}, /* Joystick A CIA PORT B; up, down, left, right, fire */
  {0x60,8,0}, {0x5A,8,1}, {0x5E,8,2}, {0x5C,8,3}, {0x62,8,4}, /* Joystick B CIA PORT A; up, down, left, right, fire */
};

void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{
    HID_KEYBD_Info_TypeDef *k_pinfo; 
    k_pinfo = USBH_HID_GetKeybdInfo(phost);

    if(k_pinfo != NULL)
    {
        uint8_t i;
        uint8_t shift_pressed = USBH_HID_ShiftPressed(k_pinfo);
        uint8_t ctrl_pressed = USBH_HID_CtrlPressed(k_pinfo);

        memset(g_keys_active_a, 0x00, MAX_KEYS + 1);
        g_keys_pressed = 0;

        for(i = 0; i < MAX_KEYS; i++)
        {
            if(k_pinfo->keys[i] != 0)
            {
                g_keys_active_a[i] = k_pinfo->keys[i];
                g_keys_pressed++;
            }
            else
            {
                break;
            }
        }

        if(g_keys_active_a[0] != 0)
        {
            g_keybd_state = KEYBD_STATE_PRESSED;
        }
        else
        {
            g_keybd_state = KEYBD_STATE_RELEASED;
        }

        if(shift_pressed)
        {
            g_shift_state = KEYBD_STATE_PRESSED;
        }
        else
        {
            g_shift_state = KEYBD_STATE_RELEASED;
        }

        if(ctrl_pressed)
        {
            g_ctrl_state = KEYBD_STATE_PRESSED;
        }
        else
        {
            g_ctrl_state = KEYBD_STATE_RELEASED;
        }
    }
    else
    {
        g_keybd_state = KEYBD_STATE_RELEASED;
        g_shift_state = KEYBD_STATE_RELEASED;
        g_ctrl_state = KEYBD_STATE_RELEASED;
        memset(g_keys_active_a, 0x00, MAX_KEYS + 1);
    }
}

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
	g_current_id = id;
}

void keybd_init()
{
	g_current_id = HOST_USER_DISCONNECTION;
    g_keybd_state = KEYBD_STATE_RELEASED;

	/* Init Host Library */
	USBH_Init(&g_usbh_host, USBH_UserProcess, 0);

	/* Add Supported Class */
	USBH_RegisterClass(&g_usbh_host, USBH_HID_CLASS);

	/* Start Device Process */
	USBH_Start(&g_usbh_host);
}


void keybd_poll()
{
    USBH_Process(&g_usbh_host);
}

keybd_state_t keybd_key_state()
{
    return g_keybd_state;
}

keybd_state_t keybd_get_shift_state()
{
    return g_shift_state;
}

keybd_state_t keybd_get_ctrl_state()
{
    return g_ctrl_state;
}

uint8_t keybd_get_active_key()
{
    return g_keys_active_a[0];
}

uint8_t *keybd_get_active_keys()
{
    return g_keys_active_a;
}

uint8_t keybd_get_active_keys_hash()
{
    return g_keys_active_a[0] ^
           g_keys_active_a[1] ^
           g_keys_active_a[2] ^
           g_keys_active_a[3] ^
           g_keys_active_a[4] ^
           g_keys_active_a[5] ^
           g_keys_active_a[6];
}

uint8_t keybd_get_active_number_of_keys()
{
    return g_keys_pressed;
}

uint8_t keybd_get_active_ascii_key()
{
    return USBH_HID_GetASCIICode(g_keys_active_a[0]);
}

if_keybd_map_t *keybd_get_default_map()
{
    return g_default_keybd_map_p;
}

/* Map USB keyboard scan codes */
void keybd_populate_map(uint8_t *conf_text, if_keybd_map_t *keybd_map_p)
{
    uint8_t keys_cnt = 0;
    char delimiter_p[2] = "\n";
    char scan_code_p[3];
    char matrix_x_p[2];
    char matrix_y_p[2];
    char *keys_pp[DEFAULT_KEY_MAX + 1];

    memset(keys_pp, 0x00, (DEFAULT_KEY_MAX + 1) * sizeof(char *));

    keys_pp[keys_cnt] = strtok((char *)conf_text, delimiter_p);

    /* Parse line */
    scan_code_p[0] = keys_pp[keys_cnt][0];
    scan_code_p[1] = keys_pp[keys_cnt][1];
    scan_code_p[2] = '\0';

    matrix_x_p[0] = keys_pp[keys_cnt][3];
    matrix_x_p[1] = '\0';

    matrix_y_p[0] = keys_pp[keys_cnt][5];
    matrix_y_p[1] = '\0';

    keybd_map_p[keys_cnt].scan_code = strtoul(scan_code_p, NULL, 16);
    keybd_map_p[keys_cnt].matrix_x = atoi(matrix_x_p);
    keybd_map_p[keys_cnt].matrix_y = atoi(matrix_y_p);

    keys_cnt++;

    while(keys_pp[keys_cnt-1] != NULL)
    {
        /* Get the map string */
        keys_pp[keys_cnt] = strtok(NULL, delimiter_p);

        /* Parse line */
        scan_code_p[0] = keys_pp[keys_cnt][0];
        scan_code_p[1] = keys_pp[keys_cnt][1];
        scan_code_p[2] = '\0';

        matrix_x_p[0] = keys_pp[keys_cnt][3];
        matrix_x_p[1] = '\0';

        matrix_y_p[0] = keys_pp[keys_cnt][5];
        matrix_y_p[1] = '\0';

        keybd_map_p[keys_cnt].scan_code = strtoul(scan_code_p, NULL, 16);
        keybd_map_p[keys_cnt].matrix_x = atoi(matrix_x_p);
        keybd_map_p[keys_cnt].matrix_y = atoi(matrix_y_p);

        keys_cnt++;
    }
}
