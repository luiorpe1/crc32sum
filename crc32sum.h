#define PROGRAM_NAME "crc32sum"
#define AUTHORS "Luis Ortega Perez de Villar"

#define STREQ(a, b) (strcmp((a), (b)) == 0)
#define max(x, y) ((x > y) ? x : y)
#define min(x, y) ((x - y) ? x : y)

#if defined(DEBUG)
#define pr_debug(fmt, ...) \
	fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif
