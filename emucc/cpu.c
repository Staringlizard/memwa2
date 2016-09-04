/*
 * memwa2 cpu component
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
 * CPU implementation (MOS6510).
 */

#include "cpu.h"
#include "bus.h"
#include "vic.h"
#include "cia.h"
#include "tap.h"
#include "if.h"

#include <assert.h>
#include <string.h>

static void IRQ(uint8_t *cc, uint8_t type, uint8_t brk);
static void ADC(uint8_t *cc); // Add Memory to Accumulator with Carry
static void AND(uint8_t *cc); // "AND" Memory with Accumulator
static void ASL(uint8_t *cc); // Shift Left One Bit (Memory or Accumulator)
static void BCC(uint8_t *cc); // Branch on Carry Clear
static void BCS(uint8_t *cc); // Branch on Carry Set
static void BEQ(uint8_t *cc); // Branch on Result Zero
static void BIT(uint8_t *cc); // Test Bits in Memory with Accumulator
static void BMI(uint8_t *cc); // Branch on Result Minus
static void BNE(uint8_t *cc); // Branch on Result not Zero
static void BPL(uint8_t *cc); // Branch on Result Plus
static void BRK(uint8_t *cc); // Force Break
static void BVC(uint8_t *cc); // Branch on Overflow Clear
static void BVS(uint8_t *cc); // Branch on Overflow Set
static void CLC(uint8_t *cc); // Clear Carry Flag
static void CLD(uint8_t *cc); // Clear Decimal Mode
static void CLI(uint8_t *cc); // Clear interrupt Disable Bit
static void CLV(uint8_t *cc); // Clear Overflow Flag
static void CMP(uint8_t *cc); // Compare Memory and Accumulator
static void CPX(uint8_t *cc); // Compare Memory and Index X
static void CPY(uint8_t *cc); // Compare Memory and Index Y
static void DEC(uint8_t *cc); // Decrement Memory by One
static void DEX(uint8_t *cc); // Decrement Index X by One
static void DEY(uint8_t *cc); // Decrement Index Y by One
static void EOR(uint8_t *cc); // "Exclusive-Or" Memory with Accumulator
static void INC(uint8_t *cc); // Increment Memory by One
static void INX(uint8_t *cc); // Increment Index X by One
static void INY(uint8_t *cc); // Increment Index Y by One
static void JMP(uint8_t *cc); // Jump to New Location
static void JSR(uint8_t *cc); // Jump to New Location Saving Return Address
static void LDA(uint8_t *cc); // Load Accumulator with Memory
static void LDX(uint8_t *cc); // Load Index X with Memory
static void LDY(uint8_t *cc); // Load Index Y with Memory
static void LSR(uint8_t *cc); // Shift Right One Bit (Memory or Accumulator)
static void NOP(uint8_t *cc); // No Operation
static void ORA(uint8_t *cc); // "OR" Memory with Accumulator
static void PHA(uint8_t *cc); // Push Accumulator on Stack
static void PHP(uint8_t *cc); // Push Processor Status on Stack
static void PLA(uint8_t *cc); // Pull Accumulator from Stack
static void PLP(uint8_t *cc); // Pull Processor Status from Stack
static void ROL(uint8_t *cc); // Rotate One Bit Left (Memory or Accumulator)
static void ROR(uint8_t *cc); // Rotate One Bit Right (Memory or Accumulator)
static void RTI(uint8_t *cc); // Return from Interrupt
static void RTS(uint8_t *cc); // Return from Subroutine
static void SBC(uint8_t *cc); // Subtract Memory from Accumulator with Borrow
static void SEC(uint8_t *cc); // Set Carry Flag
static void SED(uint8_t *cc); // Set Decimal Mode
static void SEI(uint8_t *cc); // Set Interrupt Disable Status
static void STA(uint8_t *cc); // Store Accumulator in Memory
static void STX(uint8_t *cc); // Store Index X in Memory
static void STY(uint8_t *cc); // Store Index Y in Memory
static void TAX(uint8_t *cc); // Transfer Accumulator to Index X
static void TAY(uint8_t *cc); // Transfer Accumulator to Index Y
static void TSX(uint8_t *cc); // Transfer Stack Pointer to Index X
static void TXA(uint8_t *cc); // Transfer Index X to Accumulator
static void TXS(uint8_t *cc); // Transfer Index X to Stack Pointer
static void TYA(uint8_t *cc); // Transfer Index Y to Accumulator
static void JAM(uint8_t *cc);

/* Illegal op-codes */

static void I01(uint8_t *cc);
static void I02(uint8_t *cc);
static void I03(uint8_t *cc);
static void I04(uint8_t *cc);
static void I05(uint8_t *cc);
static void I06(uint8_t *cc);
static void I07(uint8_t *cc);
static void I08(uint8_t *cc);
static void I09(uint8_t *cc);
static void I10(uint8_t *cc);
static void I11(uint8_t *cc);
static void I12(uint8_t *cc);
static void I15(uint8_t *cc);
static void I14(uint8_t *cc);
static void I15(uint8_t *cc);
static void I16(uint8_t *cc);
static void I17(uint8_t *cc);
static void I18(uint8_t *cc);
static void I19(uint8_t *cc);
static void I20(uint8_t *cc);
static void I21(uint8_t *cc);
static void I22(uint8_t *cc);

/*

PC	....	Program Counter
AC	....	Accumulator
XR	....	X Register
YR	....	Y Register
SR	....	Status Register
SP	....	Stack Pointer

The status flags are:
C - Carry: Indicates whether the result of an instruction carries into (ADC), borrows from (SBC), shifts into (ASL,LSR,
    or rotates (ROR,ROL) through the Carry status flag. MOST SIGNIFICANT.
Z - Zero: Indicates whether the arithmetic result of an instruction is zero (0).
I - Interrupt disabled: Indicates whether the processors interrupt signal line is disabled (1) or not (0).
D - Decimal: Indicates whether arithmetic instructions (ADC,SBC) operate in Binary Coded Decimal (BCD) mode or in real
    binary mode.
B - Break: Indicates whether the processor has encountered a BRK instruction.
- - Expansion bit: Not used, and always set to 1.
V - Overflow: Indicates whether the result of an arithmetic instruction (ADC,SBC) either overflows (result greater than +127)
    or underflows (result less than -128). When evaluated after the BIT instruction, indicates whether the sixth (6) bit of the
    operand is one (1) or zero (0).
N - Negative: Indicates whether the result of an arithmetic (ADC,SBC) or incremental (INC,INX,INY,DEC,DEX,DEY) instruction is
    positive (0) or negative (1) as reflected in bit seven (7) of the result.

*/

/* cpu always reads 2 cycles ! sometimes this fact is used to save cycles. e.g. set RTI to 0xDC0C and then the interrupt will be automatically cleared */

#define IMP 0
#define ACC 1
#define IMM 2
#define ABS 3
#define ABX 4
#define ABY 5
#define ZP  6
#define ZPX 7
#define ZPY 8
#define REL 9
#define IND 10
#define IZX 11
#define IZY 12

#define FLAG_CARRY      0x01
#define FLAG_ZERO       0x02
#define FLAG_INTERRUPT  0x04
#define FLAG_DECIMAL    0x08
#define FLAG_BREAK      0x10
#define FLAG_EXPANSION  0x20
#define FLAG_OVERFLOW   0x40
#define FLAG_NEGATIVE   0x80

#define IRQ_VECTOR      0xFFFE
#define NMI_VECTOR      0xFFFA
#define RST_VECTOR      0xFFFC

#define LOGIC_AND       0
#define LOGIC_OR        1
#define LOGIC_EOR       2

/* Bug in 6510 cpu makes the high byte unable to cross boundery */
#define GET_BYTE(am, baa, cc) \
{ \
  switch(am) \
  { \
  case IMM: \
    g_cpu.PC++; \
    baa = *g_cpu.PC; \
    break; \
  case ABS: \
    g_cpu.PC++; \
    baa = bus_read_byte(*g_cpu.PC + (*(g_cpu.PC + 1) << 8)); \
    g_cpu.PC++; \
    break; \
  case ABX: \
    g_cpu.PC++; \
    baa = bus_read_byte(*g_cpu.PC + (*(g_cpu.PC + 1) << 8) + g_cpu.XR); \
    g_cpu.PC++; \
    break; \
  case ABY: \
    g_cpu.PC++; \
    baa = bus_read_byte(*g_cpu.PC + (*(g_cpu.PC + 1) << 8) + g_cpu.YR); \
    g_cpu.PC++; \
    break; \
  case ZP: \
    g_cpu.PC++; \
    baa = bus_read_byte(*g_cpu.PC); \
    break; \
  case ZPX: \
    g_cpu.PC++; \
    baa = bus_read_byte((*g_cpu.PC + g_cpu.XR) & 0xFF); \
    if(((*g_cpu.PC + g_cpu.XR) & 0xFF) < (*g_cpu.PC & 0xFF)) \
      *cc += 1; \
    break; \
  case ZPY: \
    g_cpu.PC++; \
    baa = bus_read_byte((*g_cpu.PC + g_cpu.YR) & 0xFF); \
    if(((*g_cpu.PC + g_cpu.YR) & 0xFF) < (*g_cpu.PC & 0xFF)) \
      *cc += 1; \
    break; \
  case IZX: \
    g_cpu.PC++; \
    baa = bus_read_byte((bus_read_byte((*g_cpu.PC + 1 + g_cpu.XR) & 0xFF) << 8) + bus_read_byte(*g_cpu.PC + g_cpu.XR)); \
    break; \
  case IZY: \
    { \
      uint16_t a; \
      g_cpu.PC++; \
      a = (bus_read_byte((*g_cpu.PC + 1) & 0xFF) << 8) + bus_read_byte(*g_cpu.PC) + g_cpu.YR; \
      baa = bus_read_byte(a); \
      if((a & 0xFF) < ((a - g_cpu.YR) & 0xFF)) \
        *cc += 1; \
    } \
    break; \
  } \
}

// Bug in 6510 cpu makes the high byte unable to cross boundery
#define GET_ADDRESS(am, a, cc) \
{ \
  switch(am) \
  { \
  case ABS: \
    g_cpu.PC++; \
    a = *g_cpu.PC + (*(g_cpu.PC + 1) << 8); \
    g_cpu.PC++; \
    break; \
  case ABX: \
    g_cpu.PC++; \
    a = *g_cpu.PC + (*(g_cpu.PC + 1) << 8) + g_cpu.XR; \
    g_cpu.PC++; \
    break; \
  case ABY: \
    g_cpu.PC++; \
    a = *g_cpu.PC + (*(g_cpu.PC + 1) << 8) + g_cpu.YR; \
    g_cpu.PC++; \
    break; \
  case ZP: \
    g_cpu.PC++; \
    a = *g_cpu.PC; \
    break; \
  case ZPX: \
    g_cpu.PC++; \
    a = (*g_cpu.PC + g_cpu.XR) & 0xFF; \
    if(((a + g_cpu.XR) & 0xFF) < (a & 0xFF)) \
      *cc += 1; \
    break; \
  case ZPY: \
    g_cpu.PC++; \
    a = (*g_cpu.PC + g_cpu.YR) & 0xFF; \
    if(((a + g_cpu.YR) & 0xFF) < (a & 0xFF)) \
      *cc += 1; \
    break; \
  case IZX: \
    g_cpu.PC++; \
    a = (bus_read_byte((*g_cpu.PC + 1 + g_cpu.XR) & 0xFF) << 8) + bus_read_byte(*g_cpu.PC + g_cpu.XR); \
    break; \
  case IZY: \
    g_cpu.PC++; \
    a = (bus_read_byte((*g_cpu.PC + 1) & 0xFF) << 8) + bus_read_byte(*g_cpu.PC) + g_cpu.YR; \
    if((a & 0xFF) < ((a - g_cpu.YR) & 0xFF)) \
      *cc += 1; \
    break; \
  case IND: \
    g_cpu.PC++; \
    a = (bus_read_byte((*(g_cpu.PC + 1) << 8) + ((*g_cpu.PC + 1) & 0xFF)) << 8) + bus_read_byte((*(g_cpu.PC + 1) << 8) + *g_cpu.PC); \
    g_cpu.PC++; \
    break; \
  } \
}

#define ZERO_FLAG(val) \
{ \
  if((val & 0xFF) == 0) \
  { \
    g_cpu.SR |= FLAG_ZERO; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_ZERO; \
  } \
}

