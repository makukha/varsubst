#include <stdlib.h>
#include <string.h>
#include "../vsub.h"


static const char *NAME = "arrays";

typedef struct VsubVarsArrays {
    struct VsubVarsSrc super;
    const char **keys;
    const char **vals;
    int count;
} VsubVarsArrays;

static const char *_getvalue(VsubVarsArrays *src, const char *var) {
    for (int i = 0; i < src->count; i++) {
        if (strcmp(var, src->keys[i]) == 0) {
            return src->vals[i];
        }
    }
    return NULL;
}

bool vsub_add_vars_from_arrays(Vsub *sub, int c, const char *k[], const char *v[]) {
    VsubVarsArrays *src = malloc(sizeof(VsubVarsArrays));
    if (!src) {
        return false;
    }
    ((VsubVarsSrc *)src)->name = NAME;
    ((VsubVarsSrc *)src)->getvalue = (const char *(*)(void *, const char *))_getvalue;
    src->keys = k;
    src->vals = v;
    src->count = c;
    vsub_add_vsrc(sub, (VsubVarsSrc *)src);
    return true;
}
