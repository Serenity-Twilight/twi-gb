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
#include <stdint.h>
#include <twi/gb/cpu.h>
#include <twi/gb/log.h>
#include <twi/gb/mem.h>
#include <twi/std/assert.h>

//=======================================================================
//-----------------------------------------------------------------------
// Private macro definitions
//-----------------------------------------------------------------------
//=======================================================================

// Identifiers of the pointers to memory used by the CPU intreter.
// In exchange for enforcing specific identifiers for these pointers,
// they need not be passed to macro functions, thus decreasing the
// verbosity of the interpreter.
#define CPU_PTR (cpu) // Pointer to twi-gb-cpu
#define MEM_PTR (mem) // Pointer to twi-gb-mem
#define PROG_PTR (program) // Pointer to GB program code

//=======================================================================
// Macros providing abbreviated signatures for common read/write/access
// operations to and from registers, immediates, and memory.
//=======================================================================
// Access 8-bit register
#define REG8(i) (CPU_PTR->reg[i])
// Access 16-bit register
#define REG16(i) (*((uint16_t*)(CPU_PTR->reg + i)))
// Read 8-bit memory
#define READ8(addr) (twi_gb_mem_read8(MEM_PTR, addr))
// Write 8-bit memory
#define WRITE8(addr, val) (twi_gb_mem_write8(MEM_PTR, addr, val))
// Read unsigned 8-bit immediate
#define IMMED8 (*(PROG_PTR + (CPU_PTR)->pc + 1))
// Read unsigned 16-bit immediate
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	// Program memory is in little endian format.
	// Construct the value manually, independent of endianness.
#	define IMMED16 (PROG_PTR[CPU_PTR->pc + 1] | (((uint16_t)(PROG_PTR[CPU_PTR->pc + 2])) << 8))
#else // Little endian (and hopefully not mixed endian)
#	define IMMED16 (*((uint16_t*)(PROG_PTR + CPU_PTR->pc + 1)))
#endif // __BYTE_ORDER__
// Read signed 8-bit immediate
#define IMMEDs8 (*((int8_t*)(PROG_PTR + CPU_PTR->pc + 1)))

//=======================================================================
// Macros to abstract CPU register access.
// Registers that the CPU may pair together for specific instructions
// must respect the host machine endianness, or behavior will be undefined.
// Pairs are: AF, BC, DE, HL
// The first register in a pair is the higher order register,
// and the second is, then, the lower order register.
//=======================================================================

// 8-bit registers
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	// High order first, low order second
#	define REG_A REG8(0)
#	define REG_F REG8(1)
#	define REG_B REG8(2)
#	define REG_C REG8(3)
#	define REG_D REG8(4)
#	define REG_E REG8(5)
#	define REG_H REG8(6)
#	define REG_L REG8(7)
#else // Little endian (and hopefully not mixed endian)
	// Low order first, high order second
#	define REG_A REG8(1)
#	define REG_F REG8(0)
#	define REG_B REG8(3)
#	define REG_C REG8(2)
#	define REG_D REG8(5)
#	define REG_E REG8(4)
#	define REG_H REG8(7)
#	define REG_L REG8(6)
#endif // __BYTE_ORDER__

// 16-bit registers
#define REG_AF REG16(0)
#define REG_BC REG16(2)
#define REG_DE REG16(4)
#define REG_HL REG16(6)
#define REG_SP (CPU_PTR->sp)

//=======================================================================
// Operation macros
//=======================================================================
// Load `rhs`, discard `lhs`.
#define LD(lhs, rhs) (rhs)
// Add `rhs` to `lhs`.
#define ADD(lhs, rhs) ((lhs) + (rhs))
// Subtract `rhs` from `lhs`.
#define SUB(lhs, rhs) ((lhs) - (rhs))

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
	CPU_PTR->div += cyc; \
	CPU_PTR->pc += ib; \
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
// * ret: The return value at the end of each case statement.
//=======================================================================

// Shortcut macro for use in CASESET_7R, described below.
// * i: The number of `stride`s after `offset` of the case's integral trigger.
// * rhs: The right-hand side argument to pass to the operation function.
#define CASE_7R(i, r) \
	case offset + (stride * i): set(lhs, op(lhs, rhs)); ADV(cyc, 1); return (ret)

// Defines 7 switch cases, in which the right-hand side arguments
// appear in the following order:
// REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, REG_A
//
// offset + (stride * 6) is skipped when enumerating cases,
// due to the opcodes used by instructions which follow this pattern.
#define CASESET_7R(offset, stride, op, set, lhs, cyc, ret) \
	CASE_7R(0, REG_B); \
	CASE_7R(1, REG_C); \
	CASE_7R(2, REG_D); \
	CASE_7R(3, REG_E); \
	CASE_7R(4, REG_H); \
	CASE_7R(5, REG_L); \
	CASE_7R(7, REG_A)

// Same as CASESET_7R(), but handles an additional opcode at offset + (stride * 6)
// when the RHS value originates from memory pointed to by the value of REG_HL.
#define CASESET_7RHLM(offset, stride, op, set, lhs, cyc, HLM_cyc, ret) \
	CASESET_7R(offset, stride, op, set, lhs, cyc, ret); \
	case offset + (stride * 6): set(lhs, op(lhs, READ8(REG_HL))); ADV(HLM_cyc, 1); return (ret)

// Same as CASESET_7RHLMN(), but handles an additional opcode at n_offset,
// for when the RHS value originates from an unsigned 8-bit immediate.
#define CASESET_7RHLMN(offset, stride, n_offset, op, set, lhs, cyc, HLM_cyc, n_cyc, ret) \
	CASESET_7RHLM(offset, stride, op, set, lhs, cyc, HLM_cyc, ret); \
	case n_offset: set(lhs, op(lhs, IMMED8)); ADV(n_cyc, 2); return (ret)

// Shortcut macro for the implementation of CASESET_8B, defined below.
// * b: The number of strides after offset of the case's integral trigger.
//   as well as the bit offset used as the second argument of op().
#define CASE_8B(b) \
	case offset + (stride * (b)): op(lhs, b); ADV(cyc, 2); return (ret)

// Defines 8 switch cases, in which the right hand arguments are bit
// offset of the `lhs` argument, from lowest-to-highest order:
// 0, 1, 2, 3, 4, 5, 6, 7
//
// Note that the `set` argument is not present in this macro.
// As such, the `op` argument will be responsible for assignment of the result.
#define CASESET_8B(offset, stride, op, set, lhs, cyc, ret) \
	CASE_8B(0); \
	CASE_8B(1); \
	CASE_8B(2); \
	CASE_8B(3); \
	CASE_8B(4); \
	CASE_8B(5); \
	CASE_8B(6); \
	CASE_8B(7)

#define CASESET_7RHLM_8B(offset, op, cyc, HLM_cyc, ret, HLM_ret) ((void)0) // TODO

//=======================================================================
//-----------------------------------------------------------------------
// Private function declarations
//-----------------------------------------------------------------------
//=======================================================================

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
// decl interpret_once()
// def interpret_once()
// TODO: Description
//=======================================================================
static inline int_fast8_t
interpret_once(
		struct twi_gb_cpu* restrict cpu,
		struct twi_gb_mem* mem,
		const uint8_t* program
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);
	twi_assert_notnull(program);

	switch (program[cpu->pc]) {
	} // end operation lookup
}; // end interpret_once()

