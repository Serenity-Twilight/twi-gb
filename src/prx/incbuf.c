// incbuf: Incremental buffer
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "prx/incbuf.h"

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def prx_incbuf_putc()
size_t
prx_incbuf_putc(struct prx_incbuf* restrict dst, char src) {
	assert(dst != NULL);
	if (dst->pos < dst->bufsz)
		dst->buf[dst->pos] = src;
	dst->pos += 1;
	return 1;
} // end prx_incbuf_putc()

//=======================================================================
// def prx_incbuf_puts()
size_t
prx_incbuf_puts(
		struct prx_incbuf* restrict dst,
		const char* restrict src) {
	assert(dst != NULL);
	if (src == NULL)
		return 0;
	size_t srclen = strlen(src);
	if (dst->pos < dst->bufsz)
		strncpy(dst->buf + dst->pos, src, dst->bufsz - dst->pos);
	dst->pos += srclen;
	return srclen;
} // end prx_incbuf_puts()

//=======================================================================
// def prx_incbuf_printf()
size_t
prx_incbuf_printf(
		struct prx_incbuf* restrict dst,
		const char* restrict fmt, ...) {
	assert(dst != NULL);
	if (fmt == NULL)
		return 0;
	// Maximum number of characters that can be written into the buffer:
	size_t max_count = (dst->pos < dst->bufsz ? dst->bufsz - dst->pos : 0);

	va_list vargs;
	va_start(vargs, fmt);
	int formatted_length =
		vsnprintf(dst->buf + dst->pos, max_count, fmt, vargs);
	va_end(vargs);

	if (formatted_length < 0)
		return (size_t)-1; // error
	dst->pos += formatted_length;
	return formatted_length;
} // end prx_incbuf_printf()

//=======================================================================
// def prx_incbuf_terminate()
void
prx_incbuf_terminate(struct prx_incbuf* restrict dst) {
	assert(dst != NULL);
	if (dst->bufsz > 0) {
		if (dst->pos < dst->bufsz) // pos is inside of buffer
			dst->buf[dst->pos] = 0;
		else // pos is beyond end of buffer, NUL-terminate end of buffer
			dst->buf[dst->bufsz - 1] = 0;
	}
} // end prx_incbuf_terminate()