#define ZERO_FLAG_CMP(val, reg) \
{ \
  if(reg == val) \
  { \
    g_cpu.SR |= FLAG_ZERO; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_ZERO; \
  } \
}

#define ZERO_FLAG_BIT(val) \
{ \
  if((g_cpu.AC & val) == 0) \
  { \
    g_cpu.SR |= FLAG_ZERO; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_ZERO; \
  } \
}

#define NEGATIVE_FLAG(val) \
{ \
  if(val & 0x80) \
  { \
    g_cpu.SR |= FLAG_NEGATIVE; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_NEGATIVE; \
  } \
}

#define CARRY_FLAG(val) \
{ \
  if(val > 0xFF) \
  { \
    g_cpu.SR |= FLAG_CARRY; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_CARRY; \
  } \
}

#define CARRY_FLAG_CMP(val, reg) \
{ \
  if(reg >= val) \
  { \
    g_cpu.SR |= FLAG_CARRY; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_CARRY; \
  } \
}

#define CARRY_FLAG_SBC(val) \
{ \
  if(val <= 0xFF) \
  { \
    g_cpu.SR |= FLAG_CARRY; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_CARRY; \
  } \
}

#define OVERFLOW_FLAG(reg, baa, res) \
{ \
  if( \
    (((reg & 0x80) == 0x0) && ((baa & 0x80) == 0x0) && ((res & 0x80) == 0x80)) ||  \
    (((reg & 0x80) == 0x80) && ((baa & 0x80) == 0x80) && ((res & 0x80) == 0x00))  \
    ) \
  { \
    g_cpu.SR |= FLAG_OVERFLOW; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_OVERFLOW; \
  } \
}

#define OVERFLOW_FLAG_BIT(val) \
{ \
  if((val & 0x40) == 0x40) \
  { \
    g_cpu.SR |= FLAG_OVERFLOW; \
  } \
  else \
  { \
    g_cpu.SR &= ~FLAG_OVERFLOW; \
  } \
}

typedef struct
{
  uint8_t *PC;
  uint8_t AC;
  uint8_t XR;
  uint8_t YR;
  uint8_t SR;
  uint8_t SP;
  uint8_t pc_inc;
} cpu_t;

extern memory_t g_memory; /* Memory interface */
extern if_host_t g_if_host; /* Main interface */

cpu_on_chip_port_t g_cpu_on_chip_port;

static void (*g_op_code_array[])(uint8_t *cc) =
{
  BRK, ORA, JAM, I01, I20, ORA, ASL, I01, PHP, ORA, ASL, I02, I21, ORA, ASL, I01,
  BPL, ORA, JAM, I01, I20, ORA, ASL, I01, CLC, ORA, NOP, I01, I21, ORA, ASL, I01,
  JSR, AND, JAM, I03, BIT, AND, ROL, I03, PLP, AND, ROL, I02, BIT, AND, ROL, I03,
  BMI, AND, JAM, I03, I20, AND, ROL, I03, SEC, AND, NOP, I03, I21, AND, ROL, I03,
  RTI, EOR, JAM, I05, I20, EOR, LSR, I05, PHA, EOR, LSR, I04, JMP, EOR, LSR, I05,
  BVC, EOR, JAM, I05, I20, EOR, LSR, I05, CLI, EOR, NOP, I05, I21, EOR, LSR, I05,
  RTS, ADC, JAM, I06, I20, ADC, ROR, I06, PLA, ADC, ROR, I07, JMP, ADC, ROR, I06,
  BVS, ADC, JAM, I06, I20, ADC, ROR, I06, SEI, ADC, NOP, I06, I21, ADC, ROR, I06,
  I20, STA, I20, I08, STY, STA, STX, I08, DEY, NOP, TXA, I17, STY, STA, STX, I08,
  BCC, STA, JAM, I09, STY, STA, STX, I08, TYA, STA, TXS, I10, I18, STA, I11, I09,
  LDY, LDA, LDX, I12, LDY, LDA, LDX, I12, TAY, LDA, TAX, I22, LDY, LDA, LDX, I12,
  BCS, LDA, JAM, I12, LDY, LDA, LDX, I12, CLV, LDA, TSX, I19, LDY, LDA, LDX, I12,
  CPY, CMP, I20, I15, CPY, CMP, DEC, I15, INY, CMP, DEX, I14, CPY, CMP, DEC, I15,
  BNE, CMP, JAM, I15, I20, CMP, DEC, I15, CLD, CMP, NOP, I15, I21, CMP, DEC, I15,
  CPX, SBC, I20, I16, CPX, SBC, INC, I16, INX, SBC, NOP, SBC, CPX, SBC, INC, I16,
  BEQ, SBC, JAM, I16, I20, SBC, INC, I16, SED, SBC, NOP, I16, I21, SBC, INC, I16
};

static uint8_t g_inst_cycles[0x100] =
{
  7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
  2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
  2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

static uint8_t g_op_code_add_mode_array[]=
{ 
  IMM, IZX, IMP, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
  ABS, IZX, IMP, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
  IMP, IZX, IMP, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, ACC, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
  IMP, IZX, IMP, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, ACC, IMM, IND, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
  IMM, IZX, IMM, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, ABX, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABY, ABX, ABX, ABY,
  IMM, IZX, IMM, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPY, ZPY, IMP, ABY, IMP, ABY, ABX, ABX, ABY, ABY,
  IMM, IZX, IMM, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX,
  IMM, IZX, IMM, IZX, ZP,  ZP,  ZP,  ZP,  IMP, IMM, IMP, IMM, ABS, ABS, ABS, ABS,
  REL, IZY, IMP, IZY, ZPX, ZPX, ZPX, ZPX, IMP, ABY, IMP, ABY, ABX, ABX, ABX, ABX
};

static cpu_t g_cpu;
static uint8_t g_nmi_triggered; /* NMI is triggered only HIGH to LOW */

static uint8_t get_address_mode(uint8_t op_code)
{
  return g_op_code_add_mode_array[op_code & 0xFF];
}

static void branch(uint8_t branch, uint8_t *cc)
{
  uint8_t byte = 0;

  g_cpu.PC++;

  /*
   * A branch not taken requires two machine cycles. Add one if the branch is taken and add
   * one more if the branch crosses a page boundary.
   */

  if(branch)
  {
    byte = *g_cpu.PC;
    g_cpu.PC++;

    if((byte & 0x80) == 0x80) /* Means branching backward */
    {
      uint16_t tmp = *g_cpu.PC;
      g_cpu.PC -= (uint8_t)(~byte + 1);

      if((tmp & 0xFF00) != (*g_cpu.PC & 0xFF00)) /* Page cross */
      {
        *cc += 2;
      }
      else
      {
        *cc += 1;
      }
    }
    else
    {
      uint16_t tmp = *g_cpu.PC;
      g_cpu.PC += byte;

      if((tmp & 0xFF00) != (*g_cpu.PC & 0xFF00)) /* Page cross */
      {
        *cc += 2;
      }
      else
      {
        *cc += 1;
      }
    }

    g_cpu.pc_inc = 0;
  }
}

static void logical(uint8_t logic, uint8_t *cc)
{
  uint8_t byte_at_addr = 0;

  GET_BYTE(get_address_mode(*g_cpu.PC), byte_at_addr, cc);

  if(logic == LOGIC_AND)
  {
    g_cpu.AC &= byte_at_addr;
  }
  else if(logic == LOGIC_OR)
  {
    g_cpu.AC |= byte_at_addr;
  }
  else if(logic == LOGIC_EOR)
  {
    g_cpu.AC ^= byte_at_addr;
  }

  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);
}

