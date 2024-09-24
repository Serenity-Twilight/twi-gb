#include <time.h>
#include "prx/timespec.h"

//=======================================================================
//-----------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------------
//=======================================================================

//=======================================================================
// def prx_timespec_add_nsec()
static inline void
(prx_timespec_add_nsec)(struct timespec* restrict dst, int32_t nsec) {
	dst->tv_nsec += nsec;
	if (dst->tv_nsec >= 1000000000) {
		// Perform arithmetic carry:
		dst->tv_sec += 1;
		dst->tv_nsec -= 1000000000;
	}
} // end prx_timespec_add_nsec()

//=======================================================================
// def prx_timespec_cmp()
static inline long long
(prx_timespec_cmp)(struct timespec* restrict lhs, struct timespec* restrict rhs) {
	long long sec_diff = lhs->tv_sec - rhs->tv_sec;
	if (sec_diff != 0)
		return sec_diff;
	return lhs->tv_nsec - rhs->tv_nsec;
} // end prx_timespec_cmp()

//=======================================================================
// def prx_timespec_sub()
static inline void
(prx_timespec_sub)(
		struct timespec* restrict dst,
		const struct timespec* restrict lhs,
		const struct timespec* restrict rhs) {
	//LOGT("lhs={sec=%lld,nsec=%lld},rhs={sec=%lld,nsec=%lld}",
			//lhs->tv_sec, lhs->tv_nsec, rhs->tv_sec, rhs->tv_nsec);
	dst->tv_sec = lhs->tv_sec - rhs->tv_sec;
	dst->tv_nsec = lhs->tv_nsec - rhs->tv_nsec;

	if (dst->tv_nsec < 0) {
		// Perform arithmetic carry:
		dst->tv_sec -= 1;
		dst->tv_nsec += 1000000000;
	}
} // end prx_timespec_sub()

