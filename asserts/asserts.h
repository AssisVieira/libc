#ifndef ASSERTS_H
#define ASSERTS_H

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

#define assertIntEqual(X, Y)                                            \
  {                                                                     \
    const int x = (X);                                                  \
    const int y = (Y);                                                  \
    if (x != y) {                                                       \
      fprintf(stderr, "assertEquals(" #X ", " #Y ") failed at %s:%d\n", \
              __FILE__, __LINE__);                                      \
      fprintf(stderr, "\texpected: %s\n", #Y);                          \
      fprintf(stderr, "\t but was: %d\n", x);                           \
      exit(-1);                                                         \
    }                                                                   \
  }

#endif