static void flag(uint8_t value, uint8_t flag, uint8_t *cc)
{
  if(value == 0)
  {
    g_cpu.SR &= ~flag;
  }
  else
  {
    g_cpu.SR |= flag;
  }
}

static void ADC(uint8_t *cc)
{
  uint8_t byte_at_addr = 0;
  uint16_t result = 0;
  uint8_t carry_bit = 0;

  GET_BYTE(get_address_mode(*g_cpu.PC), byte_at_addr, cc);

  if((g_cpu.SR & FLAG_DECIMAL) == 0x00)
  {

    if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY)
    {
      carry_bit = 0x1;
    }

    result = g_cpu.AC + byte_at_addr + carry_bit;

    ZERO_FLAG(result);
    CARRY_FLAG(result);
    OVERFLOW_FLAG(g_cpu.AC, byte_at_addr, result);
    NEGATIVE_FLAG(result);

    g_cpu.AC = result & 0xFF;
  }
  else
  {
    uint16_t al;
    uint16_t ah;

    al = (g_cpu.AC & 0x0F) + (byte_at_addr & 0x0F) + ((g_cpu.SR & FLAG_CARRY) ? 1 : 0); /* Calculate lower nybble */

    if(al > 9)
    {
      al += 6; /* BCD fixup for lower nybble */
    }

    ah = (g_cpu.AC >> 4) + (byte_at_addr >> 4); /* Calculate upper nybble */
    if(al > 0x0F)
    {
      ah++;
    }

    if(g_cpu.AC + byte_at_addr + ((g_cpu.SR & FLAG_CARRY) ? 1 : 0))
    {
      g_cpu.SR |= FLAG_ZERO;
    }
    else
    {
      g_cpu.SR &= ~FLAG_ZERO;
    }
    if(ah << 4)
    {
      g_cpu.SR |= FLAG_NEGATIVE;
    }
    else
    {
      g_cpu.SR &= ~FLAG_NEGATIVE;
    }
    if((((ah << 4) ^ g_cpu.AC) & 0x80) && !((g_cpu.AC ^ byte_at_addr) & 0x80))
    {
      g_cpu.SR |= FLAG_OVERFLOW;
    }
    else
    {
      g_cpu.SR &= ~FLAG_OVERFLOW;
    }

    if (ah > 9) ah += 6; /* BCD fixup for upper nybble */

    if(ah > 0x0F)
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }

    g_cpu.AC = (ah << 4) | (al & 0x0f);// Compose result 
  }
}

static void AND(uint8_t *cc)
{
  logical(LOGIC_AND, cc);
}

static void ASL(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  if(address_mode == ACC)
  {
    result = g_cpu.AC << 1;
    g_cpu.AC = result & 0xFF;
  }
  else
  {
    byte_at_addr = bus_read_byte(address);
    /* RMW instructions for NMOS version is doing extra one write */
    bus_write_byte(address, byte_at_addr);
    result = byte_at_addr << 1;
    bus_write_byte(address, result & 0xFF);
  }

  ZERO_FLAG(result);
  CARRY_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void BCC(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_CARRY) == 0x0, cc);
}

static void BCS(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY, cc);
}

static void BEQ(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_ZERO) == FLAG_ZERO, cc);
}

static void BIT(uint8_t *cc)
{
  uint8_t byte_at_addr = 0;

  GET_BYTE(get_address_mode(*g_cpu.PC), byte_at_addr, cc);

  ZERO_FLAG_BIT(byte_at_addr);
  OVERFLOW_FLAG_BIT(byte_at_addr);
  NEGATIVE_FLAG(byte_at_addr);
}

static void BMI(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_NEGATIVE) == FLAG_NEGATIVE, cc);
}

static void BNE(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_ZERO) == 0x0, cc);
}

static void BPL(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_NEGATIVE) == 0x0, cc);
}

static void BRK(uint8_t *cc)
{
  if(!(g_cpu.SR & FLAG_INTERRUPT))
  {
    IRQ(cc, INTR_IRQ, FLAG_BREAK);
    g_if_host.if_host_printer.print_fp("(CC) CPU break!", PRINT_TYPE_ERROR);
  }
}

static void BVC(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_OVERFLOW) == 0x0, cc);
}

static void BVS(uint8_t *cc)
{
  branch((g_cpu.SR & FLAG_OVERFLOW) == FLAG_OVERFLOW, cc);
}

static void CLC(uint8_t *cc)
{
  flag(0, FLAG_CARRY, cc);
}

static void CLD(uint8_t *cc)
{
  flag(0, FLAG_DECIMAL, cc);
}

static void CLI(uint8_t *cc)
{
  flag(0, FLAG_INTERRUPT, cc);
}

static void CLV(uint8_t *cc)
{
  flag(0, FLAG_OVERFLOW, cc);
}

static void CMP(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  result = g_cpu.AC - byte_at_addr;

  ZERO_FLAG_CMP(byte_at_addr, g_cpu.AC);
  CARRY_FLAG_CMP(byte_at_addr, g_cpu.AC);
  NEGATIVE_FLAG(result);
}

