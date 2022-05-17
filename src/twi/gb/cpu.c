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
#define MEM (*mem)

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
#define RMEM8(addr) twi_gb_mem_read8(&MEM, addr)
// Write 8-bit memory
#define WMEM8(addr, val) twi_gb_mem_write8(&MEM, addr, val)
// Read unsigned 8-bit immediate
#define IMMED8 (RMEM8((CPU_PTR)->pc + 1))
// Read unsigned 16-bit immediate
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	// Program memory is in little endian format.
	// Construct the value manually, independent of endianness.
#	define IMMED16 (RMEM8((CPU_PTR)->pc + 1) | (((uint16_t)(RMEM((CPU_PTR)->pc + 1))) << 8))
#else // Little endian (and hopefully not mixed endian)
#	define IMMED16 (*((uint16_t*)(PROG_PTR + (CPU_PTR)->pc + 1))) // TODO
#endif // __BYTE_ORDER__
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
// decl ADV()
// def ADV()
//
// Advances the CPU cycle counter and instruction pointer by the
// specified values.
//-----------------------------------------------------------------------
// Parameters:
// * cyc: The number of cycles to advance the CPU clock by.
// * ib: The number of bytes to advance the instruction pointer by.
//=======================================================================
#define ADV(cyc, ib) { \
	(CPU_PTR)->div += cyc; \
	(CPU_PTR)->pc += ib; \
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

// A macro which writes `val` to the register `reg`.
// This is intended for use specifically with case sets, so they be
// instructed on how to set the register, versus setting memory.
// If you intend to write to registers outside of case sets, use the
// standard register access (R/W) macros defined above.
#define WREG8(reg, val) ((reg) = (val))

// Defines the return conditions of any operation which modifies memory
// via a pointer stored in HL.
// 0xFFFF is the location of the interrupt enable (IE) register.
// If this address is written to, active interrupts will have to be
// re-evaluted to know when to next preempt the CPU.
#define HLM_RETURN ((REG_HL == 0xFFFF) ? -1 : 0)

// The base case from which all casesets are constructed.
#define CASE(offset, op, set, lhs, rhs, cyc, ib, ret) \
	case offset: set(lhs, op(&REG_F, lhs, rhs)); ADV(cyc, ib); return (ret)

// Defines 7 switch cases, in which the right-hand side arguments
// appear in the following order:
// REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_A
//
// offset + (stride * 6) is skipped when enumerating cases,
// due to the opcodes used by instructions which follow this pattern.
#define CASESET_7R(offset, stride, op, set, lhs, cyc, ret) \
	CASE(offset               , op, set, lhs, REG_B, cyc, 1, ret); \
	CASE(offset + stride      , op, set, lhs, REG_C, cyc, 1, ret); \
	CASE(offset + (stride * 2), op, set, lhs, REG_D, cyc, 1, ret); \
	CASE(offset + (stride * 3), op, set, lhs, REG_E, cyc, 1, ret); \
	CASE(offset + (stride * 4), op, set, lhs, REG_H, cyc, 1, ret); \
	CASE(offset + (stride * 5), op, set, lhs, REG_L, cyc, 1, ret); \
	CASE(offset + (stride * 7), op, set, lhs, REG_A, cyc, 1, ret)

// Same as CASESET_7R(), but handles an additional opcode at `n_offset`,
// for when the RHS value originates from an unsigned 8-bit immediate.
#define CASESET_7RN(offset, stride, n_offset, op, set, lhs, cyc, n_cyc, ret) \
	CASESET_7R(offset, stride, op, set, lhs, cyc, ret); \
	CASE(n_offset, op, set, lhs, IMMED8, n_cyc, 2, ret)

// Same as CASESET_7R(), but handles an additional opcode at offset + (stride * 6)
// when the RHS value originates from memory pointed to by the value of REG_HL.
#define CASESET_7RHLM(offset, stride, op, set, lhs, cyc, HLM_cyc, ret) \
	CASESET_7R(offset, stride, op, set, lhs, cyc, ret); \
	CASE(offset + (stride * 6), op, set, lhs, RMEM8(REG_HL), HLM_cyc, 1, (ret))

// Same as CASESET_7RHLMN(), but handles an additional opcode at n_offset,
// for when the RHS value originates from an unsigned 8-bit immediate.
#define CASESET_7RHLMN(offset, stride, n_offset, op, set, lhs, cyc, HLM_cyc, n_cyc, ret) \
	CASESET_7RHLM(offset, stride, op, set, lhs, cyc, HLM_cyc, ret); \
	CASE(n_offset, op, set, lhs, IMMED8, n_cyc, 2, ret)

