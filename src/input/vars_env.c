#include <stdlib.h>
#include "../vsub_io.h"


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
    vsub_add_vsrc(sub, (VsubVarsSrc *)src);
    return true;
}
