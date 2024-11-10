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


// --- syntaxes

#define VSUB_SX_DEFAULT 0
#define VSUB_SX_DC243 1
#define VSUB_SX_GGENV 2

typedef struct VsubSyntax {
    const int id;
    const char *name;
    const char *title;
} VsubSyntax;

extern const VsubSyntax VSUB_SYNTAXES[];  // using VSUB_SX_* as indexes
extern const size_t VSUB_SYNTAXES_COUNT;

VSUB_EXPORT const VsubSyntax *vsub_FindSyntax(const char *name);  // find by name


// --- substitution context

typedef struct Vsub {
    // params
    const VsubSyntax *syntax;  // one of VSUB_SX_*; 0 by default
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
    void *tsrc;
    void *vsrc;
    void *aux;
} Vsub;

VSUB_EXPORT bool vsub_init(Vsub *sub);
VSUB_EXPORT bool vsub_alloc(Vsub *sub);
VSUB_EXPORT bool vsub_run(Vsub *sub);
VSUB_EXPORT cJSON *vsub_results(const Vsub *sub, bool include_details);
VSUB_EXPORT void vsub_free(Vsub *sub);


// --- input sources

VSUB_EXPORT bool vsub_UseTextFromFile(Vsub *sub, FILE *fp);
VSUB_EXPORT bool vsub_UseTextFromStr(Vsub *sub, const char *s);

VSUB_EXPORT bool vsub_UseVarsFromArgList(Vsub *sub, int c, const char *kv[]);
VSUB_EXPORT bool vsub_UseVarsFromArrays(Vsub *sub, int c, const char *k[], const char *v[]);
VSUB_EXPORT bool vsub_UseVarsFromEnv(Vsub *sub);


// --- output formats

#define VSUB_FMT_PLAIN 0
#define VSUB_FMT_JSON 1

VSUB_EXPORT int vsub_FindFormat(const char *name);
VSUB_EXPORT int vsub_fputs_plain(Vsub *sub, FILE *fp, bool result, bool use_color, bool detailed);
VSUB_EXPORT int vsub_fputs_json(Vsub *sub, FILE *fp, bool detailed);

extern const char *VSUB_FORMAT[];  // using VSUB_FMT_* above as indexes
extern const size_t VSUB_FORMAT_COUNT;


// --- error handling

#define VSUB_SUCCESS 0
#define VSUB_ERROR_SYNTAX 1
#define VSUB_ERROR_VARIABLE 2
#define VSUB_ERROR_PARSER 3
#define VSUB_ERROR_MEMORY 4

typedef struct VsubError {
    char id;
    const char *name;
} VsubError;

extern const VsubError VSUB_ERRORS[];  // using VSUB_* above as indexes


#endif  // VSUB_H
