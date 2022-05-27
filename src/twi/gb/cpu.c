//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi-gb.
//
// twi-gb is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// twi-gb is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with twi-gb. If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_CPU_C
#define TWI_GB_CPU_C

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <twi/gb/cpu.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/std/assertf.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private constant definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Macros to abstract CPU register access.
// Registers that the CPU may pair together for specific instructions
// must respect the host machine endianness, or behavior will be undefined.
// Pairs are: AF, BC, DE, HL
// The first register in a pair is the higher order register,
// and the second is, then, the lower order register.
//=======================================================================

// 8-bit register indices
// Register order is dependent on host endianness, to correctly emulate
// the endianness of 16-bit register pairs.
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	// High order first, low order second
#	define IDX_A 0
#	define IDX_F 1
#	define IDX_B 2
#	define IDX_C 3
#	define IDX_D 4
#	define IDX_E 5
#	define IDX_H 6
#	define IDX_L 7
#else // __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
	// Low order first, high order second
#	define IDX_A 1
#	define IDX_F 0
#	define IDX_B 3
#	define IDX_C 2
#	define IDX_D 5
#	define IDX_E 4
#	define IDX_H 7
#	define IDX_L 6
#endif // __BYTE_ORDER__

// 16-bit register indices
#define IDX_AF 0
#define IDX_BC 2
#define IDX_DE 4
#define IDX_HL 6

//=======================================================================
// def FBIT_Z
// def FBIT_N
// def FBIT_H
// def FBIT_CY
//
// Flag register bit offset definitions.
//
// Note: Each operation has its own specific behavior with regards to
// modification of the flag register. The behavior described below
// is an ideal generalization of expected behavior, but always refer to
// an operation's documentation to confirm flag behavior.
//
// Flag bits:
// z: Zero flag bit. Set if result is 0, else reset.
// n: Subtraction flag bit. Set on subtraction, reset on addition.
// h: Half-carry bit. Set on transfer of bits between nybbles.
// cy: Carry bit. Set on overflow of the register.
//=======================================================================
#define FBIT_Z 7
#define FBIT_N 6
#define FBIT_H 5
#define FBIT_CY 4

//=======================================================================
// Macros which exploit patterns in the GB CPU instruction set in order
// to cover a large number of like instructions via a single macro.
// Each of these sets of cases it quite simply known as a "case set".
//
// The arguments which may appear for a case set macro are as follows:
// * offset: The integral constant which triggers the first case of a
//   case set.
// * stride: The integral constant which all cases are the first in the
//   set are incremented.
//   For example, the 2nd case will trigger at (`offset` + `stride`).
//   The 3rd case at  (`offset` + `stride` * 2).
//   And so on.
// * n_offset: The integral constant which triggers the case of a set
//   in which an unsigned 8-bit immediate `n` is the RHS argument.
// * op: The two-operand operation to be executed.
//   Operations should not assign values or otherwise modify variables
//   in macros which accept the `set` argument. If the `set` argument is
//   not present, however, then it is `op`'s responsibility to assign the
//   outcome of the operation to its intended destination.
// * set: An operation which defines how to assign the result of `op`.
//   `set` operations should accept 2 arguments:
//   1. The first argument is the left-hand side argument of the `op` operation.
//   2. The second argument is the result of the `op` operation.
// * lhs: The left-hand side argument for all cases.
// * cyc: The number of cycles consumed by the operation when the
//   right-hand side argument is a register.
// * HLM_cyc: The number of cycles consumed by the operation when the
//   right-hand side argument is memory pointed to by REG_HL.
// * n_cyc: The number of cycles consumed by the operation when the
//   right-hand side argument is an unsigned 8-bit immediate.
// * ib: The number of bytes to advance the instruction pointer (PC).
// * ret: The return value at the end of each case statement.
//=======================================================================

// The base case from which all casesets are constructed.
#define CASE(offset, op, set, lhs, rhs, ib, cyc, ret) \
	case offset: set(lhs, op(&REG_F, lhs, rhs)); ADV(ib, cyc); return (ret)

