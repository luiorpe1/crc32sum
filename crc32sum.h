/*#define DEBUG*/
#if defined(DEBUG)
#define pr_debug(fmt, ...) \
	fprintf(stdout, fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif
