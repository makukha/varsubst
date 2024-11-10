#ifndef VSUB_AUX_H
#define VSUB_AUX_H

#include "vsub.h"


// --- parsers

// implemented syntax parser description
typedef struct VsubParser {
    void *(*create)(void *aux);
    int (*parse)(void *ctx, void *ret);
    void (*destroy)(void *ctx);
} VsubParser;

// array of implemented syntax parser descriptions
extern const VsubParser VSUB_PARSERS[];  // using VSUB_SX_* as indexes
#define VSUB_PARSERS_COUNT VSUB_SYNTAXES_COUNT


#endif  // VSUB_AUX_H