// Defines 7 switch cases for instructions which accept 2 8-bit operands.
//
// The left-hand side argument for each instruction is the value of `lhs`.
//
// The right-hand side argument for each instruction is the value of 
// one of the following 8-bit registers, used in the listed order:
// B, C, D, E, H, L, A
//
// (offset + stride * 6) is skipped when enumerating cases,
// so as to match the pattern used by actual instructions which is
// caseset seeks to imitate.
#define CASESET28_7RN(offset, stride, n_offset, op, set, lhs, cyc, ret) \
	case offset             : set(lhs, op(&REG_F, lhs, REG_B)); ADV(1, cyc); return (ret); \
	case offset + stride    : set(lhs, op(&REG_F, lhs, REG_C)); ADV(1, cyc); return (ret); \
	case offset + stride * 2: set(lhs, op(&REG_F, lhs, REG_D)); ADV(1, cyc); return (ret); \
	case offset + stride * 3: set(lhs, op(&REG_F, lhs, REG_E)); ADV(1, cyc); return (ret); \
	case offset + stride * 4: set(lhs, op(&REG_F, lhs, REG_H)); ADV(1, cyc); return (ret); \
	case offset + stride * 5: set(lhs, op(&REG_F, lhs, REG_L)); ADV(1, cyc); return (ret); \
	case offset + stride * 7: set(lhs, op(&REG_F, lhs, REG_A)); ADV(1, cyc); return (ret); \
	case n_offset           : set(lhs, op(&REG_F, lhs, IMMED8)); ADV(2, cyc+4); return (ret)

// Same as CASESET28_7RN, and handles an additional opcode at (offset + stride * 6)
// when the rhs value originates from memory pointed to by the value of REG_HL.
//
// This version of the CASESET28 macro does not accept cycle arguments
// or a return expression, as the pattern of instruction which this
// macro applies to all share identical values for these arguments.
// Cycles when the right-hand side argument is a...
// 8-bit register: 4
// 16-bit pointer in REG_HL: 8
// Unsigned 8-bit immediate: 8
#define CASESET28_7RHLPN(offset, stride, n_offset, op, set, lhs) \
	CASESET28_7RN(offset, stride, n_offset, op, set, lhs, 4, 0); \
	case offset + stride * 6: set(lhs, op(&REG_F, lhs, twi_gb_mem_read8(MEM_PTR, REG_HL))); ADV(1, 8); return 0

// Defines 8 switch cases for instructions which accept 1 8-bit operand.
// 
// The singular operand for each instruction is the value of one of the
// following, used in the listed order:
// B, C, D, E, H, L, (HL), A
//
// Operations (passed as `op`) should accept 3 arguments (f, lhs, unused),
// as if the operation accepts 2 operands. This is so that all operation
// functions share the same signature, which makes the CPU easier to test.
// The value passed as the 3rd argument is arbitrary and should be ignored.
//
// `cyc` defines the number of cycles consumed
// when the operand is an 8-bit register.
// When the operand is a pointer to memory,
// the number of cycles consumed is (`cyc` + 8).
#define CASESET18_7RHLP(offset, stride, op, ib, cyc) \
	case offset             : REG_B = op(&REG_F, REG_B, 0); ADV(ib, cyc); return 0; \
	case offset + stride    : REG_C = op(&REG_F, REG_C, 0); ADV(ib, cyc); return 0; \
	case offset + stride * 2: REG_D = op(&REG_F, REG_D, 0); ADV(ib, cyc); return 0; \
	case offset + stride * 3: REG_E = op(&REG_F, REG_E, 0); ADV(ib, cyc); return 0; \
	case offset + stride * 4: REG_H = op(&REG_F, REG_H, 0); ADV(ib, cyc); return 0; \
	case offset + stride * 5: REG_L = op(&REG_F, REG_L, 0); ADV(ib, cyc); return 0; \
	case offset + stride * 6: WMEM8(REG_HL, op(&REG_F, twi_gb_mem_read8(MEM_PTR, REG_HL), 0)); ADV(ib, (cyc) + 8); return HL_PTR_RETURN; \
	case offset + stride * 7: REG_A = op(&REG_F, REG_A, 0); ADV(ib, cyc); return 0

