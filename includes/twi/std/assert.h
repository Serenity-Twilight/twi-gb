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
// Provides definitions for assertions which allow for string
// descriptions to be printed on assertion failure, with support for
// stdio-like substitution.
//
// Additionally, assertions for common checks are provided with
// pre-written descriptions.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef TWI_ASSERT_H
#define TWI_ASSERT_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

//=======================================================================
// Programs which include this header may implement their own handling
// of the assertion failure message.
//
// The handler will receive a format string as its first argument,
// followed by a list of variadic arguments for the format string,
// alike to the printf family of functions.
//
// Note that the message handler will always be immediately followed by
// a call to `exit()` with value `EXIT_FAILURE` and should not attempt
// to circumvent this behavior.
//=======================================================================
#ifndef TWI_ASSERT_MSG_HANDLER
#	define TWI_ASSERT_MSG_HANDLER(msg, ...) \
		fprintf(stderr, "Assertion failure\n" __FILE__ ":%s():%ld\n" msg "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

//=======================================================================
// The following are macros for asserting the correctness of code.
//
// twi_assertf() allows the user to provide their own format string
// and optionally a variadic argument list to display should the
// assertion failure.
//
// The rest are convenience macros which provide their own
// predefined message for the failure of common assertions.
//=======================================================================
#if TWI_NOASSERT
# define twi_assertf(...) ((void)0)
#else
// Checks a condition, and terminates the program with a user-defined
// error message if the condition equates to false.
#	define twi_assertf(condition, msg, ...) \
		if (!(condition)) { \
			TWI_ASSERT_MSG_HANDLER("(" #condition "): " msg, ##__VA_ARGS__); \
			exit(EXIT_FAILURE); \
		}
#endif // if/else TWI_NOASSERT

// Checks a pointer, and terminates the program with a predefined
// message if the pointer points to NULL.
#define twi_assert_notnull(ptr) twi_assertf((ptr) != NULL, "`" #ptr "` cannot point to NULL.")

// Checks if the value of the variable on the left is less than
// the value of the variable on the right.
// Terminates with a predefined message if the above condition is false.
#define twi_assert_lt(lhs, rhs) { \
	long long _lhs = (lhs); \
	long long _rhs = (rhs); \
	twi_assertf(_lhs < _rhs, "`" #lhs "` must be less than `" #rhs "` (`" #lhs "` = %lld, `" #rhs "` = %lld).", _lhs, _rhs); \
}

// Checks if the value of the variable on the left is less than
// the immediate (numeric constant) value on the right.
// Terminates with a predefined message if the above condition is false.
#define twi_assert_lti(lhs, immed) { \
	long long _lhs = (lhs); \
	long long _immed = (immed); \
	twi_assertf(_lhs < _immed, "`" #lhs "` must be less than %lld (`" #lhs "` = %lld).", _immed, _lhs); \
}

// Checks if the value of the variable on the left is less than or equal to
// the immediate (numeric constant) value on the right.
// Terminates with a predefined message if the above condition is false.
#define twi_assert_lteqi(lhs, immed) { \
	long long _lhs = (lhs); \
	long long _immed = (immed); \
	twi_assertf(_lhs < _immed, "`" #lhs "` must be less than or equal to %lld (`" #lhs "` = %lld).", _immed, _lhs); \
}

// Checks if the value of the variable on the left is greater than
// the immediate (numeric constant) value on the right.
// Terminates with a predefined message if the above condition is false.
#define twi_assert_gti(lhs, immed) { \
	long long _lhs = (lhs); \
	long long _immed = (immed); \
	twi_assertf(_lhs > _immed, "`" #lhs "` must be greater than or equal to %lld (`" #lhs "` = %lld).", _immed, _lhs); \
}

#endif // TWI_ASSERT_H

