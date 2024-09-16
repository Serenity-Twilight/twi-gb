//=======================================================================
//-----------------------------------------------------------------------
// incl/prx/incbuf.h
//
// An "incrementing buffer", which is a pre-allocated one-dimensional
// array paired with its total size and current position in the buffer.
//
// The main feature of an incrementing buffer is that any insertions
// attempted that would exceed the total size of the buffer are silently
// dropped, rather than raising an error or requiring the user to
// check the buffer's current fill position manually.
//
// Incrementing buffers simplify buffer-filling operations in which:
// * small pieces of data are inserted into the buffer frequently,
// * the final amount of the data is not expected to exceed the buffer's
//   total size, and
// * its inconsequential if data exceeding the buffer's limit is lost
//
// This makes incrementing buffers ideal for concatenating complex
// text dumps of indeterminate length before feeding the text through
// a more complicated interface.
//-----------------------------------------------------------------------
//=======================================================================
#ifndef PRX_INCBUF_H
#define PRX_INCBUF_H
#include <stddef.h>

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL TYPE DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc struct prx_incbuf
//
// The incrementing buffer, or incbuf, object.
//
// No initialization or destruction functions are provided.
// The contents of an incbuf must be initialized and destroyed by the
// user.
//-----------------------------------------------------------------------
// Members:
// * buf:
//   Pointer to the buffer.
//   Allocated memory region pointed to by `buf` should be at least
//   `bufsz` bytes in size.
// * bufsz:
//   The maximum number of bytes that this buffer can contain before it
//   is full. `buf` should point to a memory region at least this large,
//   else behavior is undefined.
// * pos:
//   The current write position in the buffer, and the number of bytes
//   that have been filled already. Should be initialized to `0` before
//   first usage. Writes will begin at this position in the buffer, and
//   increment the value of `pos` automatically. If `pos >= bufsz`,
//   further writes will be ignored and not be incremented to `buf`.
//
//   Even if the buffer is full (`pos >= bufsz`), pos will continue to
//   increment by the number of characters each insertion would add as if
//   the buffer were not full. This means that `pos` will indicate the
//   total number of bytes that insertion was attempted on, regardless
//   of whether or not any were ignored.
//=======================================================================
struct prx_incbuf {
	char* buf;
	size_t bufsz;
	size_t pos;
}; // end struct prx_incbuf

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc prx_incbuf_putc()
//
// If the buffer is not full, increments `src` to the end of `dst->buf`.
// Regardless of buffer fill, `dst->pos` is incremented.
//-----------------------------------------------------------------------
// Parameters:
// * dst:
//   Pointer to incrementing buffer object to read and alter.
// * src:
//   The character to append to the buffer.
//
// Returns:
// The number of characters insertion was attempted with, which will
// always be `1` for this function.
//=======================================================================
size_t
prx_incbuf_putc(struct prx_incbuf* restrict dst, char src);

//=======================================================================
// doc prx_incbuf_puts()
//
// Attempts to insert the contents `src` into `dst`'s buffer, up to the
// maximum buffer capacity of `dst`.
// `dst->pos` will always be incremented by `strlen(src)`, regardless
// of the capacity of `dst`.
//-----------------------------------------------------------------------
// Parameters:
// * dst:
//   Pointer to the incrementing buffer object to read and alter.
// * src:
//   Pointer to a NUL-terminated character string to attempt to append
//   to `dst`.
//
// Returns:
// The number of characters in `src`, excluding the NUL-terminator.
//=======================================================================
size_t
prx_incbuf_puts(
		struct prx_incbuf* restrict dst,
		const char* restrict src);

//=======================================================================
// doc prx_incbuf_printf()
//
// Attempts to append the format string `fmt` after substitution using
// the provided variable argument list onto the end of `dst`.
//
// `dst->pos` will be incremented by the length of the format string
// after substitution (not including NUL-terminator), regardless of
// whether `dst` was large enough to contain any or all of the format
// string after substitution.
//-----------------------------------------------------------------------
// Parameters:
// * dst:
//   Pointer to the incrementing buffer object to read and alter.
// * fmt:
//   Format string containing format specifiers compatible with `printf()`.
// * ...:
//   0 or more values for substitute into `fmt`, in order of the appearance
//   of substitution tokens in `fmt`.
//
// Return:
// The number of characters in `fmt` after substitution using variable
// arguments.
//=======================================================================
size_t
prx_incbuf_printf(
		struct prx_incbuf* restrict dst,
		const char* restrict fmt, ...);

//=======================================================================
// doc prx_incbuf_terminate()
//
// Ensures `dst`'s buffer is NUL-terminated.
//
// If the buffer IS NOT full, then NUL is appended to the buffer.
// The position value is NOT incremented (further insertions will
// overwrite the NUL-terminator).
//
// If the buffer IS full, then NUL replaces the byte at the end of the
// buffer. The position value is NOT incremented.
//-----------------------------------------------------------------------
// Parameters:
// * dst:
//   Pointer to the incrementing buffer object to NUL-terminate.
//=======================================================================
void
prx_incbuf_terminate(struct prx_incbuf* restrict dst);

#endif // PRX_INCBUF_H

#if PRX_TRUNCATE_PREFIX >= 1
#	define incbuf prx_incbuf
#	define incbuf_putc prx_incbuf_putc
#	define incbuf_puts prx_incbuf_puts
#	define incbuf_printf prx_incbuf_printf
#	define incbuf_terminate prx_incbuf_terminate
#endif // PRX_TRUNCATE_PREFIX >= 1

