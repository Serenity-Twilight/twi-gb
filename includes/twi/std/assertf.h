#ifndef TWI_ASSERTF_H
#define TWI_ASSERTF_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef TWI_NOASSERTF
#	define twi_assertf(condition, format, ...) \
		if (!(condition)) { \
			fprintf(stderr, "assertion failure\n" __FILE__ ":%s:%ld\n(" \
					#condition "): " format "\n", __func__, __LINE__, ##__VA_ARGS__); \
			exit(EXIT_FAILURE); \
		}
#else
#	define twi_assertf(...) ((void)0)
#endif

#endif // TWI_ASSERTF_H
