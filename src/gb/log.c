#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "gb/log.h"

const char*
gb_log_level_short_str(int level) {
	switch (level) {
		case LVL_FTL: return "FTL";
		case LVL_ERR: return "ERR";
		case LVL_WRN: return "WRN";
		case LVL_INF: return "INF";
		case LVL_DBG: return "DBG";
		case LVL_TRC: return "TRC";
		default:      return "UNK";
	} // end switch (level)
} // end gb_log_level_str()

void
gb_log(const char* restrict filename, const char* restrict funcname,
		long lineno, int level, const char* restrict fmtstr, ...) {
	static const char file_prefix[] = "src/";
	if (!strncmp(filename, file_prefix, sizeof(file_prefix-1)))
		filename += sizeof(file_prefix-1); // Skip prefix.

	char buf[4096];
	fprintf(stderr, "%s:%s():%ld [%s]: ",
			filename, funcname, lineno,
			gb_log_level_short_str(level));

	va_list vargs;
	va_start(vargs, fmtstr);
	vfprintf(stderr, fmtstr, vargs);
	va_end(vargs);

	fputc('\n', stderr);
} // end gb_log()

