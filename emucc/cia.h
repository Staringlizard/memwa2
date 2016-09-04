/*
 * memwa2 cia component
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
 

#ifndef _CIA_H
#define _CIA_H

#include "emuccif.h"

/*
 * $DC00/56320/CIA1+0   Data Port A (Keyboard, Joystick, Paddles)
 * $DC01/56321/CIA1+1   Data Port B (Keyboard, Joystick, Paddles)
 * $DC02/56322/CIA1+2   Data Direction Register A
 * $DC03/56323/CIA1+3   Data Direction Register B
 * $DC04/56324/CIA1+4   Timer A Low-Byte  (Kernal-IRQ, Tape)
 * $DC05/56325/CIA1+5   Timer A High-Byte (Kernal-IRQ, Tape)
 * $DC06/56326/CIA1+6   Timer B Low-Byte  (Tape, Serial Port)
 * $DC07/56327/CIA1+7   Timer B High-Byte (Tape, Serial Port)
 * $DC08/56328/CIA1+8   Time-of-Day Clock: 1/10 Seconds
 * $DC09/56329/CIA1+9   Time-of-Day Clock: Seconds
 * $DC0A/56330/CIA1+10  Time-of-Day Clock: Minutes
 * $DC0B/56331/CIA1+11  Time-of-Day Clock: Hours + AM/PM Flag
 * $DC0C/56332/CIA1+12  Synchronous Serial I/O Data Buffer
 * $DC0D/56333/CIA1+13  Interrupt (IRQ) Control Register
 * $DC0E/56334/CIA1+14  Control Register A
 * $DC0F/56335/CIA1+15  Control Register B
*/

#define REG_CIA1_DATA_PORT_A                0xDC00
#define REG_CIA1_DATA_PORT_B                0xDC01
#define REG_CIA1_DATA_DIRECTION_A           0xDC02
#define REG_CIA1_DATA_DIRECTION_B           0xDC03
#define REG_CIA1_TIMER_A_LOW_BYTE           0xDC04
#define REG_CIA1_TIMER_A_HIGH_BYTE          0xDC05
#define REG_CIA1_TIMER_B_LOW_BYTE           0xDC06
#define REG_CIA1_TIMER_B_HIGH_BYTE          0xDC07
#define REG_CIA1_TIME_OF_DAY_TENTH_SEC      0xDC08
#define REG_CIA1_TIME_OF_DAY_SEC            0xDC09
#define REG_CIA1_TIME_OF_DAY_MIN            0xDC0A
#define REG_CIA1_TIME_OF_DAY_HOURS          0xDC0B
#define REG_CIA1_SYNCHRONOUS_SERIAL_IO      0xDC0C
#define REG_CIA1_INTERRUPT_CONTROL_REG      0xDC0D
#define REG_CIA1_CONTROL_A                  0xDC0E
#define REG_CIA1_CONTROL_B                  0xDC0F

/*
 * CIA 1
 *  +----------+---------------------------------------------------+
 *  | Bits 7-0 |   Read Keyboard Row Values for Keyboard Scan      |
 *  | Bit  4   |   Joystick B Fire Button: 0 = Pressed             |
 *  | Bit  3   |   Joystick B Right: 0 = Pressed, or Paddle Button |
 *  | Bit  2   |   Joystick B Left : 0 = Pressed, or Paddle Button |
 *  | Bit  1   |   Joystick B Down : 0 = Pressed                   |
 *  | Bit  0   |   Joystick B Up   : 0 = Pressed                   |
 *  +----------+---------------------------------------------------+
 * CIA 2
 *  +-------+------------------------------------------------------+
 *  | Bit 7 |   User Port PB7 / RS232 Data Set Ready               |
 *  | Bit 6 |   User Port PB6 / RS232 Clear to Send                |
 *  | Bit 5 |   User Port PB5                                      |
 *  | Bit 4 |   User Port PB4 / RS232 Carrier Detect               |
 *  | Bit 3 |   User Port PB3 / RS232 Ring Indicator               |
 *  | Bit 2 |   User Port PB2 / RS232 Data Terminal Ready          |
 *  | Bit 1 |   User Port PB1 / RS232 Request to Send              |
 *  | Bit 0 |   User Port PB0 / RS232 Received Data                |
 *  +-------+------------------------------------------------------+
 */

#define MASK_CIA_DATA_PORTB_TOOGLE_PULSE_TIMER_B        0x40
#define MASK_CIA_DATA_PORTB_TOOGLE_PULSE_TIMER_A        0x20
#define MASK_CIA1_DATA_PORTB_JOY_B_FIRE                 0x10
#define MASK_CIA1_DATA_PORTB_JOY_B_RIGHT                0x08
#define MASK_CIA1_DATA_PORTB_JOY_B_LEFT                 0x04
#define MASK_CIA1_DATA_PORTB_JOY_B_DOWN                 0x02
#define MASK_CIA1_DATA_PORTB_JOY_B_UP                   0x01

