////////////////////////////////////////////////////////////////////////////////
// Debug
////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_INCLUDE_H
#define DEBUG_INCLUDE_H

#include "../standard/standard.h"

extern bool DEBUG_ENABLED;

#define debug(FMT) \
if (DEBUG_ENABLED) {\
  char pthreadName[32] = {0};\
  pthread_getname_np(pthread_self(), pthreadName, sizeof(pthreadName));\
  printf("[%s] " FMT "\n", pthreadName);\
  fflush(stdout); \
}

#define debugf(FMT, ...) \
if (DEBUG_ENABLED) {\
  char pthreadName[32] = {0};\
  pthread_getname_np(pthread_self(), pthreadName, sizeof(pthreadName));\
  printf("[%s] " FMT "\n", pthreadName, ##__VA_ARGS__);\
  fflush(stdout); \
}

#endif
