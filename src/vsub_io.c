#include <stdlib.h>
#include "vsub_io.h"


void vsub_set_tsrc(Vsub *sub, VsubTextSrc *src) {
    if (sub->tsrc) {
        free(sub->tsrc);
    }
    sub->tsrc = src;
}

void vsub_add_vsrc(Vsub *sub, VsubVarsSrc *src) {
    src->prev = sub->vsrc;
    sub->vsrc = src;
}