static void CPX(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  result = g_cpu.XR - byte_at_addr;

  ZERO_FLAG_CMP(byte_at_addr, g_cpu.XR);
  CARRY_FLAG_CMP(byte_at_addr, g_cpu.XR);
  NEGATIVE_FLAG(result);
}

static void CPY(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  result = g_cpu.YR - byte_at_addr;

  ZERO_FLAG_CMP(byte_at_addr, g_cpu.YR);
  CARRY_FLAG_CMP(byte_at_addr, g_cpu.YR);
  NEGATIVE_FLAG(result);
}

static void DEC(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  byte_at_addr = bus_read_byte(address);
  /* RMW instructions for NMOS version is doing extra one write */
  bus_write_byte(address, byte_at_addr);
  result = byte_at_addr - 1;
  bus_write_byte(address, result & 0xFF);

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void DEX(uint8_t *cc)
{
  g_cpu.XR -= 1;

  ZERO_FLAG(g_cpu.XR);
  NEGATIVE_FLAG(g_cpu.XR);
}

static void DEY(uint8_t *cc)
{
  g_cpu.YR -= 1;

  ZERO_FLAG(g_cpu.YR);
  NEGATIVE_FLAG(g_cpu.YR);
}

static void EOR(uint8_t *cc)
{
  return logical(LOGIC_EOR, cc);
}

static void INC(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  byte_at_addr = bus_read_byte(address);

  /* RMW instructions for NMOS version is doing extra one write */
  bus_write_byte(address, byte_at_addr);
  result = byte_at_addr + 1;
  bus_write_byte(address, result & 0xFF);

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void INX(uint8_t *cc)
{
  g_cpu.XR += 1;

  ZERO_FLAG(g_cpu.XR);
  NEGATIVE_FLAG(g_cpu.XR);
}

static void INY(uint8_t *cc)
{
  g_cpu.YR += 1;

  ZERO_FLAG(g_cpu.YR);
  NEGATIVE_FLAG(g_cpu.YR);
}

static void JMP(uint8_t *cc)
{
  uint8_t address_mode;
  uint16_t address = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  g_cpu.PC = bus_translate_emu_to_host_addr(address);

  g_cpu.pc_inc = 0;
}

static void JSR(uint8_t *cc)
{
  uint16_t emulated_address;
  uint16_t address = 0;

  g_cpu.PC++;
  address = *g_cpu.PC + (*(g_cpu.PC + 1) << 8);
  g_cpu.PC++;

  emulated_address = (uint32_t)g_cpu.PC & 0xFFFF;

  bus_write_byte(g_cpu.SP + OFFSET_STACK, (emulated_address & 0xFF00) >> 8);
  g_cpu.SP--;
  bus_write_byte(g_cpu.SP + OFFSET_STACK, emulated_address & 0xFF);
  g_cpu.SP--;

  g_cpu.PC = bus_translate_emu_to_host_addr(address);

  g_cpu.pc_inc = 0;
}

static void LDA(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  g_cpu.AC = byte_at_addr;

  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);
}

static void LDX(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  g_cpu.XR = byte_at_addr;

  ZERO_FLAG(g_cpu.XR);
  NEGATIVE_FLAG(g_cpu.XR);
}

static void LDY(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_BYTE(address_mode, byte_at_addr, cc);

  g_cpu.YR = byte_at_addr;

  ZERO_FLAG(g_cpu.YR);
  NEGATIVE_FLAG(g_cpu.YR);
}

static void LSR(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  if(address_mode == ACC)
  {
    result = g_cpu.AC >> 1;
    if((g_cpu.AC & 0x01) == 0x01)
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }

    g_cpu.AC = result & 0xFF;
  }
  else
  {
    byte_at_addr = bus_read_byte(address);
    /* RMW instructions for NMOS version is doing extra one write */
    bus_write_byte(address, byte_at_addr);
    result = byte_at_addr >> 1;
    if((byte_at_addr & 0x01) == 0x01)
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }

    bus_write_byte(address, result & 0xFF);
  }

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void NOP(uint8_t *cc)
{
  ;
}

static void ORA(uint8_t *cc)
{
  return logical(LOGIC_OR, cc);
}

static void PHA(uint8_t *cc)
{
  bus_write_byte(g_cpu.SP + OFFSET_STACK, g_cpu.AC);
  g_cpu.SP--;
}

static void PHP(uint8_t *cc)
{
  bus_write_byte(g_cpu.SP + OFFSET_STACK, g_cpu.SR | FLAG_BREAK);
  g_cpu.SP--;
}

static void PLA(uint8_t *cc)
{
  g_cpu.SP++;
  g_cpu.AC = bus_read_byte(g_cpu.SP + OFFSET_STACK);

  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);
}

static void PLP(uint8_t *cc)
{
  g_cpu.SP++;
  g_cpu.SR = bus_read_byte(g_cpu.SP + OFFSET_STACK);

  g_cpu.SR &= ~FLAG_BREAK; /* Flag does not exist in cpu register, only in stack */
}

static void ROL(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  if(address_mode == ACC)
  {
    result = g_cpu.AC << 1;
    if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY)
    {
      result += 0x1;
    }

    g_cpu.AC = result & 0xFF;
  }
  else
  {
    byte_at_addr = bus_read_byte(address);
    result = byte_at_addr << 1;
    if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY)
    {
      result += 0x1;
    }
    bus_write_byte(address, result & 0xFF);
  }

  ZERO_FLAG(result);
  CARRY_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void ROR(uint8_t *cc)
{
  uint8_t address_mode;
  uint8_t byte_at_addr = 0;
  uint16_t address = 0;
  uint16_t result = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  if(address_mode == ACC)
  {
    result = g_cpu.AC >> 1;

    if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY) /* Roll in flag carry */
    {
      result += 0x80;
    }

    if((g_cpu.AC & 0x01) == 0x01) /* Roll out flag carry */
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }

    g_cpu.AC = result & 0xFF;
  }
  else
  {
    byte_at_addr = bus_read_byte(address);
    result = byte_at_addr >> 1;
    if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY) /* Roll in flag carry */
    {
      result += 0x80;
    }

    if((byte_at_addr & 0x01) == 0x01) /* Roll out flag carry */
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }


    bus_write_byte(address, result & 0xFF);
  }

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void RTI(uint8_t *cc)
{
  uint16_t address = 0;

  g_cpu.SP++;
  g_cpu.SR = bus_read_byte(g_cpu.SP + OFFSET_STACK);

  /* Calculate the return address */
  g_cpu.SP++;
  address = (bus_read_byte(((g_cpu.SP + 1) & 0xFF) + OFFSET_STACK) << 8) + bus_read_byte(g_cpu.SP + OFFSET_STACK);
  g_cpu.SP++;

  g_cpu.PC = bus_translate_emu_to_host_addr(address); /* Goto address */

  g_cpu.pc_inc = 0;
}

