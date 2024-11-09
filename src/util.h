#ifndef VSUB_UTIL_H
#define VSUB_UTIL_H

#include <stdbool.h>
#include <stdio.h>
#include "vsub.h"


char *asprintf(const char *format, ...);
int fprintfcerr(FILE *fp, bool use_color, const char *format, ...);


#endif  // VSUB_SYNTAX_H
