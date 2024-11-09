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

#include <cjson/cJSON.h>


// --- Internals ---

// parsers
typedef struct VsubParser {
    void *(*create)(void *aux);
    int (*parse)(void *ctx, void *ret);
    void (*destroy)(void *ctx);
} VsubParser;
extern const VsubParser VSUB_PARSER[];  // using VSUB_SX_* as indexes

// input base
typedef struct VsubTextSrc {
    const char *name;
    int (*getchar)(void *src);
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
    int (*getchar)(void *aux);
    const char *(*getvalue)(void *aux, const char *var);
    bool (*append_orig)(void *aux, int epos, const char *str);
    bool (*append_subst)(void *aux, int epos, const char *str);
    bool (*append_error)(void *aux, int epos, const char *errvar, const char* errmsg);
    // data
    VsubTextSrc *tsrc;
    VsubVarsSrc *vsrc;
    char *resbuf;  // result buffer
    size_t resz;   // result buffer size
    char *errbuf;  // error buffer
    size_t errz;   // error buffer size
    // parser
    const VsubParser *parser;
    void *pctx;
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
#define VSUB_SX_DEFAULT 0
#define VSUB_SX_DC243 1
#define VSUB_SX_GGENV 2
typedef struct VsubSyntax {
    const int id;
    const char *name;
    const char *title;
} VsubSyntax;
extern const VsubSyntax VSUB_SYNTAX[];  // using VSUB_SX_* as indexes
extern const size_t VSUB_SYNTAX_COUNT;
VSUB_EXPORT const VsubSyntax *vsub_syntax_lookup(const char *name);  // find by name

// results and params
typedef struct Vsub {
    // params
    const VsubSyntax *syntax;  // syntax dialect, one of VSUB_SX_*; 0 by default
    char depth;     // max subst iter count; default: 1
    size_t maxinp;  // max length of input string; unlim if set to 0
    size_t maxres;  // max length of result string; unlim if set to 0
    // result
    char *res;      // result string; points to input if plain=1 and copyplain=0
    char err;       // see error/success flags
    char *errvar;   // first variable name with error; NULL by default
    char *errmsg;   // variable error message; NULL by default
    bool trunc;     // whether result string was truncated because of maxinp or maxres
    size_t gcac;    // input bytes requested
    size_t gcbc;    // input bytes returned other than EOF
    size_t inpc;    // parsed input length
    size_t resc;    // actual length of result str
    size_t subc;    // count of total substitutions made
    char iterc;     // count of subst iterations actually performed
    // internal data
    Auxil aux;
} Vsub;

VSUB_EXPORT void vsub_init(Vsub *sub);
VSUB_EXPORT bool vsub_alloc(Vsub *sub);
VSUB_EXPORT bool vsub_run(Vsub *sub);
VSUB_EXPORT cJSON *vsub_results(const Vsub *sub, bool include_details);
VSUB_EXPORT void vsub_free(Vsub *sub);
// input text
VSUB_EXPORT bool vsub_use_text_from_file(Vsub *sub, FILE *fp);
VSUB_EXPORT bool vsub_use_text_from_str(Vsub *sub, const char *s);
// input vars
VSUB_EXPORT bool vsub_add_vars_from_arglist(Vsub *sub, int c, const char *kv[]);
VSUB_EXPORT bool vsub_add_vars_from_arrays(Vsub *sub, int c, const char *k[], const char *v[]);
VSUB_EXPORT bool vsub_add_vars_from_env(Vsub *sub);

// output formats
#define VSUB_FMT_PLAIN 0
#define VSUB_FMT_JSON 1
extern const char *VSUB_FORMAT[];  // using VSUB_FMT_* above as indexes
extern const size_t VSUB_FORMAT_COUNT;
VSUB_EXPORT int vsub_format_lookup(const char *name);
VSUB_EXPORT int vsub_fputs_plain(Vsub *sub, FILE *fp, bool result, bool use_color, bool detailed);
VSUB_EXPORT int vsub_fputs_json(Vsub *sub, FILE *fp, bool detailed);

// errors
#define VSUB_SUCCESS 0
#define VSUB_INVALID_SYNTAX 1
#define VSUB_VAR_ERROR 2
#define VSUB_PARSER_ERROR 3
#define VSUB_MEMORY_ERROR 4
typedef struct VsubError {
    char id;
    const char *title;
} VsubError;
extern const VsubError VSUB_ERRORS[];  // using VSUB_* above as indexes
// error handling
#define VSUB_COLOR_ERROR "\033[31;1m"
VSUB_EXPORT void vsub_print_error_str(const char *str, bool use_color);
VSUB_EXPORT void vsub_print_error_sub(const Vsub *sub, bool use_color);

#endif  // VSUB_H