static void RTS(uint8_t *cc)
{
  uint16_t address = 0;

  /* Calculate the return address */
  g_cpu.SP++;
  address = (bus_read_byte(((g_cpu.SP + 1) & 0xFF) + OFFSET_STACK) << 8) + bus_read_byte(g_cpu.SP + OFFSET_STACK);
  g_cpu.SP++;

  g_cpu.PC = bus_translate_emu_to_host_addr(address); /* Goto address */
}

static void SBC(uint8_t *cc) // Subtract Memory from Accumulator with Borrow
{
  uint8_t byte_at_addr = 0;
  uint16_t result = 0;

  GET_BYTE(get_address_mode(*g_cpu.PC), byte_at_addr, cc);

  if((g_cpu.SR & FLAG_DECIMAL) == 0x00)
  {
    if((g_cpu.SR & FLAG_CARRY) == 0x0)
    {
      result = g_cpu.AC + (~byte_at_addr); // This CPU will handle this in inverse, so carry will be negative
    }
    else
    {
      result = g_cpu.AC + (~byte_at_addr + 1);
    }

    ZERO_FLAG(result);
    CARRY_FLAG_SBC(result);
    OVERFLOW_FLAG(g_cpu.AC, byte_at_addr, result);
    NEGATIVE_FLAG(result);

    g_cpu.AC = result & 0xFF;
  }
  else
  {
    uint16_t tmp;
    uint16_t al;
    uint16_t ah;

    tmp = g_cpu.AC - byte_at_addr - ((g_cpu.SR & FLAG_CARRY) ? 0 : 1);

    al = (g_cpu.AC & 0x0f) - (byte_at_addr & 0x0f) - ((g_cpu.SR & FLAG_CARRY) ? 0 : 1); /* Calculate lower nybble */
    ah = (g_cpu.AC >> 4) - (byte_at_addr >> 4); /* Calculate upper nybble */

    if (al & 0x10)
    {
      al -= 6; /* BCD fixup for lower nybble */
      ah--; 
    }

    if(tmp < 0x100)
    {
      g_cpu.SR |= FLAG_CARRY;
    }
    else
    {
      g_cpu.SR &= ~FLAG_CARRY;
    }

    if(((g_cpu.AC ^ tmp) & 0x80) && ((g_cpu.AC ^ byte_at_addr) & 0x80))
    {
      g_cpu.SR |= FLAG_OVERFLOW;
    }
    else
    {
      g_cpu.SR &= ~FLAG_OVERFLOW;
    }

    if(tmp)
    {
      g_cpu.SR |= FLAG_NEGATIVE;
      g_cpu.SR |= FLAG_ZERO;
    }
    else
    {
      g_cpu.SR &= ~FLAG_NEGATIVE;
      g_cpu.SR &= ~FLAG_ZERO;
    }

    g_cpu.AC = (ah << 4) | (al & 0x0f);
  }
}

static void SEC(uint8_t *cc)
{
  flag(1, FLAG_CARRY, cc);
}

static void SED(uint8_t *cc)
{
  flag(1, FLAG_DECIMAL, cc);
}

static void SEI(uint8_t *cc)
{
  flag(1, FLAG_INTERRUPT, cc);
}

static void STA(uint8_t *cc)
{
  uint8_t address_mode;
  uint16_t address = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  bus_write_byte(address, g_cpu.AC);
}

static void STX(uint8_t *cc)
{
  uint8_t address_mode;
  uint16_t address = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  bus_write_byte(address, g_cpu.XR);
}

static void STY(uint8_t *cc)
{
  uint8_t address_mode;
  uint16_t address = 0;

  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  bus_write_byte(address, g_cpu.YR);
}

static void TAX(uint8_t *cc)
{
  g_cpu.XR = g_cpu.AC;

  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);
}

static void TAY(uint8_t *cc)
{
  g_cpu.YR = g_cpu.AC;

  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);
}

static void TSX(uint8_t *cc)
{
  g_cpu.XR = g_cpu.SP;

  ZERO_FLAG(g_cpu.SP);
  NEGATIVE_FLAG(g_cpu.SP);
}

static void TXA(uint8_t *cc)
{
  g_cpu.AC = g_cpu.XR;

  ZERO_FLAG(g_cpu.XR);
  NEGATIVE_FLAG(g_cpu.XR);
}

static void TXS(uint8_t *cc)
{
  g_cpu.SP = g_cpu.XR;
}

static void TYA(uint8_t *cc)
{
  g_cpu.AC = g_cpu.YR;

  ZERO_FLAG(g_cpu.YR);
  NEGATIVE_FLAG(g_cpu.YR);
}

static void JAM(uint8_t *cc)
{
  g_if_host.if_host_printer.print_fp("(CC) CPU jammed!", PRINT_TYPE_ERROR);
}

/* ILLEGAL OPS */

static void I01(uint8_t *cc) /* aka slo */
{
  ASL(cc);
  ORA(cc);
}

static void I02(uint8_t *cc) /* aka anc */
{
  logical(LOGIC_AND, cc);

  /* Carry flag will be set to same state as negative flag */
  if(g_cpu.SR & FLAG_NEGATIVE)
  {
    g_cpu.SR |= FLAG_CARRY;
  }
  else
  {
    g_cpu.SR &= ~FLAG_CARRY;
  }
}

static void I03(uint8_t *cc) /* aka rla */
{
  ROL(cc);
  AND(cc);
}

