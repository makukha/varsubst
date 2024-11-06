#include <stdlib.h>
#include <string.h>
#include "vsub.h"


// syntaxes

const VsubSyntax VSUB_SYNTAX[] = {
    {.id=0, .name="default", .title="simple direct substitution"},
    {.id=1, .name="dc243", .title="Docker Compose v2.4.3"},
    {.id=2, .name="ggenv", .title="GNU gettext envsubst"},
};
const size_t VSUB_SYNTAX_COUNT = sizeof(VSUB_SYNTAX) / sizeof(VSUB_SYNTAX[0]);

VsubSyntax *vsub_find_syntax(char *name) {
    for (size_t i = 0; i < VSUB_SYNTAX_COUNT; i++) {
        if (strcmp(name, VSUB_SYNTAX[i].name) == 0) {
            return &VSUB_SYNTAX[i];
        }
    }
    return NULL;
}


// aux utils

static bool aux_request_bres_size(Auxil *aux, size_t sz) {
    Vsub *sub = aux->sub;
    if (sz <= aux->bresz) {
        return true;
    }
    size_t newsz = sz + VSUB_BRES_INC;
    if (!(aux->bres = realloc(aux->bres, sz))) {
        sub->err = VSUB_MEMORY_ERROR;
        return false;
    }
    aux->bresz = newsz;
    sub->res = aux->bres;
    return true;
}

static bool aux_request_berr_size(Auxil *aux, size_t sz) {
    Vsub *sub = aux->sub;
    if (sz <= aux->berrz) {
        return true;
    }
    if (!(aux->berr = realloc(aux->berr, sz))) {
        sub->err = VSUB_MEMORY_ERROR;
        return false;
    }
    aux->berrz = sz;
    return true;
}

void aux_set_tsrc(Auxil *aux, VsubTextSrc *src) {
    if (aux->tsrc) {
        free(aux->tsrc);
    }
    aux->tsrc = src;
}

void aux_add_vsrc(Auxil *aux, VsubVarsSrc *src) {
    src->prev = aux->vsrc;
    aux->vsrc = src;
}


// aux syntax api

static char aux_getchar(Auxil *aux) {
    Vsub *sub = aux->sub;
    if (sub->maxinp == 0 || sub->inpc < sub->maxinp) {
        int c = aux->tsrc->getchar(aux->tsrc);
        sub->inpc++;
        return c;
    }
    else {
        sub->trunc = true;
        return '\0';
    }
}

static const char *aux_getvalue(Auxil *aux, const char *val) {
    VsubVarsSrc *vsrc = aux->vsrc;
    while (vsrc) {
        const char *val = vsrc->getvalue(vsrc, val);
        if (val) {
            return val;
        }
        else {
            vsrc = vsrc->prev;
        }
    }
    return NULL;
}

static bool aux_append_orig(Auxil *aux, char* str) {  // todo
//    size_t len = strlen(str);
//    sub->resc += len;
    return true;
}

static bool aux_append_subst(Auxil *aux, char* str) {  // todo
//    size_t len = strlen(str);
//    sub->resc += len;
//    sub->substc++;
    return true;
}

static bool aux_append_error(Auxil *aux, char* var, char *msg) {
    Vsub *sub = aux->sub;
    size_t sz = strlen(var) + strlen(msg) + 2;
    if (!aux_request_berr_size(aux, sz)) {
        return false;
    }
    sub->err = VSUB_VAR_ERROR;
    sub->errvar = aux->berr;
    sub->errmsg = stpcpy(sub->errvar, var) + 1;
    strcpy(sub->errmsg, msg);
    return true;
}


// vsub

static void aux_init(Auxil *aux) {
    aux->sub = NULL;
    // syntax methods
    aux->getchar = (char (*)(void *))aux_getchar;
    aux->getvalue = (const char *(*)(void *, const char *))aux_getvalue;
    aux->append_orig = (bool (*)(void *, const char *))aux_append_orig;
    aux->append_subst = (bool (*)(void *, const char *))aux_append_subst;
    aux->append_error = (bool (*)(void *, const char *, const char *))aux_append_error;
    // data
    aux->tsrc = NULL;
    aux->vsrc = NULL;
    aux->bres = NULL;
    aux->bresz = VSUB_BRES_MIN;
    aux->berr = NULL;
    aux->berrz = VSUB_BERR_MIN;
}

static bool aux_alloc(Auxil *aux) {
    Vsub *sub = aux->sub;
    if (aux->bres == NULL) {
        if (!(aux->bres = malloc(aux->bresz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
        sub->res = aux->bres;
    }
    if (aux->berr == NULL) {
        if (!(aux->berr = malloc(aux->berrz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
    }
    return true;
}

static void aux_free(Auxil *aux) {
    Vsub *sub = aux->sub;
    free(aux->bres);
    free(aux->berr);
    sub->res = aux->bres = aux->berr = NULL;
    free(aux->tsrc);
    aux->tsrc = NULL;
    VsubVarsSrc *vsrc = aux->vsrc;
    while (vsrc != NULL) {
        VsubVarsSrc *prev = vsrc->prev;
        free(vsrc);
        vsrc = prev;
    }
    aux->vsrc = NULL;
}

static void vsub_clear(Vsub *sub) {
    if (sub->res != NULL) {
        *(sub->res) = '\0';
    }
    sub->err = VSUB_SUCCESS;
    sub->errvar = NULL;
    sub->errmsg = NULL;
    sub->trunc = false;
    sub->inpc = 0;
    sub->resc = 0;
    sub->substc = 0;
    sub->depthc = 0;
}

void vsub_init(Vsub *sub) {
    // params
    sub->syntax = &VSUB_SYNTAX[VSUB_SX_DEFAULT];
    sub->depth = 1;
    sub->maxinp = 0;
    sub->maxres = 0;
    // result
    sub->res = NULL;
    vsub_clear(sub);
    // aux
    aux_init(&(sub->aux));
    sub->aux.sub = sub;
}

static bool vsub_alloc(Vsub *sub) {
    if (!aux_alloc(&(sub->aux))) {
        return false;
    }
    return true;
}

void vsub_free(Vsub *sub) {
    aux_free(&(sub->aux));
}

bool vsub_run(Vsub *sub) {
    if (!vsub_alloc(sub)) {
        return false;
    }
    vsub_clear(sub);
    // todo: run
    return true;
}
