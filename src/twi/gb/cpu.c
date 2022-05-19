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
#include <stdint.h>
#include <stdio.h>
#include <twi/gb/cpu.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/std/assertf.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private macro definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Aliases for the twi-gb-cpu and twi-gb-mem arguments of the interpreter.
// Allows for interpreter macros to be used in contexts with different
// variable names/types. Primarily useful in testing.
//=======================================================================
#define CPU_PTR (cpu)
#define MEM_PTR (mem)

//=======================================================================
// Macros providing abbreviated signatures for common read/write/access
// operations to and from registers, immediates, and memory.
//=======================================================================
// Access 8-bit or 16-bit register
// These macros exists primarily for defining special macros for each
// individual register. Use those macros (defined later) whenever possible.
#define REG8(cpu_ptr, idx) ((cpu_ptr)->reg[idx])
#define REG16(cpu_ptr, idx) (*((uint16_t*)((cpu_ptr)->reg + (idx))))
// Read 8-bit memory
#define RMEM8(addr) twi_gb_mem_read8(MEM_PTR, addr)
// Write 8-bit memory
#define WMEM8(addr, val) twi_gb_mem_write8(MEM_PTR, addr, val)
// Read unsigned 8-bit immediate
#define IMMED8 (RMEM8((CPU_PTR)->pc + 1))
// Read unsigned 16-bit immediate
#define IMMED16 (twi_gb_mem_read16(MEM_PTR, (CPU_PTR)->pc + 1))
// Read signed 8-bit immediate
#define IMMEDs8 (*((int8_t*)(PROG_PTR + (CPU_PTR)->pc + 1))) // TODO

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

// Direct 8-bit register access
#	define REG_A REG8(CPU_PTR, IDX_A)
#	define REG_F REG8(CPU_PTR, IDX_F)
#	define REG_B REG8(CPU_PTR, IDX_B)
#	define REG_C REG8(CPU_PTR, IDX_C)
#	define REG_D REG8(CPU_PTR, IDX_D)
#	define REG_E REG8(CPU_PTR, IDX_E)
#	define REG_H REG8(CPU_PTR, IDX_H)
#	define REG_L REG8(CPU_PTR, IDX_L)

// Direct 16-bit register access
#define REG_AF REG16(CPU_PTR, IDX_AF)
#define REG_BC REG16(CPU_PTR, IDX_BC)
#define REG_DE REG16(CPU_PTR, IDX_DE)
#define REG_HL REG16(CPU_PTR, IDX_HL)
#define REG_SP ((CPU_PTR)->sp)

//=======================================================================
// Flag manipulation and retrieval functions.
//=======================================================================

// Flag bit offsets
#define FBIT_Z 7
#define FBIT_N 6
#define FBIT_H 5
#define FBIT_C 4

// Set flag bits.
// f_ptr: Pointer to flag value.
// z: Zero flag bit, 1 or 0
// n: Negative flag bit, 1 or 0
// h: Half-carry flag bit, 1 or 0
// c: Carry flag bit, 1 or 0
#define SET_FLAGS(f_ptr, z, n, h, c) \
	*(f_ptr) = ((z) << FBIT_Z) | ((n) << FBIT_N) | ((h) << FBIT_H) | ((c) << FBIT_C)

// Get flag bit.
// f_ptr: Pointer to flag value.
// fbit: Bit offset of flag to read.
//
// returns 1 if flag is set, 0 if flag is cleared
#define GET_FLAG(f_ptr, fbit) ((*(f_ptr) >> (fbit)) & 0x1)

//=======================================================================
// def ADV()
//
// Advances the CPU cycle counter and instruction pointer by the
// specified values.
//-----------------------------------------------------------------------
// Parameters:
// * ib: The number of bytes to advance the instruction pointer by.
// * cyc: The number of cycles to advance the CPU clock by.
//=======================================================================
#define ADV(ib, cyc) { \
	(CPU_PTR)->pc += ib; \
	(CPU_PTR)->div += cyc; \
}

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

