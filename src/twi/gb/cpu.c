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

// The name of the twi-gb CPU object pointer that is used in functions
// which reference macros following this one. In exchange for mandating
// a common identifier for the twi-gb CPU pointer between various functions,
// extensive use of the identifier in accessing CPU struct members can
// be avoided, decreasing verbosity.
#define CPU_PTR cpu

// 8-bit and 16-bit register access-by-index macros.
// Provides indirection between register usage and the name of the
// struct member containing registers, in case the member identifier changes.
#define REG8(i) ((CPU_PTR)->reg[i])
#define REG16(i) (*((uint16_t*)((CPU_PTR)->reg + i)))

// Macros to abstract CPU register access.
// Registers that the CPU may pair together for specific instructions
// must respect the host machine endianness, or behavior will be undefined.
// Pairs are: AB, CD, EF, HL
// The first register in a pair is the higher order register,
// and the second is, then, the lower order register.

// 8-bit registers
#if __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
	// Low order first, high order second
#	define REG_A REG8(1)
#	define REG_B REG8(0)
#	define REG_C REG8(3)
#	define REG_D REG8(2)
#	define REG_E REG8(5)
#	define REG_F REG8(4)
#	define REG_H REG8(7)
#	define REG_L REG8(6)
#else
	// High order first, low order second
#	define REG_A REG8(0)
#	define REG_B REG8(1)
#	define REG_C REG8(2)
#	define REG_D REG8(3)
#	define REG_E REG8(4)
#	define REG_F REG8(5)
#	define REG_H REG8(6)
#	define REG_L REG8(7)
#endif // __BYTE_ORDER__

// 16-bit registers
#define REG_AB REG16(0)
#define REG_CD REG16(2)
#define REG_EF REG16(4)
#define REG_HL REG16(6)

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
// decl interpret()
// def interpret()
// TODO: Description
//=======================================================================
static inline int_fast8_t
interpret(
		struct twi_gb_cpu* restrict cpu,
		struct twi_gb_mem* restrict mem,
		int64_t* restrict remaining_time
) {
	twi_assert_notnull(cpu);
	twi_assert_notnull(mem);
	twi_assert_notnull(remaining_time);
}; // end interpret()