/*
 * $DD00/56576/CIA2+0  Data Port A (Serial Bus, RS232, VIC Base Mem.)
 * $DD01/56577/CIA2+1  Data Port B (User Port, RS232)
 * $DD02/56578/CIA2+2  Data Direction Register A
 * $DD03/56579/CIA2+3  Data Direction Register B
 * $DD04/56580/CIA2+4  Timer A Low-Byte  (RS232)
 * $DD05/56581/CIA2+5  Timer A High-Byte (RS232)
 * $DD06/56582/CIA2+6  Timer B Low-Byte  (RS232)
 * $DD07/56583/CIA2+7  Timer B High-Byte (RS232)
 * $DD08/56584/CIA2+8  Time-of-Day Clock: 1/10 Seconds
 * $DD09/56585/CIA2+9  Time-of-Day Clock: Seconds
 * $DD0A/56586/CIA2+10 Time-of-Day Clock: Minutes
 * $DD0B/56587/CIA2+11 Time-of-Day Clock: Hours + AM/PM Flag
 * $DD0C/56588/CIA2+12 Synchronous Serial I/O Data Buffer
 * $DD0D/56589/CIA2+13 Interrupt (NMI) Control Register
 * $DD0E/56590/CIA2+14 Control Register A
 * $DD0F/56591/CIA2+15 Control Register B
 */

#define REG_CIA2_DATA_PORT_A                0xDD00
#define REG_CIA2_DATA_PORT_B                0xDD01
#define REG_CIA2_DATA_DIRECTION_A           0xDD02
#define REG_CIA2_DATA_DIRECTION_B           0xDD03
#define REG_CIA2_TIMER_A_LOW_BYTE           0xDD04
#define REG_CIA2_TIMER_A_HIGH_BYTE          0xDD05
#define REG_CIA2_TIMER_B_LOW_BYTE           0xDD06
#define REG_CIA2_TIMER_B_HIGH_BYTE          0xDD07
#define REG_CIA2_TIME_OF_DAY_TENTH_SEC      0xDD08
#define REG_CIA2_TIME_OF_DAY_SEC            0xDD09
#define REG_CIA2_TIME_OF_DAY_MIN            0xDD0A
#define REG_CIA2_TIME_OF_DAY_HOURS          0xDD0B
#define REG_CIA2_SYNCHRONOUS_SERIAL_IO      0xDD0C
#define REG_CIA2_INTERRUPT_CONTROL_REG      0xDD0D
#define REG_CIA2_CONTROL_A                  0xDD0E
#define REG_CIA2_CONTROL_B                  0xDD0F


/*
 * +----------+---------------------------------------------------+
 * | Bit  7   |  Serial Bus Data Input                            |
 * | Bit  6   |  Serial Bus Clock Pulse Input                     |
 * | Bit  5   |  Serial Bus Data Output                           |
 * | Bit  4   |  Serial Bus Clock Pulse Output                    |
 * | Bit  3   |  Serial Bus ATN Signal Output                     |
 * | Bit  2   |  RS232 Data Output (User Port)                    |
 * | Bit  1-0 |  VIC Chip System Memory Bank Select (low active!) |
 * +----------+---------------------------------------------------+
 */

#define MASK_CIA2_DATA_PORTA_SB_DATA_INPUT          0x80
#define MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_INPUT   0x40
#define MASK_CIA2_DATA_PORTA_SB_DATA_OUTPUT         0x20
#define MASK_CIA2_DATA_PORTA_SB_CLOCK_PULSE_OUTPUT  0x10
#define MASK_CIA2_DATA_PORTA_SB_ATN_SIGNAL_OUTPUT   0x08
#define MASK_CIA2_DATA_PORTA_RS232_DATA_OUTPUT      0x04
#define MASK_CIA2_DATA_PORTA_VIC_CHIP_BANK          0x03