// Defines 4 switch cases with user provided operands.
// Supports operations with both 1 and 2 operands.
// Intended for usage with 16-bit instructions.
//
// Due to the inconsistency of instructions "following" this pattern,
// all lhs and rhs arguments are provided by the user.
// Operands are paired with the operand with the like-index.
// Example:
// op(lhs1, rhs1)
// op(lhs2, rhs2)
// and so on.
// 
// No stride argument is accepted as instructions following this
// caseset pattern all share the same stride.
#define CASESET16_4(offset, op, set, lhs1, lhs2, lhs3, lhs4, rhs1, rhs2, rhs3, rhs4, ib, cyc, ret) \
	case offset       : set(lhs1, op(&REG_F, lhs1, rhs1)); ADV(ib, cyc); return (ret); \
	case offset + 0x10: set(lhs2, op(&REG_F, lhs2, rhs2)); ADV(ib, cyc); return (ret); \
	case offset + 0x20: set(lhs3, op(&REG_F, lhs3, rhs3)); ADV(ib, cyc); return (ret); \
	case offset + 0x30: set(lhs4, op(&REG_F, lhs4, rhs4)); ADV(ib, cyc); return (ret)

// A version of CASESET16_4 with the following restrictions:
// * lhs follows the pattern of BC, DE, HL, and a fourth register specified
//   by the user via `case4_lhs`, in that order.
// * A single rhs value, provided by the user, is used by all four cases.
#define CASESET16_4RR(offset, op, set, lhs4, rhs, ib, cyc, ret) \
	CASESET16_4(offset, op, set, REG_BC, REG_DE, REG_HL, lhs4, rhs, rhs, rhs, rhs, ib, cyc, ret)

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================
static inline int_fast8_t
interpret_once(
		struct twi_gb_cpu* restrict,
		struct twi_gb_mem* restrict
);
static char identify_reg8(uint_fast8_t);
static const char* identify_reg16(uint_fast8_t);

//=======================================================================
//-----------------------------------------------------------------------
// Public function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Direct register access macros
// def REG8(): Access 8-bit register contents.
// def REG16(): Access 16-bit register contents.
//-----------------------------------------------------------------------
// Parameters:
// * cpu_ptr: Pointer to twi-gb-cpu which owns the register.
//   Behavior is undefined if `cpu_ptr` does not point to a twi-gb-cpu.
// * idx: Index of the desired register.
//   Behavior is undefined if `idx` does not represent a valid register
//   matching the register type expected by the call signature.
//
// Returns: Direct read/write access to the requested register.
//=======================================================================
#define REG8(cpu_ptr, idx) ((cpu_ptr)->reg[idx])
#define REG16(cpu_ptr, idx) (*((uint16_t*)((cpu_ptr)->reg + (idx))))

//-----------------------------------------------------------------------
// Direct register access shorthand macros
// def REG_A: Access register A.
// def REG_F: Access register F.
// def REG_B: Access register B.
// def REG_C: Access register C.
// def REG_D: Access register D.
// def REG_E: Access register E.
// def REG_H: Access register H.
// def REG_L: Access register L.
// def REG_AF: Access register pair AF.
// def REG_BC: Access register pair BC.
// def REG_DE: Access register pair DE.
// def REG_HL: Access register pair HL.
//
// Perform 0-argument register accesses on the twi-gb-cpu pointed to by
// the alias CPU_PTR.
//-----------------------------------------------------------------------
// 8-bit
#define REG_A REG8(CPU_PTR, IDX_A)
#define REG_F REG8(CPU_PTR, IDX_F)
#define REG_B REG8(CPU_PTR, IDX_B)
#define REG_C REG8(CPU_PTR, IDX_C)
#define REG_D REG8(CPU_PTR, IDX_D)
#define REG_E REG8(CPU_PTR, IDX_E)
#define REG_H REG8(CPU_PTR, IDX_H)
#define REG_L REG8(CPU_PTR, IDX_L)
// 16-bit
#define REG_AF REG16(CPU_PTR, IDX_AF)
#define REG_BC REG16(CPU_PTR, IDX_BC)
#define REG_DE REG16(CPU_PTR, IDX_DE)
#define REG_HL REG16(CPU_PTR, IDX_HL)

