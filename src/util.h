#ifndef VSUB_UTIL_H
#define VSUB_UTIL_H

#include <stdbool.h>
#include <stdio.h>


// useful macros

#define CNT(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define FIT(x,min,max) (MIN(MAX((x),(min)),(max)))


// dynamically growing sprintf

char *asprintf(const char *format, ...);


// simple resizable pointer array

typedef struct PtrArray {
    void **items;
    size_t count;
    size_t avail;
} PtrArray;

#define ARR_MIN_EXTRA 4
#define ARR_EXTRA_FACTOR 2
#define ARR_MAX_EXTRA 4096

void arr_init(PtrArray *arr);
bool arr_realloc(PtrArray *arr, size_t count);
void arr_free(PtrArray *arr);
bool arr_append(PtrArray *arr, void *ptr);


#endif  // VSUB_UTIL_H
