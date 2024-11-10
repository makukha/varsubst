#include <stdlib.h>
#include "vsub_io.h"


void vsub_SetTextSrc(Vsub *sub, VsubTextSrc *src) {
    if (sub->tsrc) {
        free(sub->tsrc);
    }
    sub->tsrc = src;
}

void vsub_AddVarsSrc(Vsub *sub, VsubVarsSrc *src) {
    src->prev = sub->vsrc;
    sub->vsrc = src;
}
