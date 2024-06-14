#ifndef PRX_IO_H
#define PRX_IO_H
#include <stdio.h>

size_t
prx_io_fload(
		char* restrict dst,
		size_t dstsz,
		FILE* restrict file);
size_t
prx_io_fpload(
		char* restrict dst,
		size_t dstsz,
		const char* restrict fp);

#if PRX_TRUNCATE_PREFIX >= 1
#	define io_fload prx_io_fload
#	define io_fpload prx_io_fpload
#endif

#endif // PRX_IO_H

