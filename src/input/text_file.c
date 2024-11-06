#include <stdio.h>
#include <stdlib.h>
#include "../vsub.h"


static const char *NAME = "file";

typedef struct VsubTextFile {
    struct VsubTextSrc super;
    FILE *fp;
} VsubTextFile;

static char _getchar(VsubTextFile *src) {
    char c = fgetc(src->fp);
    return (c == EOF) ? '\0' : c;
}

bool vsub_use_text_from_file(Vsub *sub, FILE *fp) {
    VsubTextFile *src = malloc(sizeof(VsubTextFile));
    if (!src) {
        return false;
    }
    ((VsubTextSrc *)src)->name = NAME;
    ((VsubTextSrc *)src)->getchar = (char (*)(void *))_getchar;
    src->fp = fp;
    aux_set_tsrc(&(sub->aux), (VsubTextSrc *)src);
    return true;
}
