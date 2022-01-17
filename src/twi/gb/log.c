// TODO: File description.
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <twi/gb/log.h>

//======================================================================
//======================================================================
// Internal function declarations
//======================================================================
//======================================================================

//======================================================================
//======================================================================
// External variable declarations
//======================================================================
//======================================================================
struct twi_log twi_gb_stdlog;

//======================================================================
//======================================================================
// External function definitions
//======================================================================
//======================================================================

//======================================================================
// def twi_gb_stdlog_init()
//======================================================================
int_fast8_t
twi_gb_stdlog_init(
		enum twi_log_level level,
		const char* progname,
		const char* directory,
		uint8_t max_files
) {
	// Clamp `level` into an existing range.
	// Keep the original level so as to be able to report this oddity.
	enum twi_log_level clamped_level;
	if (level < TWI_LOG_LEVEL_NONE)
		clamped_level = TWI_LOG_LEVEL_NONE;
	else if (level > TWI_LOG_LEVEL_GLOBAL_MAX)
		clamped_level = TWI_LOG_LEVEL_GLOBAL_MAX;
	else
		clamped_level = level;

	// Initialize log.
	twi_log_init(&twi_gb_stdlog, clamped_level);
	if (twi_gb_stdlog.max_lvl <= TWI_LOG_LEVEL_NONE) {
		// No log messages will be written, so no output stream is necessary.
		return 0;
	}

	// Generate the new log file pathname prefix.
	char prefix[TWI_LOG_PATHNAME_BUFSZ];
	size_t idx = 0;
	// Copy `directory` into `prefix`.
	while (idx < TWI_LOG_PATHNAME_BUFSZ && *directory != 0)
		prefix[idx++] = *(directory++);
	// Append forward slash to the end of `prefix` if `directory` did not
	// end with one already.
	if (prefix[idx - 1] != '/' && idx < (TWI_LOG_PATHNAME_BUFSZ - 1))
		prefix[idx++] = '/';
	// Append `progname` onto `prefix`.
	while (idx < TWI_LOG_PATHNAME_BUFSZ && *progname != 0)
		prefix[idx++] = *(progname++);
	// Check if the combined contents of `directory` and `progname` did
	// not exceed the size of the prefix buffer. If they did, fail.
	if (idx >= (TWI_LOG_PATHNAME_BUFSZ - 2))
		return 1;
	// Append period (to separate progname from timestamp/index) and null byte.
	prefix[idx] = '.';
	prefix[idx + 1] = 0;

	// Open log output stream.
	int_fast8_t twi_log_status;
	if (max_files == 0) {
		// No maximum. Use timestamped file names to ensure that no
		// file overwrites any existing file.
		twi_log_status = twi_log_stream_open_timestamped(
				&twi_gb_stdlog, prefix, ".log");
	} else {
		// Use indexed filenames to ensure that once the maximum file
		// limit has been reached, the oldest file is automatically 
		// overwritten.
		twi_log_status = twi_log_stream_open_indexed(
				&twi_gb_stdlog, prefix, ".log", max_files);
	}

	// Check if an error prevented the output stream from opening.
	// If so, report to stderr and return failure.
	if (twi_log_status) {
		if (twi_log_status < 0) {
			// libc error
			fprintf(stderr,
					__FILE__ ":%s: Unable to open twi_log.\n"
					"\tlibc: %s error (%s)\n"
					"\tprefix: \"%s\"\n"
					"\tsuffix: \".log\"\n"
					"\tmax_files: %" PRIu8 "\n",
					__func__, twi_log_err_libc,
					(errno ? strerror(errno) : "errno not set"),
					prefix, max_files);
		} else {
			// Requested pathname too big.
			fprintf(stderr,
					__FILE__ ":%s: Unable to open twi_log.\n"
					"\tRequested pathname exceeds TWI_LOG_PATHNAME_BUFSZ (%d)\n"
					"\tprefix: \"%s\"\n"
					"\tsuffix: \".log\"\n"
					"\tmax_files: %" PRIu8 "\n",
					__func__, TWI_LOG_PATHNAME_BUFSZ, prefix, max_files);
		}
		return twi_log_status;
	}

	// Log file opened successfully.
	// Finish configuring remaining twi_log settings.
	twi_gb_stdlog.implicit_fp = "src/twi/";

	// If log has been successfully opened, and the caller's provided
	// logging level had to be clamped, report now.
	if (level != clamped_level) {
		// TODO: Non-numeric logging levels.
		LOGW("Logging level clamped to %d (originally was %d).",
				clamped_level, level);
	}
	return 0;
} // end twi_gb_stdlog_init()

//======================================================================
// def twi_gb_stdlog_destroy()
//======================================================================
void
twi_gb_stdlog_destroy() {
	twi_log_destroy(&twi_gb_stdlog);
} // end twi_gb_stdlog_destroy()

