#ifndef _SIDBUS_H
#define _SIDBUS_H

#include "stm32f7xx_hal.h"
#include "main.h"

typedef enum
{
    SIDBUS_STATE_ACTIVATE_CHIP,
    SIDBUS_STATE_DATA_SENT
} sidbus_state_t;

void sidbus_irq();
void sidbus_init();
void sidbus_write(uint8_t addr, uint8_t value);
uint8_t sidbus_read(uint8_t addr);

#endif
