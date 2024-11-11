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


// --- auxiliary object

typedef struct Auxil {
    Vsub *sub;
    // syntax methods
    int (*getchar)(void *aux);
    const char *(*getvalue)(void *aux, const char *var);
    bool (*append_orig)(void *aux, int epos, const char *str);
    bool (*append_subst)(void *aux, int epos, const char *str);
    bool (*append_error)(void *aux, int epos, const char *errvar, const char* errmsg);
    // data
    char *resbuf;  // result buffer
    size_t resz;   // result buffer size
    char *errbuf;  // error buffer
    size_t errz;   // error buffer size
    // parser
    const VsubParser *parser;
    void *pctx;
} Auxil;

// buffer management constants
#define VSUB_BRES_MIN 256  // initial result buffer size
#define VSUB_BRES_INC 1024 // additional free space reserved on every reallocation
#define VSUB_BERR_MIN 256  // initial error buffer size


// --- parser generator configuration

// parser generator settings
#define PCC_ERROR(auxil) { ((Vsub*)auxil->sub)->err = VSUB_ERR_SYNTAX; return 0; }
#define PCC_GETCHAR(auxil) auxil->getchar(auxil)


// --- parser grammar helpers

// actions
#define _use_Input    { auxil->append_orig(auxil, _0e, _0); }
#define _use_Const(s) { auxil->append_orig(auxil, _0e, s); }
#define _use_Value    { auxil->append_subst(auxil, _0e, __tmp); }
#define _use_Other(s) { auxil->append_subst(auxil, _0e, s); }
#define _use_Error(e) { auxil->append_error(auxil, _0e, __tmp, e); return 0; }
#define USE(a) _use_ ## a;

// rules
#define _get_Value(v)  const char *__tmp = auxil->getvalue(auxil, v)
#define _if_Set(v)     _get_Value(v); if(__tmp != NULL)
#define _if_Empty(v)   _get_Value(v); if(__tmp != NULL && strlen(__tmp) == 0)
#define _if_Filled(v)  _get_Value(v); if(__tmp != NULL && strlen(__tmp) >= 1)
#define _if_Missing(v) _get_Value(v); if(__tmp == NULL || strlen(__tmp) == 0)
#define IF(s)   { _if_ ## s
#define THEN(a) _use_ ## a
#define ELSE(a) else _use_ ## a }


#endif  // VSUB_AUX_H
