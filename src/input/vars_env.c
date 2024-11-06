#include <stdlib.h>
#include "../vsub.h"


static const char *NAME = "env";

typedef struct VsubVarsEnv {
    struct VsubVarsSrc super;
} VsubVarsEnv;

static const char *_getvalue(VsubVarsEnv *src, const char *var) {
    return getenv(var);
}

bool vsub_add_vars_from_env(Vsub *sub) {
    VsubVarsEnv *src = malloc(sizeof(VsubVarsEnv));
    if (!src) {
        return false;
    }
    ((VsubVarsSrc *)src)->name = NAME;
    ((VsubVarsSrc *)src)->getvalue = (const char *(*)(void *, const char *))_getvalue;
    aux_add_vsrc(&(sub->aux), (VsubVarsSrc *)src);
    return true;
}