//=======================================================================
// Immediate read functions
// def immed8(): Fetch the current instruction's unsigned 8-bit immediate.
// def immed16(): Fetch the current instruction's unsigned 16-bit immediate.
// def immeds8(): Fetch the current instruction's signed 8-bit immediate.
//
// Behavior is undefined if the current instruction within the twi-gb-mem
// pointed to by `mem` pointed to by the twi-gb-cpu pointed to by `cpu`
// does not possess an immediate of size and signedness matching the
// function used to fetch it.
//-----------------------------------------------------------------------
// Parameters:
// * cpu: Pointer to the twi-gb-cpu determining current instruction
//   within the program.
//   Behavior is undefined if `cpu` does not point to a twi-gb-cpu object.
// * mem: Pointer to the twi-gb-mem which contains the program.
//   Behavior is undefined if `mem` does not point to an active twi-gb-mem
//   object.
//
// Returns: The immediate follow of the current instruction within
// `mem` pointed to by `cpu`.
//=======================================================================
static inline uint8_t
immed8(
		const struct twi_gb_cpu* restrict cpu,
		const struct twi_gb_mem* restrict mem
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);
	return twi_gb_mem_read8(mem, cpu->pc + 1);
} // end immed8()
static inline uint16_t
immed16(
		const struct twi_gb_cpu* restrict cpu,
		const struct twi_gb_mem* restrict mem
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);
	return twi_gb_mem_read16(mem, cpu->pc + 1);
} // end immed16()
static inline int8_t
immeds8(
		const struct twi_gb_cpu* restrict cpu,
		const struct twi_gb_mem* restrict mem
) {
	uint8_t u_val = twi_gb_mem_read8(mem, cpu->pc + 1);
	return *((int8_t*)(&u_val));
} // end immeds8()

//-----------------------------------------------------------------------
// Immediate read shorthand macros
// def IMMED8: Read unsigned 8-bit immediate.
// def IMMED16: Read unsigned 16-bit immediate.
// def IMMEDs8: Read signed 8-bit immediate.
//
// 0-argument shorthand immediate access to the instruction pointed to
// by the twi-gb-cpu pointed to by the alias of `CPU_PTR` within the
// program within the twi-gb-mem pointed to by the alias of `MEM_PTR`.
//-----------------------------------------------------------------------
#define IMMED8 immed8(CPU_PTR, MEM_PTR)
#define IMMED16 immed16(CPU_PTR, MEM_PTR)
#define IMMEDs8 immeds8(CPU_PTR, MEM_PTR)

//=======================================================================
// def fset()
//
// Define all flags.
//-----------------------------------------------------------------------
// Parameters:
// * f: Pointer to flag register to set.
//   Behavior is undefined if `f` points to NULL.
// * z: Value of the zero flag bit.
// * n: Value of the subtraction flag bit.
// * h: Value of the half-carry flag bit.
// * cy: Value of the carry flag bit.
//=======================================================================
static inline void
fset(uint8_t* restrict f, bool z, bool n, bool h, bool cy) {
	twi_assert_notnull(f);
	*f = (z << FBIT_Z) | (n << FBIT_N) | (h << FBIT_H) | (cy << FBIT_CY);
} // end fset()

//=======================================================================
// def fget()
//
// Retrieve flag bit value.
//-----------------------------------------------------------------------
// Parameters:
// * f: Pointer to flag register to read.
//   Behavior is undefined if `f` points to NULL.
// * bit: Bit offset of requested flag.
//   Behavior is undefined if `bit` is not a valid flag bit offset.
//
// Returns: True/1 if the requested bit is set, or false/0 if it is reset.
//=======================================================================
static inline bool
fget(uint8_t* restrict f, uint_fast8_t bit) {
	twi_assert_notnull(f);
	twi_assertf(bit == FBIT_Z || bit == FBIT_N || bit == FBIT_H || bit == FBIT_CY,
			"`bit` (underlying value: %d) must be one of: z, n, h, or cy.", bit);
	return (*f & (0x1 << bit));
} // end fget()

//=======================================================================
// def ADV()
//
// Advances the instruction pointer (pc) and cycle counter (div) of
// the twi-gb-cpu pointed to by the alias of `CPU_PTR` by `ib` bytes
// and `cyc` cycles, respectively.
//-----------------------------------------------------------------------
// Parameters:
// * ib: The number of bytes to advance the instruction pointer by.
// * cyc: The number of cycles to advance the CPU clock by.
//=======================================================================
#define ADV(ib, cyc) (CPU_PTR)->pc += ib; (CPU_PTR)->div += cyc

//=======================================================================
// Operation functions.
//
// Behavior is undefined if `f` points to NULL.
//-----------------------------------------------------------------------
// Parameters:
// * f: A pointer to the 8-bit flag register.
// * lhs: The left-hand side 8-bit or 16-bit value.
// * rhs: The right-hand side 8-bit or 16-bit value.
//
// Returns: The result of the operation.
//=======================================================================

//-----------------------------------------------------------------------
// def NOP()
//
// Pseudo-operation. Consume arguments and do nothing.
// Useful for particularly complex instructions, such as PUSH and POP,
// which require greater context and therefore perform their tasks
// within their writeback function instead.
//-----------------------------------------------------------------------
#define NOP(...) ((void)0)

//-----------------------------------------------------------------------
// Load operations
// def LD8()
// def LD16()
//
// Discard lhs and return rhs. No flag changes.
//-----------------------------------------------------------------------
static inline uint8_t LD8(uint8_t* restrict, uint8_t, uint8_t rhs) { return rhs; }
static inline uint16_t LD16(uint8_t* restrict, uint16_t, uint16_t rhs) { return rhs; }

//-----------------------------------------------------------------------
// 8-bit addition operations
// def ADD8(): Add rhs value to lhs value, return sum.
// def ADC(): Add rhs value and carry bit to lhs value, return sum.
//
// Flag changes:
// z: Set if sum is 0, else reset.
// n: Reset.
// h: Set on carry from bit 3, else reset.
// cy: Set if overflow occurs, else reset.
//-----------------------------------------------------------------------
static inline uint8_t ADD8(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint16_t sum = lhs + rhs;
	fset(f, ((uint8_t)sum) == 0, 0, ((lhs & 0xF) + (rhs & 0xF)) > 0xF, sum > 0xFF);
	return (uint8_t)sum;
} // end ADD8()
static inline uint8_t ADC(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	bool cy = fget(f, FBIT_CY);
	uint16_t sum = lhs + rhs + cy;
	fset(f, ((uint8_t)sum) == 0, 0, ((lhs & 0xF) + (rhs & 0xF) + cy) > 0xF, sum > 0xFF);
	return (uint8_t)sum;
} // end ADC()

//-----------------------------------------------------------------------
// 16-bit addition operations
// def ADD16(): Add rhs value to lhs value, return sum.
//
// z: No change.
// n: Reset.
// h: Set on carry from bit 11, else reset.
// cy: Set on carry from bit 15, else reset.
//-----------------------------------------------------------------------
static inline uint16_t ADD16(uint8_t* restrict f, uint16_t lhs, uint16_t rhs) {
	uint32_t sum = lhs + rhs;
	fset(f, fget(f, FBIT_Z), 0, ((lhs & 0xFFF) + (rhs & 0xFFF)) > 0xFFF, sum > 0xFFFF);
	return (uint16_t)sum;
} // end ADD16()

//-----------------------------------------------------------------------
// Subtraction
// def SUB(): Subtract rhs value from lhs value.
// def SBC(): Subtract rhs value and carry bit from lhs value.
// (Note: CP is identical to SUB with its return value discarded).
//
// z: Set if difference is 0, else reset.
// n: Set.
// h: Set on borrow from bit 4, else reset.
// cy: Set on borrow from bit 8, else reset.
//-----------------------------------------------------------------------
static inline uint8_t SUB(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint16_t diff = lhs - rhs;
	fset(f, diff == 0, 1, ((lhs & 0xF) - (rhs - 0xF)) > 0xF, diff > 0xFF);
	return (uint8_t)diff;
} // end SUB()
static inline uint8_t SBC(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	bool cy = fget(f, FBIT_CY);
	uint16_t diff = lhs - rhs - cy;
	fset(f, diff == 0, 1, ((lhs & 0xF) - (rhs & 0xF) - cy) > 0xF, diff > 0xFF);
	return (uint8_t)diff;
} // end SBC()

//-----------------------------------------------------------------------
// Logical operations
// def AND(): Logical AND of lhs and rhs values.
// def XOR(): Logical XOR of lhs and rhs values.
// def OR(): Logical OR of lhs and rhs values.
//
// z: Set if result is 0, else reset.
// n: Reset.
// h: Set for AND, else reset.
// cy: Reset.
//-----------------------------------------------------------------------
static inline uint8_t AND(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs & rhs;
	fset(f, result == 0, 0, 1, 0);
	return result;
} // end AND()
static inline uint8_t XOR(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs ^ rhs;
	fset(f, result == 0, 0, 0, 0);
	return result;
} // end XOR()
static inline uint8_t OR(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs | rhs;
	fset(f, result == 0, 0, 0, 0);
	return result;
} // end OR()

