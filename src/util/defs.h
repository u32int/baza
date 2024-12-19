// misc macro/type definitions
#ifndef _UTIL_DEFS_H
#define _UTIL_DEFS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t byte;

#define FATAL(...) do {	\
    fprintf(stderr, "["__FILE__":%d] ", __LINE__); \
    fprintf(stderr, "FATAL: "__VA_ARGS__); \
    fputc('\n', stderr); \
    exit(1); \
} while(0)

#define ENSURE(expr) do { \
  BazaResult res = expr; \
  if (res != RESULT_OK) \
     return res; \
} while(0)

#endif /* _UTIL_DEFS_H */
