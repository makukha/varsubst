#include <stdio.h>
#include <stdlib.h>
#include "../vsub_io.h"


static const char *NAME = "file";

typedef struct VsubTextFile {
    struct VsubTextSrc super;
    FILE *fp;
    bool eof;
} VsubTextFile;

static int _getchar(VsubTextFile *src) {
    if (src->eof) {  // after EOF reached
        return -1;
    }
    char c = fgetc(src->fp);
    if (c < 0) {  // EOF reached
        src->eof = true;
        return -1;
    }
    return (int)c;
}

bool vsub_UseTextFromFile(Vsub *sub, FILE *fp) {
    VsubTextFile *src = malloc(sizeof(VsubTextFile));
    if (!src) {
        return false;
    }
    ((VsubTextSrc *)src)->name = NAME;
    ((VsubTextSrc *)src)->getchar = (int (*)(void *))_getchar;
    src->fp = fp;
    src->eof = false;
    vsub_set_tsrc(sub, (VsubTextSrc *)src);
    return true;
}