//-----------------------------------------------------------------------
// Incrementation operations
// def INC8()
// def INC16()
//
// Increment lhs value by 1. Rhs value is ignored.
//
// NOTE: 16-bit versions make no flag changes.
// 8-bit flag changes:
// z: Set if result is 0, else reset.
// h: Reset.
// n: Set on carry from bit 3, else reset.
// cy: No change.
//-----------------------------------------------------------------------
static inline uint8_t INC8(uint8_t* restrict f, uint8_t lhs, uint8_t) {
	uint8_t sum = lhs + 1;
	fset(f, sum == 0, 0, (lhs & 0xF) == 0xF, fget(f, FBIT_CY));
	return sum;
} // end INC8()
static inline uint16_t INC16(uint8_t* restrict, uint16_t lhs, uint16_t) {
	return lhs + 1;
} // end INC16()

//-----------------------------------------------------------------------
// Decrementation operations
// def DEC8()
// def DEC16()
//
// Decrement lhs value by 1. Rhs value is ignored.
//
// NOTE: 16-bit versions make no flag changes.
// 8-bit flag changes:
// z: Set if result is 0, else reset.
// h: Set.
// n: Set on borrow from bit 4, else reset.
// cy: No change.
//-----------------------------------------------------------------------
static inline uint8_t DEC8(uint8_t* restrict f, uint8_t lhs, uint8_t) {
	uint8_t diff = lhs - 1;
	fset(f, diff == 0, 1, (lhs & 0xF) == 0, fget(f, FBIT_CY));
	return diff;
} // end DEC8()
static inline uint16_t DEC16(uint8_t* restrict, uint16_t lhs, uint16_t) {
	return lhs - 1;
} // end DEC16()

//=======================================================================
// Writeback macros
//
// Defines write operations in a modular, function-like form.
// Useful in keeping operations separate from writebacks.
//
// Each writeback macro shares an identical signature, but how the
// signature is interpreted is up to the macro.
//
// In general, the lhs argument represents the writeback destination,
// and the rhs argument represents the value to write to the destination.
//=======================================================================

//-----------------------------------------------------------------------
// def WREG()
//
// Writeback to a register (8 or 16-bit).
// reg: Write-able access to a register.
// val: Value to write which matches the width of the destination register.
//-----------------------------------------------------------------------
#define WREG(reg, val) ((reg) = (val))

//-----------------------------------------------------------------------
// def WMEM8()
//
// Writeback to an 8-bit memory location in the twi-gb-mem pointed to
// by the alias MEM_PTR.
// addr: The address to write to.
// val: 8-bit value to write to the `dest` address.
//-----------------------------------------------------------------------
#define WMEM8(addr, val) twi_gb_mem_write8(MEM_PTR, addr, val)

//-----------------------------------------------------------------------
// #define WIGN()
//
// No writeback.
// Source must still be present so as to not remove the computation.
// Primarily for operations such as CP, which modify the flag register
// without writing a value to any destination.
//-----------------------------------------------------------------------
#define WIGN(ignored, op) (op)

//-----------------------------------------------------------------------
// def WPUSH()
//
// Write the contents of the 16-bit register indicated by the first
// argument to the 16-bit memory location within the twi-gb-mem pointed
// to by the alias of MEM_PTR pointed to by the value of the SP register
// contained within the twi-gb-cpu pointed to by the alias of CPU_PTR.
// Then increment that SP register by 2.
//-----------------------------------------------------------------------
#define WPUSH(rr, ignored) \
	twi_gb_mem_write16(MEM_PTR, (CPU_PTR)->sp, rr); \
	(CPU_PTR)->sp += 2

//-----------------------------------------------------------------------
// def WPOP()
//
// Decrement the value of the SP register contained with the twi-gb-cpu
// pointed to by CPU_PTR by 2. Then read 2 bytes from the memory address
// contained with that SP register from the twi-gb-mem pointed to by the
// alias MEM_PTR.
//-----------------------------------------------------------------------
#define WPOP(rr, ignored) \
	(CPU_PTR)->sp -= 2; \
	(rr) = twi_gb_mem_read16(MEM_PTR, (CPU_PTR)->sp)

