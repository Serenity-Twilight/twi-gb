//======================================================================
// General utility functions too small and uncategorized to exist
// in a file with a more well-defined purpose.
//
// Written by Serenity Twilight
//======================================================================
#ifndef TWI_STD_UTILS_H
#define TWI_STD_UTILS_H

//======================================================================
// decl twi_trim()
//
// Excludes whitespace characters from the null-terminated character
// array pointed to by `str` by assigning NULL to the first byte after
// the final non-whitespace character prior to the original terminating
// NULL byte, and returning a pointer to the first non-whitespace
// character at or after the character pointed to by `str`.
//
// Note that the null-terminated character array pointed to by `str`
// MUST exist within mutable memory. String literals passed to this
// function will likely cause the process to crash, or exhibit undefined
// behavior at the very least.
//
// Parameters:
// * str - Pointer to a null-terminated character array to trim.
//
// Returns:
// Pointer to the first non-whitespace character at or after the
// character pointed to by `str`.
//======================================================================
char*
twi_trim(char* str);

//======================================================================
// decl twi_ktrim()
//
// Excludes whitespace characters from the null-terminated character
// array pointed to by `str` _without_ modifying the character array.
//
// This is done by returning a pointer to the first non-whitespace
// character at or after the character pointed to by `str`,
// and assigning the address of character immediately *after* the
// final non-whitespace character prior to the null terminator
// to the variable pointed to by `end`.
//
// Parameters:
// * end - Pointer to a character pointer. Assigned the address of the
//   character immediately following the final non-whitespace character
//   of `str`.
// * str - Pointer to a constant null-terminated character array to
//   trim.
//
// Returns:
// Pointer to the first non-whitespace character at or after the
// character pointed to by `str`.
//======================================================================
const char*
twi_ktrim(char** end, const char* str);

//======================================================================
// decl twi_btrim()
//
// Excludes leading and trailing whitespace characters from the
// null-terminated character array pointed to by `str`, and copies the
// trimmed array contents to the buffer pointed to by `buf`.
//
// The number of characters copied to `buf` will not exceed the value
// of (`bufsz - 1). The contents of `buf` will always be
// null-terminated after a copy.
//
// If the trimmed contents of `str` is copied to `buf`, then
// `buf` is returned.
//
// If the address of `str` lies within the range of the buffer
// pointed to by `buf` of size `bufsz`, behavior is undefined.
//
// If `str` == NULL or `buf` == NULL, behavior is undefined.
//
// If the null-terminated character array pointed to by `str` is
// not in need of trimming (no leading or trailing whitespace
// exists), then no copying occurs and `str` is returned.
//
// Parameters:
// * bufsz - The number of bytes the buffer at `buf` is capable of
//   holding.
// * buf - Pointer to the buffer to which the trimmed contents of `str`
//   is copied, assuming `str` is trimmed.
// * str - Pointer to a null-terminated character array to trim.
//
// Returns:
// Pointer to a null-terminated character array contents the original
// contents of `str` with leading and trailing whitespace trimmed off.
// If `str` originally contained such whitespace, this will be a
// pointer to `buf`, else it will be a pointer to `str` and the buffer
// will remain unmodified.
//======================================================================
char*
twi_btrim(size_t bufsz, char buf[bufsz], const char* str);

#endif // TWI_STD_UTILS_H

