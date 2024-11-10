#include <stdlib.h>
#include <string.h>
#include "../vsub.h"


static const char *NAME = "arglist";

typedef struct VsubVarsArglist {
    struct VsubVarsSrc super;
    const char **kv;
    int count;
} VsubVarsArglist;

static const char *_getvalue(VsubVarsArglist *src, const char *var) {
    size_t len = strlen(var);
    for (int i = 0; i < src->count; i++) {
        if (strncmp(var, src->kv[i], len) == 0 && src->kv[i][len] == '=') {
            return &(src->kv[i][len + 1]);
        }
    }
    return NULL;
}

bool vsub_add_vars_from_arglist(Vsub *sub, int c, const char *kv[]) {
    VsubVarsArglist *src = malloc(sizeof(VsubVarsArglist));
    if (!src) {
        return false;
    }
    ((VsubVarsSrc *)src)->name = NAME;
    ((VsubVarsSrc *)src)->getvalue = (const char *(*)(void *, const char *))_getvalue;
    src->kv = kv;
    src->count = c;
    vsub_add_vsrc(sub, (VsubVarsSrc *)src);
    return true;
}
