#ifndef GB_LOG_H
#define GB_LOG_H

#define LVL_NONE 0
#define LVL_FTL 1
#define LVL_ERR 2
#define LVL_WRN 3
#define LVL_INF 4
#define LVL_DBG 5
#define LVL_TRC 6

const char*
gb_log_level_short_str(int level);
void
gb_log(const char* restrict filename, const char* restrict funcname,
		long lineno, int level, const char* restrict fmtstr, ...);

#define GB_LOG(level, fmtstr, ...) \
	if (level <= GB_LOG_MAX_LEVEL) \
		gb_log(__FILE__, __func__, __LINE__, level, fmtstr, ##__VA_ARGS__)

#define LOGF(fmtstr, ...) GB_LOG(LVL_FTL, fmtstr, ##__VA_ARGS__)
#define LOGE(fmtstr, ...) GB_LOG(LVL_ERR, fmtstr, ##__VA_ARGS__)
#define LOGW(fmtstr, ...) GB_LOG(LVL_WRN, fmtstr, ##__VA_ARGS__)
#define LOGI(fmtstr, ...) GB_LOG(LVL_INF, fmtstr, ##__VA_ARGS__)
#define LOGD(fmtstr, ...) GB_LOG(LVL_DBG, fmtstr, ##__VA_ARGS__)
#define LOGT(fmtstr, ...) GB_LOG(LVL_TRC, fmtstr, ##__VA_ARGS__)

#endif // GB_LOG_H

