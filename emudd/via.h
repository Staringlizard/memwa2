/*
 * memwa2 via component
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
 

#ifndef _VIA_H
#define _VIA_H

#include "emuddif.h"


/*
 $1800/6144/VIA1+0:   Data Port B (ATN, CLOCK, DATA, Device #)

   +----------+---------------------------------------------------+
   | Bit  7   |   ATN IN                                          |
   | Bits 6-5 |   Device address preset switches:                 |
   |          |     00 = #8, 01 = #9, 10 = #10, 11 = #11          |
   | Bit  4   |   ATN acknowledge OUT                             |
   | Bit  3   |   CLOCK OUT                                       |
   | Bit  2   |   CLOCK IN                                        |
   | Bit  1   |   DATA OUT                                        |
   | Bit  0   |   DATA IN                                         |
   +----------+---------------------------------------------------+
*/
#define REG_VIA1_DATA_PORTB                 0x1800

/*
 $1801/6145/VIA1+1:   Data Port A (free for parallel cables)

   This port is mostly used for extensions using parallel data transfer.
   These lines are then connected to the Userport of a C64.
*/
#define REG_VIA1_DATA_PORTA                 0x1801

/*
 $1802/6146/VIA1+2:   Data Direction Register B

   +----------+---------------------------------------------------+
   | Bit  x   |   1 = Pin PBx set to Output, 0 = Input            |
   +----------+---------------------------------------------------+
Default: $1A, %00011010.
*/
#define REG_VIA1_DIRECTION_PORTB            0x1802

/*
 $1803/6147/VIA1+3:   Data Direction Register A

   +----------+---------------------------------------------------+
   | Bit  x   |   1 = Pin PAx set to Output, 0 = Input            |
   +----------+---------------------------------------------------+
Default: $FF, %11111111.
*/
#define REG_VIA1_DIRECTION_PORTA            0x1803

/*
$1804/6148/VIA1+4:   Timer 1 Low-Byte (Timeout Errors) 
$1805/6149/VIA1+5:   Timer 1 High-Byte (Timeout Errors)
*/
#define REG_VIA1_TIMER_LOW_BYTE             0x1804
#define REG_VIA1_TIMER_HIGH_BYTE            0x1805

/*
 $1806/6150/VIA1+6:   Timer 1-Latch Low-Byte (Timeout Errors)
 $1807/6151/VIA1+7:   Timer 1-Latch High-Byte (Timeout Errors)
*/
#define REG_VIA1_TIMER_LATCH_LOW_BYTE       0x1806
#define REG_VIA1_TIMER_LATCH_HIGH_BYTE      0x1807

/*
 $180B/6155/VIA1+11:   Auxiliary Control Register

   +----------+---------------------------------------------------------+
   | Bits 7-6 |   Timer 1 Control:                                      |
   |          |     00 = Timed Interrupt when Timer 1 is loaded, no PB7 |
   |          |     01 = Continuous Interrupts, no PB7                  |
   |          |     10 = Timed Interrupt when Timer 1 is loaded,        |
   |          |          one-shot on PB7                                |
   |          |     11 = Continuous Interrupts, square-wave on PB7      |
   | Bit  5   |   Timer 2 Control: 0 = Timed Interrupt                  |
   |          |                    1 = Count Pulses on PB6              |
   | Bits 4-2 |   Shift Register Control:                               |
   |          |     000 = Disabled                                      |
   |          |     001 = Shift in under control of Timer 2             |
   |          |     010 = Shift in under control of Phi2                |
   |          |     011 = Shift in under control of ext. Clock          |
   |          |     100 = Shift out free-running at Timer 2 rate        |
   |          |     101 = Shift out under control of Timer 2            |
   |          |     110 = Shift out under control of Phi2               |
   |          |     111 = Shift out under control of ext. Clock         |
   | Bit  1   |   1 = enable latching PB                                |
   | Bit  0   |   1 = enable latching PA                                |
   +----------+---------------------------------------------------------+
*/
#define REG_VIA1_AUXILIARY_CTRL             0x180B

/*
 $180C/6156/VIA1+12:   Peripheral Control Register

   +----------+-----------------------------------------------------+
   | Bits 7-5 |   CB2 Control:                                      |
   |          |     000 = Input negative active edge                |
   |          |     001 = Independent interrupt input negative edge |
   |          |     010 = Input positive active edge                |
   |          |     011 = Independent interrupt input positive edge |
   |          |     100 = Handshake output                          |
   |          |     101 = Pulse output                              |
   |          |     110 = Low output                                |
   |          |     111 = High output                               |
   | Bit  4   |   CB1 Interrupt Control: 0 = Negative active edge   |
   |          |                          1 = Positive active edge   |
   | Bit  3-1 |   CA2 Control: see Bits 7-5                         |
   | Bit  0   |   CA1 Interrupt Control: see Bit 4                  |
   +----------+-----------------------------------------------------+

   CA1 (Input): ATN IN (make Interrupt if ATN occurs)
*/
#define REG_VIA1_PERIPHERAL_CTRL            0x180C
/*
 $180D/6157/VIA1+13:   Interrupt Flag Register

   +-------+------------------------------------------------------+
   | Bit 7 |   1 = Interrupt occured                              |
   | Bit 6 |   Timer 1                                            |
   | Bit 5 |   Timer 2                                            |
   | Bit 4 |   CB1                                                |
   | Bit 3 |   CB2                                                |
   | Bit 2 |   Shift Register ($180A)                             |
   | Bit 1 |   CA1 (ATN IN)                                       |
   | Bit 0 |   CA2                                                |
   +-------+------------------------------------------------------+
*/
#define REG_VIA1_IRQ_STATUS                 0x180D

