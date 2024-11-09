#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"


char *asprintf(const char *format, ...) {
    char *ret = NULL;
    size_t sz = 0;
    va_list args;
    va_start(args, format);
    FILE *mp = open_memstream(&ret, &sz);
    if (!mp) {
        ret = NULL;
        goto end;
    }
    if (vfprintf(mp, format, args) == EOF) {
        ret = NULL;
        goto end;
    }
end:
    va_end(args);
    fclose(mp);
    return ret;
}

int fprintfcerr(FILE *fp, bool use_color, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret;

    if ((ret = fputs(use_color ? VSUB_COLOR_ERROR : "", fp) == EOF)) {
        goto done;
    };
    if ((ret = vfprintf(fp, format, args)) == EOF) {
        goto done;
    }
    if ((ret = fputs(use_color ? "\033[0m" : "", fp)) == EOF) {
        goto done;
    }
done:
    return ret;
}
