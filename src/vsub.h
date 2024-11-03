#ifndef VSUB_H
#define VSUB_H

#define VSUB_VERSION "0.1.0"

#ifdef _WIN32
  #define VSUB_EXPORT __declspec(dllexport)
#else
  #define VSUB_EXPORT
#endif

#include <stddef.h>

// exports

extern const int VSUB_SXS_LIST[];
extern const size_t VSUB_SXS_COUNT;

VSUB_EXPORT void vsub();
VSUB_EXPORT const char* vsub_syntax_short_str(int sx);
VSUB_EXPORT const char* vsub_syntax_long_str(int sx);
VSUB_EXPORT const char* vsub_missing_str(char mv);

// missing var types

#define VSUB_MV_VALID 0
#define VSUB_MV_UNSET 1
#define VSUB_MV_EMPTY 2

// syntax dialects

#define VSUB_SX_DEFAULT 0
#define VSUB_SX_DC243 1

#endif  // VSUB_H