// The following macros define how to write an instruction's
// resulting value after its has been computed.
// Each accepts 2 arguments:
// 1: Defines the destination.
// 2: Defines the value.
//
// These are intended to be used as `set` arguments for CASESET macros.
#define WREG(reg, val) ((reg) = (val)) // To register.
#define WMEM8(addr, val) twi_gb_mem_write8(MEM_PTR, addr, val) // To byte in memory.
#define WIGN(na, val) (val) // Perform computation, but do not write.
// Write rr to (SP), increment SP by 2.
#define WPUSH(rr, ...) \
	twi_gb_mem_write16(MEM_PTR, REG_SP, rr); \
	REG_SP += 2;
// Decrement SP by 2. Write (SP) to rr.
#define WPOP(rr, ...) \
	REG_SP -= 2; \
	rr = twi_gb_mem_read16(MEM_PTR, REG_SP);

// Defines the return conditions of any operation which modifies memory
// via a pointer stored in HL.
// 0xFFFF is the location of the interrupt enable (IE) register.
// If this address is written to, active interrupts will have to be
// re-evaluted to know when to next preempt the CPU.
#define HLM_RETURN ((REG_HL == 0xFFFF) ? -1 : 0)

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
#define CASESET28_7R(offset, stride, op, set, lhs, cyc, ret) \
	CASE(offset               , op, set, lhs, REG_B, 1, cyc, ret); \
	CASE(offset + stride      , op, set, lhs, REG_C, 1, cyc, ret); \
	CASE(offset + stride * 2, op, set, lhs, REG_D, 1, cyc, ret); \
	CASE(offset + stride * 3, op, set, lhs, REG_E, 1, cyc, ret); \
	CASE(offset + stride * 4, op, set, lhs, REG_H, 1, cyc, ret); \
	CASE(offset + stride * 5, op, set, lhs, REG_L, 1, cyc, ret); \
	CASE(offset + stride * 7, op, set, lhs, REG_A, 1, cyc, ret)

// Same as CASESET28_7R(), and handles an additional opcode at `n_offset`,
// for when the rhs value originates from an unsigned 8-bit immediate.
#define CASESET28_7RN(offset, stride, n_offset, op, set, lhs, cyc, n_cyc, ret) \
	CASESET28_7R(offset, stride, op, set, lhs, cyc, ret); \
	CASE(n_offset, op, set, lhs, IMMED8, 2, n_cyc, ret)

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
	CASESET28_7RN(offset, stride, n_offset, op, set, lhs, 4, 8, 0); \
	CASE(offset + stride * 6, op, set, lhs, RMEM8(REG_HL), 1, 8, HLM_RETURN)

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
	case offset + stride * 6: WMEM8(REG_HL, op(&REG_F, RMEM8(REG_HL), 0)); ADV(ib, (cyc) + 8); return HLM_RETURN; \
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
// * As the targets are 16-bit registers, no return expression is accepted.
#define CASESET16_4RR(offset, op, set, lhs4, rhs, ib, cyc) \
	CASESET16_4(offset, op, set, REG_BC, REG_DE, REG_HL, lhs4, rhs, rhs, rhs, rhs, ib, cyc, 0)

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

//=======================================================================
//-----------------------------------------------------------------------
// Private function definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// Operation functions.
//
// Behavior is undefined if `f` points to NULL.
//-----------------------------------------------------------------------
// * f: A pointer to the 8-bit flag register.
// * lhs: The left-hand side 8-bit or 16-bit value.
// * rhs: The right-hand side 8-bit or 16-bit value.
//=======================================================================
// NOP: Consume arguments and do nothing.
#define NOP(...) ((void)0)

// LD: Overwrite lhs value with rhs value. No flag changes.
static inline uint8_t LD8(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) { return rhs; }
static inline uint16_t LD16(uint8_t* restrict f, uint16_t lhs, uint16_t rhs) { return rhs; }

