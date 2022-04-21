//=======================================================================
//-----------------------------------------------------------------------
// Copyright 2022 Serenity Twilight
//
// This file is part of twi standard library, my own personal library
// of utilities that I reuse amongst my projects.
//
// The twi standard library is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// The twi standard library is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the twi standard library.
// If not, see <https://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------
// Commonly used mechanisms for writing automated tests are defined here.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_TEST_H
#define TWI_TEST_H

#include <stdio.h>

//=======================================================================
// def twi_test()
//
// Tests a condition, and, if the condition evaluates to false,
// prints a message to `stderr` indicating the location of the failure
// and the condition which triggered it.
//
// Additionally, the program will locally jump to the label `test_fail`.
// It is recommend the user print as much information as possible about
// the state of the test context at the operation which failed.
//
// If the condition evaluates to true, then no message is printed and
// no jump occurs.
//-----------------------------------------------------------------------
// Parameters:
// * condition: A conditions which evaluates to a boolean type.
//=======================================================================
#define twi_test(condition) \
	if (!(condition)) { \
		fprintf(stderr, "Test failed (" #condition ")\n" __FILE__ ":%s():%ld\n", __func__, __LINE__); \
		goto test_fail; \
	}

#endif // TWI_TEST_H

