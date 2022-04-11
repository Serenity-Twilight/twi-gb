//======================================================================
//----------------------------------------------------------------------
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
//----------------------------------------------------------------------
//======================================================================
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <twi/std/assert.h>
#include <twi/std/utils.h>

//======================================================================
//----------------------------------------------------------------------
// Public function definitions
//----------------------------------------------------------------------
//======================================================================

//======================================================================
// def twi_fload()
//======================================================================
int_fast8_t
twi_fload(size_t bufsz, void* restrict buf, const char* restrict path, int_fast8_t binary) {
	twi_assert_notnull(buf);
	twi_assert_notnull(path);

	errno = 0;
	FILE* src;
	if (binary)
		src = fopen(path, "rb");
	else
		src = fopen(path, "r");
	if (src == NULL)
		return 1;

	errno = 0;
	fread(buf, 1, bufsz, src);
	int error = ferror(src);
	fclose(src);
	return error;
} // end twi_fload()

//======================================================================
// def twi_fsize()
//======================================================================
size_t
twi_fsize(const char* restrict path) {
	twi_assert_notnull(path);
	struct stat statbuf;
	if (stat(path, &statbuf))
		return ((size_t)-1);
	return (size_t)statbuf.st_size;
} // end twi_fsize()

//======================================================================
//----------------------------------------------------------------------
// Private function definitions
//----------------------------------------------------------------------
//======================================================================