static void I04(uint8_t *cc) /* aka alr */
{
  uint16_t result = 0;

  AND(cc);

  /* LSR */
  result = g_cpu.AC >> 1;
  if((g_cpu.AC & 0x01) == 0x01)
  {
    g_cpu.SR |= FLAG_CARRY;
  }
  else
  {
    g_cpu.SR &= ~FLAG_CARRY;
  }

  g_cpu.AC = result & 0xFF;

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void I05(uint8_t *cc) /* aka sre */
{
  LSR(cc);
  EOR(cc);
}

static void I06(uint8_t *cc) /* aka rra */
{
  ROR(cc);
  ADC(cc);
}

static void I07(uint8_t *cc) /* aka arr */
{
  uint16_t result = 0;

  AND(cc);
  result = g_cpu.AC >> 1;

  /* ROR */
  if((g_cpu.SR & FLAG_CARRY) == FLAG_CARRY) /* Roll in flag carry */
  {
    result += 0x80;
  }

  if((g_cpu.AC & 0x01) == 0x01) /* Roll out flag carry */
  {
    g_cpu.SR |= FLAG_CARRY;
  }
  else
  {
    g_cpu.SR &= ~FLAG_CARRY;
  }

  g_cpu.AC = result & 0xFF;

  ZERO_FLAG(result);
  NEGATIVE_FLAG(result);
}

static void I08(uint8_t *cc) /* aka axs */
{
  uint8_t sr = g_cpu.SR;

  STX(cc);
  PHA(cc);
  AND(cc);
  STA(cc);
  PLA(cc);

  /* Opcode does not affect status register */
  g_cpu.SR = sr;
}

static void I14(uint8_t *cc) /* aka sax */
{
  uint8_t address_mode;
  uint16_t address = 0;
  uint8_t tmp;

  /* STA */
  address_mode = get_address_mode(*g_cpu.PC);
  GET_ADDRESS(address_mode, address, cc);
  tmp = g_cpu.AC;

  TXA(cc);

  /* AND */
  g_cpu.AC &= tmp;
  ZERO_FLAG(g_cpu.AC);
  NEGATIVE_FLAG(g_cpu.AC);

  SEC(cc);

  SBC(cc);

  TAX(cc);

  /* LDA */
  g_cpu.AC = tmp;
}

static void I09(uint8_t *cc) /* aka ahx / axa */
{
  uint8_t address_mode;
  uint16_t address = 0;
  uint8_t tmp;
  uint8_t acc = g_cpu.AC;
  uint8_t x = g_cpu.XR;
  uint8_t y = g_cpu.YR;
  uint8_t s = g_cpu.SR;

  /* STX */
  tmp = g_cpu.XR;

  PHA(cc);

  /* AND */
  g_cpu.AC &= tmp;

  /* AND */
  address_mode = get_address_mode(*g_cpu.PC);
  GET_ADDRESS(address_mode, address, cc);
  g_cpu.AC &= (address >> 8) + 1;

  STA(cc);
  PLA(cc);

  /* LDX */
  g_cpu.XR = tmp;

  /* These are not affected */
  g_cpu.AC = acc;
  g_cpu.XR = x;
  g_cpu.YR = y;
  g_cpu.SR = s;
}

static void I10(uint8_t *cc) /* aka tas */
{
  uint8_t address_mode;
  uint16_t address = 0;
  uint8_t tmp;
  uint8_t acc = g_cpu.AC;
  uint8_t x = g_cpu.XR;
  uint8_t y = g_cpu.YR;
  uint8_t s = g_cpu.SR;

  /* STX */
  tmp = g_cpu.XR;

  PHA(cc);

  /* AND */
  g_cpu.AC &= tmp;

  TAX(cc);

  TXS(cc);

  /* AND */
  address_mode = get_address_mode(*g_cpu.PC);
  GET_ADDRESS(address_mode, address, cc);
  g_cpu.AC &= (address >> 8) + 1;

  STA(cc);
  PLA(cc);

  g_cpu.XR = tmp;

  /* These are not affected */
  g_cpu.AC = acc;
  g_cpu.XR = x;
  g_cpu.YR = y;
  g_cpu.SR = s;

}

static void I11(uint8_t *cc) /* aka shx / xas */
{
  uint8_t address_mode;
  uint16_t address = 0;
  uint8_t acc = g_cpu.AC;
  uint8_t x = g_cpu.XR;
  uint8_t y = g_cpu.YR;
  uint8_t s = g_cpu.SR;

  PHA(cc);
  TXA(cc);

  /* AND */
  address_mode = get_address_mode(*g_cpu.PC);
  GET_ADDRESS(address_mode, address, cc);
  g_cpu.AC &= (address >> 8) + 1;

  STA(cc);
  PLA(cc);

  /* These are not affected */
  g_cpu.AC = acc;
  g_cpu.XR = x;
  g_cpu.YR = y;
  g_cpu.SR = s;
}

static void I12(uint8_t *cc) /* aka lax */
{
  LDA(cc);
  LDX(cc);
}

static void I15(uint8_t *cc) /* aka dcp */
{
  DEC(cc);
  CMP(cc);
}

static void I16(uint8_t *cc) /* aka isc / ins */
{
  INC(cc);
  SBC(cc);
}

static void I17(uint8_t *cc) /* aka xaa */
{
  TXA(cc);
  AND(cc);
}

static void I18(uint8_t *cc) /* aka shy / say */
{
  uint8_t address_mode;
  uint16_t address = 0;
  uint8_t acc = g_cpu.AC;
  uint8_t x = g_cpu.XR;
  uint8_t y = g_cpu.YR;
  uint8_t s = g_cpu.SR;

  PHA(cc);
  TYA(cc);

  /* AND */
  address_mode = get_address_mode(*g_cpu.PC);
  GET_ADDRESS(address_mode, address, cc);
  g_cpu.AC &= (address >> 8) + 1;

  STA(cc);
  PLA(cc);

  /* These are not affected */
  g_cpu.AC = acc;
  g_cpu.XR = x;
  g_cpu.YR = y;
  g_cpu.SR = s;
}

static void I19(uint8_t *cc) /* aka las */
{
  uint8_t address_mode;
  uint16_t address = 0;
  address_mode = get_address_mode(*g_cpu.PC);

  GET_ADDRESS(address_mode, address, cc);

  g_cpu.AC = bus_read_byte(address) & g_cpu.SP;
  g_cpu.XR = g_cpu.AC;
  g_cpu.SP = g_cpu.AC;
}

static void I20(uint8_t *cc) /* aka skb */
{
  g_cpu.PC++;
}

static void I21(uint8_t *cc) /* aka skw */
{
  g_cpu.PC++;
  g_cpu.PC++;
}

static void I22(uint8_t *cc) /* aka oal */
{
  g_cpu.AC |= 0xEE;
  AND(cc);
  TAX(cc);
}

static void IRQ(uint8_t *cc, uint8_t type, uint8_t brk)
{
  uint16_t emulated_address;
  uint16_t address = 0;
  uint8_t low_byte = 0;
  uint8_t high_byte = 0;

  emulated_address = (uint32_t)g_cpu.PC & 0xFFFF;

  bus_write_byte(g_cpu.SP + OFFSET_STACK, (emulated_address & 0xFF00) >> 8);
  g_cpu.SP--;

  bus_write_byte(g_cpu.SP + OFFSET_STACK, emulated_address & 0xFF);
  g_cpu.SP--;

  if(brk)
  {
    bus_write_byte(g_cpu.SP + OFFSET_STACK, g_cpu.SR | brk);
  }
  else
  {
    bus_write_byte(g_cpu.SP + OFFSET_STACK, g_cpu.SR & ~brk);
  }
  g_cpu.SP--;

  if(type == INTR_NMI)
  {
    low_byte = bus_read_byte(NMI_VECTOR);
    high_byte = bus_read_byte(NMI_VECTOR + 1);
  }
  else
  {
    low_byte = bus_read_byte(IRQ_VECTOR);
    high_byte = bus_read_byte(IRQ_VECTOR + 1);
  }

  /*
   * Should this flag not be set in case of NMI ?
   * Information about this is not consequent.
   */
  g_cpu.SR |= FLAG_INTERRUPT;

  address = (high_byte << 8) + low_byte;

  g_cpu.PC = bus_translate_emu_to_host_addr(address);

  /*
   * The interrupt sequence takes two clocks for internal operations, two to push the return address onto the stack,
   * one to push the processor status register, and two more to get the ISR's beginning address from $FFFE-FFFF
   * (for IRQ) or $FFFA-FFFB (for NMI)-- in that order.
   */
  *cc = 7;
}

static uint8_t event_read_address_zero(uint16_t addr)
{
  return g_cpu_on_chip_port.addr_zero;
}

static uint8_t event_read_address_one(uint16_t addr)
{
  return g_cpu_on_chip_port.addr_one;
}

static void event_write_address_zero(uint16_t addr, uint8_t value)
{
    g_cpu_on_chip_port.addr_zero = value;
}

static void event_write_address_one(uint16_t addr, uint8_t value)
{
  uint8_t saved = g_cpu_on_chip_port.addr_one;
  uint8_t mask = g_cpu_on_chip_port.addr_zero;
  g_cpu_on_chip_port.addr_one &= ~mask; /* Clear the bits allowed to write to */
  g_cpu_on_chip_port.addr_one |= value & mask; /* Write the bits allowed to write to */
  bus_mem_conifig(g_cpu_on_chip_port.addr_one & 0x07);

  /* Inform tap as well! */
  if((g_cpu_on_chip_port.addr_one & MASK_DATASETTE_MOTOR_CTRL) !=
     (saved & MASK_DATASETTE_MOTOR_CTRL))
  {
    tap_set_motor(g_cpu_on_chip_port.addr_one);
  }
}

static void reset(uint16_t reset_vector)
{
  uint16_t reset_vector_address = 0;

  reset_vector_address = bus_read_byte(reset_vector) |
                        (bus_read_byte(reset_vector + 1) << 8);

  g_cpu.PC = bus_translate_emu_to_host_addr(reset_vector_address);

  g_cpu.AC = 0;
  g_cpu.XR = 0;  
  g_cpu.YR = 0;
  g_cpu.SR = FLAG_EXPANSION; /* Always set */
  g_cpu.SP = 0xFF;
  g_cpu.pc_inc = 1;
}

uint32_t cpu_step()
{
  uint8_t cc;
  uint8_t op_code;

  if(!g_nmi_triggered &&
     g_memory.io_p[REG_CIA2_INTERRUPT_CONTROL_REG] & MASK_CIA_INTERRUPT_CONTROL_REG_MULTI)
  {
    g_nmi_triggered = 1;
    IRQ(&cc, INTR_NMI, 0);
    return cc;
  }

  /*
   * Check all interrupt sources and interrupt cpu if set.
   * Do not forget to check so that no nmi is running and that
   * interrupt flag is not set.
   */
  if(!(g_cpu.SR & FLAG_INTERRUPT) &&
    ((g_memory.io_p[REG_INTRRUPT] & MASK_INTRRUPT_IRQ) ||
     (g_memory.io_p[REG_CIA1_INTERRUPT_CONTROL_REG] & MASK_CIA_INTERRUPT_CONTROL_REG_MULTI)))
  {
    IRQ(&cc, INTR_IRQ, 0);
    return cc;
  }

  /* CPU always reads 2 bytes! */
  op_code = *g_cpu.PC;
  (void)bus_read_byte((uint32_t)(g_cpu.PC + 1) & 0xFFFF);
  cc = g_inst_cycles[op_code];
  g_op_code_array[op_code](&cc); /* Execute opcode */

  if(g_cpu.pc_inc)
  {
    g_cpu.PC++;
  }
  else
  {
    g_cpu.pc_inc = 1;
  }

  return cc;
}

void cpu_init()
{
  reset(RST_VECTOR);

  /*
   * Cpu has responsibility for the two first address in zero page.
   * These addresses are hard wired to the cpu on-chip port and has
   * no memory attached to it when read and written by g_cpu. However,
   * it is possible to read these two addresses from ram as well
   * (and even write to them) using vic but this is another story.
   */
  bus_event_read_subscribe(0x0000, event_read_address_zero);
  bus_event_read_subscribe(0x0001, event_read_address_one);
  bus_event_write_subscribe(0x0000, event_write_address_zero);
  bus_event_write_subscribe(0x0001, event_write_address_one);

  g_cpu_on_chip_port.addr_zero = PORT_DIRECTION_DEFAULT;
  g_cpu_on_chip_port.addr_one = PORT_DEFAULT;
}

void cpu_reset()
{
  reset(RST_VECTOR);  
}

void cpu_clear_nmi()
{
  g_nmi_triggered = 0;
}

uint8_t cpu_get_nmi()
{
  return g_nmi_triggered;
}
