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
// This file defines a structure from which objects which contain the
// state of an emulated twi-gb CPU. It also declares interfaces used
// for modifying the state of aforementioned objects.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_CPU_H
#define TWI_GB_CPU_H

#include <stdint.h>

//=======================================================================
//-----------------------------------------------------------------------
// Public type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb_cpu
// def struct twi_gb_cpu
//
// Maintains the necessary state to emulate a GB CPU.
//-----------------------------------------------------------------------
// Members (ALL INTERNAL):
// * reg: The 8 8-bit, general-purpose registers of the twi-gb CPU:
//   A, B, C, D, E, F, H, L
//   Additionally, the pairs of AB, CD, EF, and HL will be utilized by
//   specific instructions, forming effective 16-bit registers.
//   Hence the need for 16-bit alignment.
//   In these cases, the endianness of each 16-bit register pair is
//   Big Endian with regards to its name (HL, H = High, L = Low).
//=======================================================================
struct twi_gb_cpu {
	_Alignas(uint16_t) uint8_t reg[8];
}; // end struct twi_gb_cpu

//=======================================================================
//-----------------------------------------------------------------------
// Public function declartations
//-----------------------------------------------------------------------
//=======================================================================

#endif // TWI_GB_CPU_H