// Shortcut macro for the implementation of CASESET_8B, defined below.
// * b: The number of strides after offset of the case's integral trigger.
//   as well as the bit offset used as the second argument of op().
#define CASE_8B(b) \
	case offset + (stride * (b)): set(lhs, op(lhs, b)); ADV(cyc, 2); return (ret)

// Defines 8 switch cases, in which the right hand arguments are bit
// offset of the `lhs` argument, from lowest-to-highest order:
// 0, 1, 2, 3, 4, 5, 6, 7
//
// Note that the `set` argument is not present in this macro.
// As such, the `op` argument will be responsible for assignment of the result.
#define CASESET_8B(offset, stride, op, lhs, cyc, ret) \
	CASE_8B(0); \
	CASE_8B(1); \
	CASE_8B(2); \
	CASE_8B(3); \
	CASE_8B(4); \
	CASE_8B(5); \
	CASE_8B(6); \
	CASE_8B(7)

// Defines cases for all 64 permutations of an operation with
// Either one of the 7 8-bit registers (not F) or HL as the LHS argument,
// and a bit offset of 0-7 as the RHS argument.
//
// Note the following:
// * No `stride` argument is taken as the stride for all operations with
//   this opcode pattern is the same.
// * As operations with bit offset arguments may perform both a read and
//   a write on the LHS argument, an alternate op function is required when
//   interacting with the memory pointed to by HL (defined by `HLM_op`).
// * No return for non-HLM operations is accepted, as non-HLM operations
//   never modify memory, and therefore never prompt re-evaluation of
//   active interrupts. All non-HLM operations return 0.
#define CASESET_7RHLM_8B(offset, op, HLM_op, cyc, HLM_cyc, HLM_ret) \
	CASESET_8B(offset      , 0x8, op, REG_B, cyc, 0); \
	CASESET_8B(offset + 0x1, 0x8, op, REG_C, cyc, 0); \
	CASESET_8B(offset + 0x2, 0x8, op, REG_D, cyc, 0); \
	CASESET_8B(offset + 0x3, 0x8, op, REG_E, cyc, 0); \
	CASESET_8B(offset + 0x4, 0x8, op, REG_H, cyc, 0); \
	CASESET_8B(offset + 0x5, 0x8, op, REG_L, cyc, 0); \
	CASESET_8B(offset + 0x6, 0x8, HLM_op, REG_HL, HLM_cyc, HLM_ret); \
	CASESET_8B(offset + 0x7, 0x8, op, REG_A, cyc, 0)

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
static inline uint8_t LD8(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) { return rhs; }
static inline uint16_t LD16(uint8_t* restrict f, uint16_t lhs, uint16_t rhs) { return rhs; }
static inline uint8_t ADD8(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) { return lhs + rhs; }
static inline uint16_t ADD16(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) { return lhs + rhs; }
static inline uint8_t SUB(uint8_t* restrict f, uint8_t lhs, uint8_t rhs) { return lhs - rhs; }

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
		CASESET_7RHLMN(0x40, 0x1, 0x06, LD8, WREG8, REG_B, 4, 8, 8, 0); // LD B, r|(HL)|n
		CASESET_7RHLMN(0x48, 0x1, 0x0E, LD8, WREG8, REG_C, 4, 8, 8, 0); // LD C, r|(HL)|n
		CASESET_7RHLMN(0x50, 0x1, 0x16, LD8, WREG8, REG_D, 4, 8, 8, 0); // LD D, r|(HL)|n
		CASESET_7RHLMN(0x58, 0x1, 0x1E, LD8, WREG8, REG_E, 4, 8, 8, 0); // LD E, r|(HL)|n
		CASESET_7RHLMN(0x60, 0x1, 0x26, LD8, WREG8, REG_H, 4, 8, 8, 0); // LD H, r|(HL)|n
		CASESET_7RHLMN(0x68, 0x1, 0x2E, LD8, WREG8, REG_L, 4, 8, 8, 0); // LD L, r|(HL)|n
		CASESET_7RN(0x70, 0x1, 0x36, LD8, WMEM8, REG_HL, 8, 12, HLM_RETURN); // LD (HL), r|n
		CASESET_7RHLMN(0x78, 0x1, 0x3E, LD8, WREG8, REG_A, 4, 8, 8, 0); // LD A, r|(HL)|n
	} // end operation lookup
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

