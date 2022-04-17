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
// TODO
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_GB_GB_H
#define TWI_GB_GB_H

#include <twi/gb/mem.h>
#include <twi/gb/vid.h>

//=======================================================================
//-----------------------------------------------------------------------
// Public type definitions
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl struct twi_gb
// def struct twi_gb
// TODO
//=======================================================================
struct twi_gb {
	struct twi_gb_mem mem;
	struct twi_gb_vid vid;
}; // end struct twi_gb

//=======================================================================
//-----------------------------------------------------------------------
// Public function declarations
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// decl twi_gb_run()
// TODO
//=======================================================================
void
twi_gb_run(struct twi_gb* gb);

#endif // TWI_GB_GB_H

