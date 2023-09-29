#include <stdarg.h>

// #define fatal(...) fatal(stderr, __VA_ARGS__)
// #define error(...) error(stderr, __VA_ARGS__)
// #define warning(...) warning(stderr, __VA_ARGS__)
// #define debug(...) debug(stderr, __VA_ARGS__)

void fatal(char *fmt, ...);
void error(char *fmt, ...);
void warning(char *fmt, ...);
void debug(char *fmt, ...);