// ADD: Add rhs value to lhs value.
// ADC: Add rhs value and carry bit to lhs value.
//
// Z: 
// * 8-bit: Set if sum == 0, else reset.
// * 16-bit: No change.
// N: Reset.
// H:
// * 8-bit: Set if carrying from bit 3, else reset.
// * 16-bit: Set if carrying from bit 11, else reset.
// C: Set if overflow occurs, else reset.
static inline uint8_t ADD8(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint16_t sum = lhs + rhs;
	SET_FLAGS(f, sum == 0, 0, ((lhs & 0xF) + (rhs & 0xF)) > 0xF, sum > 0xFF);
	return (uint8_t)sum;
} // end ADD8()
static inline uint16_t ADD16(uint8_t* restrict f, uint16_t lhs, uint16_t rhs) {
	uint32_t sum = lhs + rhs;
	SET_FLAGS(f, GET_FLAG(f, FBIT_Z), 0, ((lhs & 0xFFF) + (rhs & 0xFFF)) > 0xFFF, sum > 0xFFFF);
	return (uint16_t)sum;
} // end ADD16()
static inline uint8_t ADC(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint_fast8_t c = GET_FLAG(f, FBIT_C);
	uint16_t sum = lhs + rhs + c;
	SET_FLAGS(f, sum == 0, 0, ((lhs & 0xF) + (rhs & 0xF) + c) > 0xF, sum > 0xFF);
	return (uint8_t)sum;
} // end ADC()

// SUB: Subtract rhs value from lhs value.
// SBC: Subtract rhs value and carry bit from lhs value.
// (Note: CP is identical to SUB with its return value discarded).
//
// Z: Set if difference == 0, else reset.
// N: Set.
// H: Set if borrowing from bit 4, else reset.
// C: Set if overflow occurs, else reset.
static inline uint8_t SUB(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint16_t diff = lhs - rhs;
	SET_FLAGS(f, diff == 0, 1, ((lhs & 0xF) - (rhs - 0xF)) > 0xF, diff > 0xFF);
	return (uint8_t)diff;
} // end SUB()
static inline uint8_t SBC(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint_fast8_t c = GET_FLAG(f, FBIT_C);
	uint16_t diff = lhs - rhs - c;
	SET_FLAGS(f, diff == 0, 1, ((lhs & 0xF) - (rhs & 0xF) - c) > 0xF, diff > 0xFF);
	return (uint8_t)diff;
} // end SBC()

// AND: Logical AND of lhs and rhs values.
// OR: Logical OR of lhs and rhs values.
// XOR: Logical XOR of lhs and rhs values.
//
// Z: Set if result == 0, else reset.
// N: Reset.
// H: Set for AND, else reset.
// C: Reset.
static inline uint8_t AND(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs & rhs;
	SET_FLAGS(f, result == 0, 0, 1, 0);
	return result;
} // end AND()
static inline uint8_t OR(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs | rhs;
	SET_FLAGS(f, result == 0, 0, 0, 0);
	return result;
} // end OR()
static inline uint8_t XOR(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) {
	uint8_t result = lhs ^ rhs;
	SET_FLAGS(f, result == 0, 0, 0, 0);
	return result;
} // end XOR()

