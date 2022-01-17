#ifndef TWI_GB_DEBUG_H
#define TWI_GB_DEBUG_H

#include <stdint.h>

// TODO: Do not overwrite user-specific max logging level if one exists.
#ifdef TWI_GB_NDEBUG // Debugging disabled.
#	define TWI_LOG_LEVEL_GLOBAL_MAX TWI_LOG_LEVEL_INFO
#else // Debugging enabled.
# define TWI_LOG_LEVEL_GLOBAL_MAX TWI_LOG_LEVEL_TRACE
#endif

#include <twi/log.h>

extern struct twi_log twi_gb_stdlog;

// Logging macros for stdlog.
#define LOGF(msg, ...) TWI_LOGF(twi_gb_stdlog, msg, ##__VA_ARGS__)
#define LOGE(msg, ...) TWI_LOGE(twi_gb_stdlog, msg, ##__VA_ARGS__)
#define LOGW(msg, ...) TWI_LOGW(twi_gb_stdlog, msg, ##__VA_ARGS__)
#define LOGI(msg, ...) TWI_LOGI(twi_gb_stdlog, msg, ##__VA_ARGS__)
#define LOGD(msg, ...) TWI_LOGD(twi_gb_stdlog, msg, ##__VA_ARGS__)
#define LOGT(msg, ...) TWI_LOGT(twi_gb_stdlog, msg, ##__VA_ARGS__)

int_fast8_t twi_gb_stdlog_init(
		enum twi_log_level level,
		const char* progname,
		const char* directory,
		uint8_t max_files
);
void twi_gb_stdlog_destroy();

#endif // TWI_GB_DEBUG_H

