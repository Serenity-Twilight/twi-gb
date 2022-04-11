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
// General, uncategorized utility functions of useful operations.
//----------------------------------------------------------------------
//======================================================================
#ifndef TWI_STD_UTILS_H
#define TWI_STD_UTILS_H

//======================================================================
//----------------------------------------------------------------------
// Public function declarations
//----------------------------------------------------------------------
//======================================================================

//======================================================================
// decl twi_fsize()
//
// Provides the size of the file identified by `path` in bytes.
//
// Behavior is undefined if `path` does not point to a NULL-terminated
// character string.
//
// Arguments:
// * path:
// A path identifying the target file.
//
// Returns: The number of bytes contained in the resource attached to
// `stream` on success, or ((size_t)-1) on failure. On failure, `errno`
// is set and can be referenced to identify the cause of the error.
//======================================================================
size_t
twi_fsize(const char* restrict path);

#endif // TWI_STD_UTILS_H

