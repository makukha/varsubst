#include <stdlib.h>
#include <string.h>
#include "../vsubio.h"


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

bool vsub_UseTextFromStr(Vsub *sub, const char *s) {
    VsubTextStr *src = malloc(sizeof(VsubTextStr));
    if (!src) {
        return false;
    }
    ((VsubTextSrc *)src)->name = NAME;
    ((VsubTextSrc *)src)->getchar = (int (*)(void *))_getchar;
    src->str = s;
    src->len = strlen(s);
    src->i = 0;
    vsub_SetTextSrc(sub, (VsubTextSrc *)src);
    return true;
}
