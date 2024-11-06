#ifndef VSUB_H
#define VSUB_H

#define VSUB_VERSION "0.1.0"

#ifdef _WIN32
  #define VSUB_EXPORT __declspec(dllexport)
#else
  #define VSUB_EXPORT
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>


// --- Internals ---

// input base
typedef struct VsubTextSrc {
    const char *name;
    char (*getchar)(void *src);
} VsubTextSrc;
typedef struct VsubVarsSrc {
    const char *name;
    const char *(*getvalue)(void *src, const char *var);
    void *prev;
} VsubVarsSrc;

// auxiliary struct
typedef struct Auxil {
    void *sub;
    // syntax methods
    char (*getchar)(void *aux);
    const char *(*getvalue)(void *aux, const char *var);
    bool (*append_orig)(void *aux, const char *str);
    bool (*append_subst)(void *aux, const char *str);
    bool (*append_error)(void *aux, const char *errvar, const char* errmsg);
    // data
    VsubTextSrc *tsrc;
    VsubVarsSrc *vsrc;
    char *bres;    // internally allocated result buffer, if needed
    size_t bresz;  // result buffer size
    char *berr;    // buffer for errvar & errmsg
    size_t berrz;  // error buffer size
} Auxil;

// input helpers
void aux_set_tsrc(Auxil *aux, VsubTextSrc *src);
void aux_add_vsrc(Auxil *aux, VsubVarsSrc *src);

// buffer constants
#define VSUB_BRES_MIN 256  // initial result buffer size
#define VSUB_BRES_INC 1024 // additional free space reserved on every reallocation
#define VSUB_BERR_MIN 256  // initial error buffer size


// --- API ---

// syntaxes
typedef struct VsubSyntax {
    const int id;
    const char *name;
    const char *title;
} VsubSyntax;
VSUB_EXPORT VsubSyntax *vsub_find_syntax(char *name);  // find by name
extern const VsubSyntax VSUB_SYNTAX[];                 // uses VSUB_SX_* as indexes
extern const size_t VSUB_SYNTAX_COUNT;
#define VSUB_SX_DEFAULT 0
#define VSUB_SX_DC243 1
#define VSUB_SX_GGENV 2

// results and params
typedef struct Vsub {
    // params
    VsubSyntax *syntax;  // syntax dialect, one of VSUB_SX_*; 0 by default
    char depth;     // max subst iter count; default: 1
    size_t maxinp;  // max length of input string; unlim if set to 0
    size_t maxres;  // max length of result string, incl. term. zero; unlim if set to 0
    // result
    char *res;      // result string; points to input if plain=1 and copyplain=0
    char err;       // see error/success flags
    char *errvar;   // first variable name with error; NULL by default
    char *errmsg;   // variable error message; NULL by default
    bool trunc;     // whether result string was truncated because of maxinp or maxres
    size_t inpc;    // consumed length of input str
    size_t resc;    // actual length of result str
    size_t substc;  // count of total substitutions made
    char depthc;    // count of subst iterations actually performed
    // internal data
    Auxil aux;
} Vsub;

VSUB_EXPORT void vsub_init(Vsub *sub);
VSUB_EXPORT bool vsub_run(Vsub *sub);
VSUB_EXPORT void vsub_free(Vsub *sub);
// input text
VSUB_EXPORT bool vsub_use_text_from_file(Vsub *sub, FILE *fp);
VSUB_EXPORT bool vsub_use_text_from_str(Vsub *sub, const char *s);
// input vars
VSUB_EXPORT bool vsub_add_vars_from_arglist(Vsub *sub, int c, const char *kv[]);
VSUB_EXPORT bool vsub_add_vars_from_arrays(Vsub *sub, int c, const char *k[], const char *v[]);
VSUB_EXPORT bool vsub_add_vars_from_env(Vsub *sub);

// error types
#define VSUB_SUCCESS 0
#define VSUB_INVALID_SYNTAX 1
#define VSUB_VAR_ERROR 2
#define VSUB_PARSER_ERROR 3
#define VSUB_MEMORY_ERROR 4


#endif  // VSUB_H
