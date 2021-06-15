#ifndef TWI_LOG_H
#define TWI_LOG_H

#include <stdint.h>
#include <time.h>

#ifndef TWI_LOG_GLOBAL_MAX_LEVEL
#	define TWI_LOG_GLOBAL_MAX_LEVEL TWI_LOG_LEVEL_TRACE
#endif

enum twi_log_level {
	TWI_LOG_LEVEL_NONE,
	TWI_LOG_LEVEL_FATAL,
	TWI_LOG_LEVEL_ERROR,
	TWI_LOG_LEVEL_WARN,
	TWI_LOG_LEVEL_INFO,
	TWI_LOG_LEVEL_DEBUG,
	TWI_LOG_LEVEL_TRACE
};

struct twi_log {
	// public
	uint8_t max_level;
	// private
	struct {
		struct timespec epoch;
	} priv;
};

//======================================================================
// decl TWI_LOG()
//
// General-purpose logging macro for struct twi_log.
//
// If lvl is less than or equal to the compile-time global max logging
// level as well as the logging level of the twi_log pointed to by
// log_ptr, the null-terminated character array pointed to by msg
// is written to all output streams associated with the twi_log
// pointed to by log_ptr, with substitution upon the null-terminated
// character array pointed to by msg alike to that of stdio's printf
// family of functions using the variadic argument list following
// msg.
//
// The contents of msg is prefixed with a timestamp indicating the
// time since the initialization of the twi_log pointed to by
// log_ptr, the file name and line number in which TWI_LOG was
// invoked, and the human-readable format of the logging level. 
// The contents of msg are suffixed by a line feed.
// TODO: Timestamp resolution/format
//
// If lvl is greater than either the global max logging level or the
// max logging level of the twi_log pointed to by log_ptr, or lvl is
// equal to TWI_LOG_LEVEL_NONE, or log_ptr points to NULL, then
// TWI_LOG does nothing.
//
// Parameters:
// * struct twi_log* log_ptr - Pointer to an initialized twi_log to
//                             write to the file streams of.
// * enum twi_log_level lvl - One of the values of twi_log_level,
//                            excluding TWI_LOG_LEVEL_NONE.
// * const char* msg - Pointer to a null-terminated character string,
//                     containing a number of printf-alike
//                     substitutions equal in number to the number of
//                     variadic arguments.
// * ... - Variadic argument list, matching number and respective types
//         to the substitution escape sequences in msg.
//======================================================================
#define TWI_LOG(log_ptr, lvl, msg, ...) \
	if (lvl != TWI_LOG_LEVEL_NONE \
	 && lvl <= TWI_LOG_GLOBAL_MAX_LEVEL \
	 && log_ptr && lvl <= log_ptr->max_level) { \
		twi_log_write( \
				log_ptr, \
				lvl, \
				__FILE__, \
				__LINE__, \
				msg, \
				##__VA_ARGS__ \
		); \
	}

//======================================================================
// decl twi_log_write()
//
// DO NOT CALL THIS FUNCTION DIRECTLY!
// Instead, invoke this function via the TWI_LOG macro, or one of its
// log level-specific variants. 
//
// For optimization reasons, log level checking is done at the call
// site via the aforementioned macros. Invoking this function directly
// ignores maximum log level configuration and requires the caller to
// be responsible for providing additional callsite metadata.
// TODO: Make arguments anonymous.
//======================================================================
void
twi_log_write(
		struct twi_log* log,
		enum twi_log_level level,
		const char* file,
		long line,
		const char* msg,
		...
);

#endif // TWI_LOG_H
