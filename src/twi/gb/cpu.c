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
#include <twi/gb/cpu.h>

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

// Register access macro.
// Provides indirection between register usage and the name of the
// struct member containing registers, in case the member identifier changes.
#define REG(i) ((CPU_PTR)->reg[i])

// Macros to abstract CPU register access.
// Registers that the CPU may pair together for specific instructions
// must respect the host machine endianness, or behavior will be undefined.
// Pairs are: AB, CD, EF, HL
// The first register in a pair is the higher order register,
// and the second is, then, the lower order register.

// 8-bit registers
#if __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
	// Low order first, high order second
#	define REG_A REG(1)
#	define REG_B REG(0)
#	define REG_C REG(3)
#	define REG_D REG(2)
#	define REG_E REG(5)
#	define REG_F REG(4)
#	define REG_H REG(7)
#	define REG_L REG(6)
#else
	// High order first, low order second
#	define REG_A REG(0)
#	define REG_B REG(1)
#	define REG_C REG(2)
#	define REG_D REG(3)
#	define REG_E REG(4)
#	define REG_F REG(5)
#	define REG_H REG(6)
#	define REG_L REG(7)
#endif // __BYTE_ORDER__

// 16-bit registers
#define REG_AB (*((uint16_t*)(&REG(0))))
#define REG_CD (*((uint16_t*)(&REG(2))))
#define REG_EF (*((uint16_t*)(&REG(4))))
#define REG_HL (*((uint16_t*)(&REG(6))))

