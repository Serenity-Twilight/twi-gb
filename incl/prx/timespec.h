#ifndef PRX_TIMESPEC_H
#define PRX_TIMESPEC_H
#include <time.h>

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// doc prx_timespec_add_nsec()
// TODO
//=======================================================================
void
(prx_timespec_add_nsec)(struct timespec* restrict dst, int32_t nsec);

//=======================================================================
// doc prx_timespec_cmp()
// TODO
//=======================================================================
long long
(prx_timespec_cmp)(struct timespec* restrict lhs, struct timespec* restrict rhs);

//=======================================================================
// doc prx_timespec_sub()
// TODO
//=======================================================================
void
(prx_timespec_sub)(
		struct timespec* restrict dst,
		const struct timespec* restrict lhs,
		const struct timespec* restrict rhs);

#endif // PRX_TIMESPEC_H

#undef timespec_add_nsec
#undef timespec_cmp
#undef timespec_sub
#if PRX_TRUNCATE_PREFIX >= 1
#	define timespec_add_nsec prx_timespec_add_nsec
#	define timespec_cmp prx_timespec_cmp
#	define timespec_sub prx_timespec_sub
#endif // PRX_TRUNCATE_PREFIX >= 1

