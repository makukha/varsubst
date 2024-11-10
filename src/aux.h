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
    void *sub;  // todo: can safely type now
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
#define PCC_ERROR(auxil) { ((Vsub*)auxil->sub)->err = VSUB_ERROR_SYNTAX; return 0; }
#define PCC_GETCHAR(auxil) auxil->getchar(auxil)

// actions
#define use_Input auxil->append_orig(auxil, _0e, _0)
#define use_Value auxil->append_subst(auxil, _0e, __val)
#define use_Other auxil->append_subst(auxil, _0e, other)
#define use_Error { auxil->append_error(auxil, _0e, var, errmsg); return 0; }
#define USE(a) use_ ## a;

// rules
#define __get_Value const char *__val = auxil->getvalue(auxil, var)
#define __is_Set (__val != NULL)
#define __is_Empty (__val != NULL && strlen(__val) == 0)
#define __is_Filled (__val != NULL && strlen(__val) >= 1)
#define __is_Missing (__val == NULL || strlen(__val) == 0)
#define IF(s) { __get_Value; if __is_ ## s
#define THEN(a) use_ ## a;
#define ELSE(a) else use_ ## a; }


#endif  // VSUB_AUX_H