//=======================================================================
// def HL_PTR_RETURN
//
// Common return expression, used for when writing to a memory address
// pointed to the 16-bit HL register. Signals the caller when the value
// within the HL register contained with the twi-gb-cpu pointed to by the
// alias CPU_PTR was 0xFFFF at the time of the write.
//
// Writing to address 0xFFFF potentially modifies active interupts.
// The main interpreter loop will need to re-evaluate active interupts
// every time the value at 0xFFFF changes.
//=======================================================================
#define HL_PTR_RETURN ((REG_HL == 0xFFFF) ? -1 : 0)

//=======================================================================
// def interpret_once()
// TODO: Description
//=======================================================================
static inline int_fast8_t
interpret_once(
		struct twi_gb_cpu* restrict cpu,
		struct twi_gb_mem* restrict mem
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);
	// Assign cpu and mem pointers to aliases used by macros.
#define CPU_PTR (cpu)
#define MEM_PTR (mem)
	switch (twi_gb_mem_read8(mem, cpu->pc)) {
		case 0x00: ADV(1, 4); return 0; // NOP
		CASESET16_4RR(0x01, LD16, WREG, cpu->sp, IMMED16, 3, 12, 0); // LD rr, nn
		CASESET16_4RR(0x03, INC16, WREG, cpu->sp, 0, 1, 8, 0); // INC rr
		CASESET18_7RHLP(0x04, 8, INC8, 1, 4); // INC r|(HL)
		CASESET18_7RHLP(0x05, 8, DEC8, 1, 4); // DEC r|(HL)
		CASESET16_4(0x09, ADD16, WREG, REG_HL, REG_HL, REG_HL, REG_HL, REG_BC, REG_DE, REG_HL, cpu->sp, 1, 8, 0); // ADD rr, rr
		CASESET16_4RR(0x0B, DEC16, WREG, cpu->sp, 0, 1, 8, 0); // DEC rr
		CASESET28_7RHLPN(0x40, 1, 0x06, LD8, WREG, REG_B); // LD B, r|(HL)|n
		CASESET28_7RHLPN(0x48, 1, 0x0E, LD8, WREG, REG_C); // LD C, r|(HL)|n
		CASESET28_7RHLPN(0x50, 1, 0x16, LD8, WREG, REG_D); // LD D, r|(HL)|n
		CASESET28_7RHLPN(0x58, 1, 0x1E, LD8, WREG, REG_E); // LD E, r|(HL)|n
		CASESET28_7RHLPN(0x60, 1, 0x26, LD8, WREG, REG_H); // LD H, r|(HL)|n
		CASESET28_7RHLPN(0x68, 1, 0x2E, LD8, WREG, REG_L); // LD L, r|(HL)|n
		CASESET28_7RN(0x70, 1, 0x36, LD8, WMEM8, REG_HL, 8, HL_PTR_RETURN); // LD (HL), r|n
		CASESET28_7RHLPN(0x78, 1, 0x3E, LD8, WREG, REG_A); // LD A, r|(HL)|n
		CASESET28_7RHLPN(0x80, 1, 0xC6, ADD8, WREG, REG_A); // ADD A, r|(HL)|n
		CASESET28_7RHLPN(0x88, 1, 0xCE, ADC, WREG, REG_A); // ADC A, r|(HL)|n
		CASESET28_7RHLPN(0x90, 1, 0xD6, SUB, WREG, REG_A); // SUB A, r|(HL)|n
		CASESET28_7RHLPN(0x98, 1, 0xDE, SBC, WREG, REG_A); // SBC A, r|(HL)|n
		CASESET28_7RHLPN(0xA0, 1, 0xE6, AND, WREG, REG_A); // AND A, r|(HL)|n
		CASESET28_7RHLPN(0xA8, 1, 0xEE, XOR, WREG, REG_A); // XOR A, r|(HL)|n
		CASESET28_7RHLPN(0xB0, 1, 0xF6, OR, WREG, REG_A); // OR A, r|(HL)|n
		CASESET28_7RHLPN(0xB8, 1, 0xFE, SUB, WIGN, REG_A); // CP A, r|(HL)|n
		CASESET16_4RR(0xC1, NOP, WPOP, REG_AF, 0, 1, 12, 0); // POP rr
		CASESET16_4RR(0xC5, NOP, WPUSH, REG_AF, 0, 1, 16, (cpu->sp - 2) >= 0xFFFE); // PUSH rr
	} // end operation lookup and execution
#undef CPU_PTR
#undef MEM_PTR
}; // end interpret_once()

