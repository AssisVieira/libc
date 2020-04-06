#ifndef ASSERTS_H
#define ASSERTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define assert(X)                                                            \
  if (!(X)) {                                                                \
    fprintf(stderr, "assert(" #X ") failed at %s:%d\n", __FILE__, __LINE__); \
    exit(-1);                                                                \
  }

#define assertNull(X)                                                 \
  if ((X) != NULL) {                                                  \
    fprintf(stderr, "assertNull(" #X ") failed at %s:%d\n", __FILE__, \
            __LINE__);                                                \
    exit(-1);                                                         \
  }

#define assertNotNull(X)                                                 \
  if ((X) == NULL) {                                                     \
    fprintf(stderr, "assertNotNull(" #X ") failed at %s:%d\n", __FILE__, \
            __LINE__);                                                   \
    exit(-1);                                                            \
  }

void assertNumEqual(long long x, long long y, const char *X, const char *Y,
                    const char *file, size_t line) {
  if (x != y) {
    fprintf(stderr, "assertEqual(%s, %s) failed at %s:%lu\n", X, Y, file, line);
    fprintf(stderr, "\texpected: %s\n", Y);
    fprintf(stderr, "\t but was: %lld\n", x);
    exit(-1);
  }
}

void assertUnsignedNumEqual(unsigned long long x, unsigned long long y,
                            const char *X, const char *Y, const char *file,
                            size_t line) {
  if (x != y) {
    fprintf(stderr, "assertEqual(%s, %s) failed at %s:%lu\n", X, Y, file, line);
    fprintf(stderr, "\texpected: %s\n", Y);
    fprintf(stderr, "\t but was: %llu\n", x);
    exit(-1);
  }
}

#define assertStrEqual(X, Y)                                               \
  {                                                                        \
    const char *x = (X);                                                   \
    const char *y = (Y);                                                   \
    if (x != y && strcmp(x, y) != 0) {                                     \
      fprintf(stderr, "assertStrEquals(" #X ", " #Y ") failed at %s:%d\n", \
              __FILE__, __LINE__);                                         \
      fprintf(stderr, "\texpected: %s\n", Y);                              \
      fprintf(stderr, "\t but was: %s\n", x);                              \
      exit(-1);                                                            \
    }                                                                      \
  }

#define assertPtrEqual(X, Y)                                               \
  {                                                                        \
    const void *x = (X);                                                   \
    const void *y = (Y);                                                   \
    if (x != y) {                                                          \
      fprintf(stderr, "assertPtrEquals(" #X ", " #Y ") failed at %s:%d\n", \
              __FILE__, __LINE__);                                         \
      fprintf(stderr, "\texpected: %p\n", Y);                              \
      fprintf(stderr, "\t but was: %p\n", x);                              \
      exit(-1);                                                            \
    }                                                                      \
  }

#define assertEqual(X, Y)                           \
  _Generic((X) + 0, long long                       \
           : assertNumEqual, long                   \
           : assertNumEqual, int                    \
           : assertNumEqual, short                  \
           : assertNumEqual, unsigned long long     \
           : assertUnsignedNumEqual, unsigned long  \
           : assertUnsignedNumEqual, unsigned int   \
           : assertUnsignedNumEqual, unsigned short \
           : assertUnsignedNumEqual)(X, Y, #X, #Y, __FILE__, __LINE__)

#endif