/*
 $180E/6158/VIA1+14:   Interrupt Enable Register

   +-------+------------------------------------------------------+
   | Bit 7 |   On Read:  Always 1                                 |
   |       |   On Write: 1 = Set Int.-Flags, 0 = Clear Int-.Flags |
   | Bit 6 |   Timer 1                                            |
   | Bit 5 |   Timer 2                                            |
   | Bit 4 |   CB1                                                |
   | Bit 3 |   CB2                                                |
   | Bit 2 |   Shift Register                                     |
   | Bit 1 |   CA1  (ATN IN)                                      |
   | Bit 0 |   CA2                                                |
   +-------+------------------------------------------------------+
*/
#define REG_VIA1_IRQ_ENABLE                   0x180E

#define MASK_VIA1_PORTB_DATA_IN             0x01
#define MASK_VIA1_PORTB_DATA_OUT            0x02
#define MASK_VIA1_PORTB_CLK_IN              0x04
#define MASK_VIA1_PORTB_CLK_OUT             0x08
#define MASK_VIA1_PORTB_ATNA_OUT            0x10
#define MASK_VIA1_PORTB_DEVICE_NO           0x60
#define MASK_VIA1_PORTB_ATN_IN              0x80

/*
 $1C00/7168/VIA2+0:   Data Port B (SYNC, Motors, Bit Rates, LED)

   +----------+---------------------------------------------------+
   | Bit  7   |   0 = SYNC found                                  |
   | Bits 6-5 |   Bit rates (*):                                  |
   |          |     00 = 250000 Bit/s    01 = 266667 Bit/s        |
   |          |     10 = 285714 Bit/s    11 = 307692 Bit/s        |
   | Bit  4   |   Write Protect Sense: 1 = On                     |
   | Bit  3   |   Drive LED: 1 = On                               |
   | Bit  2   |   Drive Motor: 1 = On                             |
   | Bit  1-0 |   Step motor for head movement:                   |
   |          |     Sequence 00/01/10/11/00... moves inwards      |
   |          |     Sequence 00/11/10/01/00... moves outwards     |
   +----------+---------------------------------------------------+
   (*) Usually used: %11 for Tracks  1-17
                     %10 for Tracks 18-24
                     %01 for Tracks 25-30
                     %00 for Tracks 31-35
*/
#define REG_VIA2_DATA_PORTB                 0x1C00

/* 
Port A. Data byte last read from or to be next written onto disk. 
 $1C01/7169/VIA2+1:   Data Port A (Data to/from head)
*/
#define REG_VIA2_DATA_PORTA                 0x1C01

/*
 $1C02/7170/VIA2+2:   Data Direction Register B

   +----------+---------------------------------------------------+
   | Bit  x   |   1 = Pin PBx set to Output, 0 = Input            |
   +----------+---------------------------------------------------+
Default: $6F, %01101111.
*/
#define REG_VIA2_DIRECTION_PORTB            0x1C02

/*
 $1C03/7171/VIA2+3:   Data Direction Register A

   +----------+---------------------------------------------------+
   | Bit  x   |   1 = Pin PAx set to Output, 0 = Input            |
   +----------+---------------------------------------------------+
$00: Read from disk.
$FF: Write onto disk.
*/
#define REG_VIA2_DIRECTION_PORTA            0x1C03

/*
 $1C04/7172/VIA2+4:   Timer 1 Low-Byte (IRQ Timer)
 $1C05/7173/VIA2+5:   Timer 1 High-Byte (IRQ Timer)
 */
#define REG_VIA2_TIMER_LOW_BYTE             0x1C04
#define REG_VIA2_TIMER_HIGH_BYTE            0x1C05

/*
 $1C06/7174/VIA2+6:   Timer 1-Latch Low-Byte (IRQ Timer)
 $1C07/7175/VIA2+7:   Timer 1-Latch High-Byte (IRQ Timer)
 */
#define REG_VIA2_TIMER_LATCH_LOW_BYTE       0x1C06
#define REG_VIA2_TIMER_LATCH_HIGH_BYTE      0x1C07