// INC: Increment lhs value by 1.
// DEC: Decrement lhs value by 1.
//
// NOTE: 16-bit versions make no flag changes.
// 8-bit flag changes:
// Z: Set if result == 0, else reset.
// N: Set if DEC, reset if INC.
// H:
// * INC: Set if carry from bit 3, else reset.
// * DEC: Set if borrow from bit 4, else reset.
// C: No change.
static inline uint8_t INC8(uint8_t* restrict f, uint8_t lhs, uint8_t) {
	uint8_t sum = lhs + 1;
	SET_FLAGS(f, sum == 0, 0, (lhs & 0xF) == 0xF, GET_FLAG(f, FBIT_C));
	return sum;
} // end INC8()
static inline uint16_t INC16(uint8_t* restrict f, uint16_t lhs, uint16_t) {
	return lhs + 1;
} // end INC16()
static inline uint8_t DEC8(uint8_t* restrict f, uint8_t lhs, uint8_t) {
	uint8_t diff = lhs - 1;
	SET_FLAGS(f, diff == 0, 1, (lhs & 0xF) == 0, GET_FLAG(f, FBIT_C));
	return diff;
} // end DEC8()
static inline uint16_t DEC16(uint8_t* restrict f, uint16_t lhs, uint16_t) {
	return lhs - 1;
} // end DEC16()

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

	switch (twi_gb_mem_read8(mem, cpu->pc)) {
		case 0x00: ADV(1, 4); return 0; // NOP
		CASESET16_4RR(0x01, LD16, WREG, REG_SP, IMMED16, 3, 12); // LD rr, nn
		CASESET16_4RR(0x03, INC16, WREG, REG_SP, 0, 1, 8); // INC rr
		CASESET18_7RHLP(0x04, 8, INC8, 1, 4); // INC r|(HL)
		CASESET18_7RHLP(0x05, 8, DEC8, 1, 4); // DEC r|(HL)
		CASESET16_4RR(0x0B, DEC16, WREG, REG_SP, 0, 1, 8); // DEC rr
		CASESET28_7RHLPN(0x40, 1, 0x06, LD8, WREG, REG_B); // LD B, r|(HL)|n
		CASESET28_7RHLPN(0x48, 1, 0x0E, LD8, WREG, REG_C); // LD C, r|(HL)|n
		CASESET28_7RHLPN(0x50, 1, 0x16, LD8, WREG, REG_D); // LD D, r|(HL)|n
		CASESET28_7RHLPN(0x58, 1, 0x1E, LD8, WREG, REG_E); // LD E, r|(HL)|n
		CASESET28_7RHLPN(0x60, 1, 0x26, LD8, WREG, REG_H); // LD H, r|(HL)|n
		CASESET28_7RHLPN(0x68, 1, 0x2E, LD8, WREG, REG_L); // LD L, r|(HL)|n
		CASESET28_7RN(0x70, 1, 0x36, LD8, WMEM8, REG_HL, 8, 12, HLM_RETURN); // LD (HL), r|n
		CASESET28_7RHLPN(0x78, 1, 0x3E, LD8, WREG, REG_A); // LD A, r|(HL)|n
		CASESET28_7RHLPN(0x80, 1, 0xC6, ADD8, WREG, REG_A); // ADD A, r|(HL)|n
		CASESET28_7RHLPN(0x88, 1, 0xCE, ADC, WREG, REG_A); // ADC A, r|(HL)|n
		CASESET28_7RHLPN(0x90, 1, 0xD6, SUB, WREG, REG_A); // SUB A, r|(HL)|n
		CASESET28_7RHLPN(0x98, 1, 0xDE, SBC, WREG, REG_A); // SBC A, r|(HL)|n
		CASESET28_7RHLPN(0xA0, 1, 0xE6, AND, WREG, REG_A); // AND A, r|(HL)|n
		CASESET28_7RHLPN(0xA8, 1, 0xEE, XOR, WREG, REG_A); // XOR A, r|(HL)|n
		CASESET28_7RHLPN(0xB0, 1, 0xF6, OR, WREG, REG_A); // OR A, r|(HL)|n
		CASESET28_7RHLPN(0xB8, 1, 0xFE, SUB, WIGN, REG_A); // CP A, r|(HL)|n
		CASESET16_4RR(0xC1, NOP, WPOP, REG_AF, 0, 1, 12); // POP rr
		CASESET16_4RR(0xC5, NOP, WPUSH, REG_AF, 0, 1, 16); // PUSH rr
	} // end operation lookup and execution
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

#endif // TWI_GB_CPU_C

