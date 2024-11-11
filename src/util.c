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


// simple pointer array

void arr_init(PtrArray *arr) {
  arr->items = NULL;
  arr->count = 0;
  arr->avail = 0;
}

bool arr_realloc(PtrArray *arr, size_t count) {
    if (count <= arr->avail) {
        return true;
    }
    size_t extra = FIT(arr->count * ARR_EXTRA_FACTOR, ARR_MIN_EXTRA, ARR_MAX_EXTRA);
    size_t sz = (count + extra) * sizeof(void *);
    void *newitems = NULL;
    if (!(newitems = realloc(arr->items, sz))) {
        return false;
    }
    arr->items = newitems;
    arr->avail = count + extra;
    return true;
}

void arr_free(PtrArray *arr) {
    free(arr->items);
    arr->items = NULL;
    arr->count = arr->avail = 0;
}

bool arr_append(PtrArray *arr, void *ptr) {
    if (!arr_realloc(arr, arr->count + 1)) {
        return false;
    }
    arr->items[arr->count] = ptr;
    arr->count++;
    return true;
}
