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
// TODO: document
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
// TODO: document
//=======================================================================
size_t
prx_incbuf_putc(struct prx_incbuf* restrict dst, char src);

//=======================================================================
// doc prx_incbuf_puts()
// TODO: document
//=======================================================================
size_t
prx_incbuf_puts(
		struct prx_incbuf* restrict dst,
		const char* restrict src);

//=======================================================================
// doc prx_incbuf_printf()
// TODO: document
//=======================================================================
size_t
prx_incbuf_printf(
		struct prx_incbuf* restrict dst,
		const char* restrict fmt, ...);

//=======================================================================
// doc prx_incbuf_terminate()
// TODO: document
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

