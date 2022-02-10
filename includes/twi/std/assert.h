//=======================================================================
//=======================================================================
// TODO
// Written by Serenity Twilight
//=======================================================================
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
#	define twi_assert_notnull(...) ((void)0)
#else
// Checks a condition, and terminates the program with a user-defined
// error message if the condition equates to false.
#	define twi_assertf(condition, msg, ...) \
		if (!(condition)) { \
			TWI_ASSERT_MSG_HANDLER("(" #condition "): " msg, ##__VA_ARGS__); \
			exit(EXIT_FAILURE); \
		}

// Checks a pointer, and terminates the program with a predefined
// message if the pointer points to NULL.
#define twi_assert_notnull(ptr) twi_assertf((ptr) != NULL, "`" #ptr "` cannot point to NULL.")
#endif // TWI_NOASSERT
#endif // TWI_ASSERT_H