/*
 * CIA 1
 *  +-------+------------------------------------------------------+
 *  | Bit 7 |   On Read:  1 = Interrupt occured                    |
 *  |       |   On Write: 1 = Set Int.-Flags, 0 = Clear Int-.Flags |
 *  | Bit 4 |   FLAG1 IRQ (Cassette Read / Serial Bus SRQ Input)   |
 *  | Bit 3 |   Serial Port Interrupt ($DC0C full/empty)           |
 *  | Bit 2 |   Time-of-Day Clock Alarm Interrupt                  |
 *  | Bit 1 |   Timer B Interrupt (Tape, Serial Port)              |
 *  | Bit 0 |   Timer A Interrupt (Kernal-IRQ, Tape)               |
 *  +-------+------------------------------------------------------+
 * CIA 2
 *  +-------+------------------------------------------------------+
 *  | Bit 7 |   On Read:  1 = Interrupt occured                    |
 *  |       |   On Write: 1 = Set Int.-Flags, 0 = Clear Int.-Flags |
 *  | Bit 4 |   FLAG1 NMI (User/RS232 Received Data Input)         |
 *  | Bit 3 |   Serial Port Interrupt ($DD0C full/empty)           |
 *  | Bit 2 |   Time-of-Day Clock Alarm Interrupt                  |
 *  | Bit 1 |   Timer B Interrupt (RS232)                          |
 *  | Bit 0 |   Timer A Interrupt (RS232)                          |
 *  +-------+------------------------------------------------------+
 */

#define MASK_CIA_INTERRUPT_CONTROL_REG_MULTI        0x80
#define MASK_CIA_INTERRUPT_CONTROL_REG_FLAG1        0x10
#define MASK_CIA_INTERRUPT_CONTROL_REG_SERIAL       0x08
#define MASK_CIA_INTERRUPT_CONTROL_REG_TOD          0x04
#define MASK_CIA_INTERRUPT_CONTROL_REG_TIMER_B      0x02
#define MASK_CIA_INTERRUPT_CONTROL_REG_TIMER_A      0x01

/*
 *  +-------+--------------------------------------------------------+
 *  | Bit 7 |   Time-of-Day Clock Frequency: 1 = 50 Hz, 0 = 60 Hz    |
 *  | Bit 6 |   Serial Port ($DC0C) I/O Mode: 1 = Output, 0 = Input  |
 *  | Bit 5 |   Timer A Counts: 1 = CNT Signals, 0 = System 02 Clock |
 *  | Bit 4 |   Force Load Timer A: 1 = Yes                          |
 *  | Bit 3 |   Timer A Run Mode: 1 = One-Shot, 0 = Continuous       |
 *  | Bit 2 |   Timer A Output Mode to PB6: 1 = Toggle, 0 = Pulse    |
 *  | Bit 1 |   Timer A Output on PB6: 1 = Yes, 0 = No               |
 *  | Bit 0 |   Start/Stop Timer A: 1 = Start, 0 = Stop              |
 *  +-------+--------------------------------------------------------+
 */

#define MASK_CIA_CONTROL_A_TOD_CLOCK_FREQ         0x80
#define MASK_CIA_CONTROL_A_SERIAL_PORT_IO_MODE    0x40
#define MASK_CIA_CONTROL_A_TIMER_A_COUNTS         0x20
#define MASK_CIA_CONTROL_FORCE_LOAD_TIMER         0x10
#define MASK_CIA_CONTROL_RUN_MODE                 0x08
#define MASK_CIA_CONTROL_OUTPUT_MODE              0x04
#define MASK_CIA_CONTROL_OUTPUT_ON                0x02
#define MASK_CIA_CONTROL_START_STOP_TIMER         0x01


/*
 *  +----------+-----------------------------------------------------+
 *  | Bit  7   |   Set Alarm/TOD-Clock: 1 = Alarm, 0 = Clock         |
 *  | Bits 6-5 |   Timer B Mode Select:                              |
 *  |          |            00 = Count System 02 Clock Pulses        |
 *  |          |            01 = Count Positive CNT Transitions      |
 *  |          |            10 = Count Timer A Underflow Pulses      |
 *  |          |            11 = Count Timer A Underflows While CNT  |
 *  | Bit  4   |   Force Load Timer B: 1 = Yes                       |
 *  | Bit  3   |   Timer B Run Mode: 1 = One-Shot, 0 = Continuous    |
 *  | Bit  2   |   Timer B Output Mode to PB7: 1 = Toggle, 0 = Pulse |
 *  | Bit  1   |   Timer B Output on PB7: 1 = Yes, 0 = No            |
 *  | Bit  0   |   Start/Stop Timer B: 1 = Start, 0 = Stop           |
 *  +----------+-----------------------------------------------------+
 */

#define MASK_CIA_CONTROL_B_SET_ALARM_TOD_CLOCK   0x80
#define MASK_CIA_CONTROL_B_TIMER_B_MODE_SELECT   0x60
#define MASK_CIA_CONTROL_B_TIMER_B_MODE_SELECT_CNT   0x20

void cia_init();
void cia_step(uint32_t cc);
void cia_request_irq_tape();
void cia_tenth_second();
void cia_serial_port_activity(uint8_t data);

#endif