//=======================================================================
// def identify_reg8()
//
// Provides the single character representation of the 8-bit register
// indicated by `reg_idx`.
//
// In order to identify 16-bit register pairs, use identify_reg16().
//-----------------------------------------------------------------------
// Parameters:
// * reg_idx: An 8-bit register index.
//
// Returns: The single character identifier of the register specified by
// the provided index, or '?' if the argument is not a valid index.
//=======================================================================
static char
identify_reg8(uint_fast8_t reg_idx) {
	switch (reg_idx) {
		case IDX_A: return 'A';
		case IDX_F: return 'F';
		case IDX_B: return 'B';
		case IDX_C: return 'C';
		case IDX_D: return 'D';
		case IDX_E: return 'E';
		case IDX_H: return 'H';
		case IDX_L: return 'L';
		default: return '?';
	} // end register identification-by-index
} // end identify_reg8()

//=======================================================================
// def identify_reg16()
//
// Provides the null-terminated character string representation of
// the 16-bit register pair identified by `reg_idx'.
//
// In order to identify 8-bit registers, use identify_reg8().
//-----------------------------------------------------------------------
// Parameters:
// * reg_idx: A 16-bit register index.
//
// Returns: A pointer to the constant null-terminated string identifier
// of the register specified by the provided index, or "??" if the
// argument is not a valid index.
//=======================================================================
static const char*
identify_reg16(uint_fast8_t reg_idx) {
	switch (reg_idx) {
		case IDX_AF: return "AF";
		case IDX_BC: return "BC";
		case IDX_DE: return "DE";
		case IDX_HL: return "HL";
		default: return "??";
	} // end register identification-by-index
} // end identify_reg16()

//=======================================================================
// def twi_gb_cpu_diffdump
//=======================================================================
void twi_gb_cpu_diffdump(
		FILE* dest,
		const char* lhs_name,
		const struct twi_gb_cpu* restrict lhs,
		const char* rhs_name,
		const struct twi_gb_cpu* restrict rhs
) {
	twi_assert_notnull(dest);
	twi_assert_notnull(lhs_name);
	twi_assert_notnull(lhs);
	twi_assert_notnull(rhs_name);
	twi_assert_notnull(rhs);

	fprintf(dest, "== begin twi-gb-cpu diffdump ==\n");
	// Dump unmatched 8-bit registers
	for (uint8_t r = 0; r < sizeof(lhs->reg); ++r) {
		if (REG8(lhs, r) != REG8(rhs, r)) {
			fprintf(dest, "%c: %s = 0x%" PRIX8 " (%" PRIu8 "); %s = 0x%" PRIX8 " (%" PRIu8 ")\n",
					identify_reg8(r),
					lhs_name, REG8(lhs, r), REG8(lhs, r),
					rhs_name, REG8(rhs, r), REG8(rhs, r)
			);
		} // end if equivalent 8-bit registers do not have the same value
	} // end 8-bit register comparisons
	
	// 16-bit format string is used repeatedly.
	static const char* fmt16 = "%s: %s = 0x%" PRIX16 " (%" PRIu16 "); %s = 0x%" PRIX16 " (%" PRIu16 ")\n";
	
	// Dump unmatched 16-bit registers
	for (uint8_t r = 0; r < sizeof(lhs->reg); r += 2) {
		if (REG16(lhs, r) != REG16(rhs, r)) {
			fprintf(dest, fmt16, identify_reg16(r),
					lhs_name, REG16(lhs, r), REG16(lhs, r),
					rhs_name, REG16(rhs, r), REG16(rhs, r)
			);
		} // end if equivalent 16-bit registers do not have the same value
	} // end 16-bit register comparisons
	
	if (lhs->sp != rhs->sp)
		fprintf(dest, fmt16, "SP", lhs_name, lhs->sp, lhs->sp, rhs_name, rhs->sp, rhs->sp);
	if (lhs->pc != rhs->pc)
		fprintf(dest, fmt16, "PC", lhs_name, lhs->pc, lhs->pc, rhs_name, rhs->pc, rhs->pc);
	if (lhs->div != rhs->div)
		fprintf(dest, fmt16, "DIV", lhs_name, lhs->div, lhs->div, rhs_name, rhs->div, rhs->div);
	
	fprintf(dest, "== end twi-gb-cpu diffdump ==\n");
} // end twi_gb_cpu_dump()

#endif // TWI_GB_CPU_C