/*
 $1C0B/7179/VIA2+11:   Auxiliary Control Register

   +----------+---------------------------------------------------------+
   | Bits 7-6 |   Timer 1 Control:                                      |
   |          |     00 = Timed Interrupt when Timer 1 is loaded, no PB7 |
   |          |     01 = Continuous Interrupts, no PB7                  |
   |          |     10 = Timed Interrupt when Timer 1 is loaded,        |
   |          |          one-shot on PB7                                |
   |          |     11 = Continuous Interrupts, square-wave on PB7      |
   | Bit  5   |   Timer 2 Control: 0 = Timed Interrupt                  |
   |          |                    1 = Count Pulses on PB6              |
   | Bits 4-2 |   Shift Register Control:                               |
   |          |     000 = Disabled                                      |
   |          |     001 = Shift in under control of Timer 2             |
   |          |     010 = Shift in under control of Phi2                |
   |          |     011 = Shift in under control of ext. Clock          |
   |          |     100 = Shift out free-running at Timer 2 rate        |
   |          |     101 = Shift out under control of Timer 2            |
   |          |     110 = Shift out under control of Phi2               |
   |          |     111 = Shift out under control of ext. Clock         |
   | Bit  1   |   1 = enable latching PB                                |
   | Bit  0   |   1 = enable latching PA                                |
   +----------+---------------------------------------------------------+
*/
#define REG_VIA2_AUXILIARY_CTRL              0x1C0B

/*
Peripheral control register. Bits:
Bit #1: 1 = Attach Byte Ready line to oVerflow processor flag. (Whenever a data byte has been successfully read from or written to disk, V flag is set to 1.)
Bit #5: Head control; 0 = Read; 1 = Write.

 $1C0C/7180/VIA2+12:   Peripheral Control Register

   +----------+-----------------------------------------------------+
   | Bits 7-5 |   CB2 Control:                                      |
   |          |     000 = Input negative active edge                |
   |          |     001 = Independent interrupt input negative edge |
   |          |     010 = Input positive active edge                |
   |          |     011 = Independent interrupt input positive edge |
   |          |     100 = Handshake output                          |
   |          |     101 = Pulse output                              |
   |          |     110 = Low output                                |
   |          |     111 = High output                               |
   | Bit  4   |   CB1 Interrupt Control: 0 = Negative active edge   |
   |          |                          1 = Positive active edge   |
   | Bit  3-1 |   CA2 Control: see Bits 7-5                         |
   | Bit  0   |   CA1 Interrupt Control: see Bit 4                  |
   +----------+-----------------------------------------------------+

   CA1 (Input):  BYTE-READY
   CA2 (Output): SOE (High = activate BYTE-READY)
   CB2 (Output): Head Mode (Low = Write, High = Read)
*/
#define REG_VIA2_PERIPHERAL_CTRL            0x1C0C

/*
Interrupt status register. Bits:
Bit #6: 1 = Timer underflow occurred.

 $1C0D/7181/VIA2+13:   Interrupt Flag Register

   +-------+------------------------------------------------------+
   | Bit 7 |   1 = Interrupt occured                              |
   | Bit 6 |   Timer 1                                            |
   | Bit 5 |   Timer 2                                            |
   | Bit 4 |   CB1                                                |
   | Bit 3 |   CB2                                                |
   | Bit 2 |   Shift Register                                     |
   | Bit 1 |   CA1                                                |
   | Bit 0 |   CA2                                                |
   +-------+------------------------------------------------------+
*/
#define REG_VIA2_IRQ_STATUS                 0x1C0D

/*
 $1C0E/7182/VIA2+14:   Interrupt Enable Register

   +-------+------------------------------------------------------+
   | Bit 7 |   On Read:  Always 1                                 |
   |       |   On Write: 1 = Set Int.-Flags, 0 = Clear Int-.Flags |
   | Bit 6 |   Timer 1                                            |
   | Bit 5 |   Timer 2                                            |
   | Bit 4 |   CB1                                                |
   | Bit 3 |   CB2                                                |
   | Bit 2 |   Shift Register ($1C0A)                             |
   | Bit 1 |   CA1                                                |
   | Bit 0 |   CA2                                                |
   +-------+------------------------------------------------------+
*/
#define REG_VIA2_IRQ_ENABLE                   0x1C0E

#define MASK_VIA2_PORTB_HEAD                0x03
#define MASK_VIA2_PORTB_MOTOR               0x04
#define MASK_VIA2_PORTB_LED                 0x08
#define MASK_VIA2_PORTB_WR_PROTECT          0x10
#define MASK_VIA2_PORTB_DATA_DENSITY        0x60
#define MASK_VIA2_PORTB_SYNC                0x80

#define MASK_VIA_IRQ_STATUS_IRQ_OCCURED     0x80
#define MASK_VIA_IRQ_STATUS_ATN             0x02
#define MASK_VIA_IRQ_STATUS_TIMER           0x40

#define MASK_VIA_AUXILIARY_TIMER_CTRL       0xC0

#define MASK_VIA_IRQ_ENABLE_ATN               0x02
#define MASK_VIA_IRQ_ENABLE_TIMER             0x40

void via_init();
void via_step(uint32_t cc);
void via_serial_port_activity(uint8_t data);

#endif
