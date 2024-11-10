#include <stdio.h>
#include "../vsub.h"


int vsub_OutputPlain(Vsub *sub, FILE *fp) {
    if (sub->res) {
        if (fprintf(fp, "%s", sub->res) < 0) {
            return VSUB_ERR_FILE_WRITE;
        }
    }
    return VSUB_SUCCESS;
}
