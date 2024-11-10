#include <stdlib.h>
#include <string.h>
#include "../vsub_io.h"


static const char *NAME = "str";

typedef struct VsubTextStr {
    struct VsubTextSrc super;
    const char *str;
    size_t len;
    size_t i;
} VsubTextStr;

static int _getchar(VsubTextStr *src) {
    if (src->i >= src->len - 1) {
        return -1;
    }
    return (int)src->str[src->i++];
}

bool vsub_use_text_from_str(Vsub *sub, const char *s) {
    VsubTextStr *src = malloc(sizeof(VsubTextStr));
    if (!src) {
        return false;
    }
    ((VsubTextSrc *)src)->name = NAME;
    ((VsubTextSrc *)src)->getchar = (int (*)(void *))_getchar;
    src->str = s;
    src->len = strlen(s);
    src->i = 0;
    vsub_set_tsrc(sub, (VsubTextSrc *)src);
    return true;
}
