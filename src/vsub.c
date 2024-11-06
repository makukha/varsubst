#include <stdlib.h>
#include <string.h>
#include "vsub.h"


#define MIN(a,b) (((a)<(b))?(a):(b))


// syntaxes

const VsubSyntax VSUB_SYNTAX[] = {
    {.id=0, .name="default", .title="simple direct substitution"},
    {.id=1, .name="dc243", .title="Docker Compose v2.4.3"},
    {.id=2, .name="ggenv", .title="GNU gettext envsubst"},
};
const size_t VSUB_SYNTAX_COUNT = sizeof(VSUB_SYNTAX) / sizeof(VSUB_SYNTAX[0]);

const VsubSyntax *vsub_find_syntax(const char *name) {
    for (size_t i = 0; i < VSUB_SYNTAX_COUNT; i++) {
        if (strcmp(name, VSUB_SYNTAX[i].name) == 0) {
            return &VSUB_SYNTAX[i];
        }
    }
    return NULL;
}


// memory management

static bool aux_request_res_size(Auxil *aux, size_t sz) {
    Vsub *sub = aux->sub;
    if (sz <= aux->resz) {
        return true;
    }
    size_t newsz = sz + VSUB_BRES_INC;
    char *newbuf = realloc(sub->res, sz);
    if (!newbuf) {
        sub->err = VSUB_MEMORY_ERROR;
        return false;
    }
    aux->resz = newsz;
    sub->res = newbuf;
    return true;
}

static bool aux_request_err_size(Auxil *aux, size_t sz) {
    Vsub *sub = aux->sub;
    if (sz <= aux->errz) {
        return true;
    }
    char *newbuf = realloc(sub->errvar, sz);
    if (!newbuf) {
        sub->err = VSUB_MEMORY_ERROR;
        return false;
    }
    aux->errz = sz;
    sub->errvar = newbuf;
    return true;
}


// input sources management

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

//static bool aux_append(Auxil *aux, char *str) {
//    Vsub *sub = aux->sub;
//    size_t current = sub->resc;
//    size_t required = sub->resc + strlen(str);
//    size_t allowed = (sub->maxres == 0) ? required : MIN(sub->maxres, required);
//    if (allowed < required) {
//        sub->trunc = true;
//    }
//    if (allowed <= current) {
//        return false;
//    }
//    if (!aux_request_bres_size(aux, allowed + 1)) {
//        return false;
//    }
//
//}

static bool aux_append_orig(Auxil *aux, char *str) {  // todo
    Vsub *sub = aux->sub;
    size_t sz = sub->resc + strlen(str) + 1;

//    size_t len = strlen(str);
//    sub->resc += len;
    return true;
}

static bool aux_append_subst(Auxil *aux, char *str) {  // todo
//    size_t len = strlen(str);
//    sub->resc += len;
//    sub->substc++;
    return true;
}

static bool aux_append_error(Auxil *aux, char *var, char *msg) {
    Vsub *sub = aux->sub;
    if (!aux_request_err_size(aux, strlen(var) + strlen(msg) + 2)) {
        return false;
    }
    sub->err = VSUB_VAR_ERROR;
    sub->errmsg = stpcpy(sub->errvar, var) + 1;  // skip term zero
    strcpy(sub->errmsg, msg);
    return true;
}


// vsub management

static void vsub_clear(Vsub *sub) {
    if (sub->res != NULL) {     // if allocated, set to empty string
        *(sub->res) = '\0';
    }
    sub->err = VSUB_SUCCESS;
    if (sub->errvar != NULL) {  // if allocated, set to empty string
        *(sub->errvar) = '\0';
    }
    sub->errmsg = sub->errvar;  // set to empty string or NULL
    sub->trunc = false;
    sub->inpc = 0;
    sub->resc = 0;
    sub->substc = 0;
    sub->depthc = 0;
}

void vsub_init(Vsub *sub) {
    // vsub params
    sub->syntax = &VSUB_SYNTAX[VSUB_SX_DEFAULT];
    sub->depth = 1;
    sub->maxinp = 0;
    sub->maxres = 0;
    // vsub result
    sub->res = NULL;     // tell vsub_clear() that it is unallocated
    sub->errvar = NULL;
    vsub_clear(sub);

    // aux
    sub->aux.sub = sub;
    // aux syntax methods
    sub->aux.getchar = (char (*)(void *))aux_getchar;
    sub->aux.getvalue = (const char *(*)(void *, const char *))aux_getvalue;
    sub->aux.append_orig = (bool (*)(void *, const char *))aux_append_orig;
    sub->aux.append_subst = (bool (*)(void *, const char *))aux_append_subst;
    sub->aux.append_error = (bool (*)(void *, const char *, const char *))aux_append_error;
    // data
    sub->aux.tsrc = NULL;
    sub->aux.vsrc = NULL;
    sub->aux.resz = VSUB_BRES_MIN;
    sub->aux.errz = VSUB_BERR_MIN;
}

static bool vsub_alloc(Vsub *sub) {
    if (sub->res == NULL) {
        if (!(sub->res = malloc(sub->aux.resz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
    }
    if (sub->errvar == NULL) {
        if (!(sub->errvar = malloc(sub->aux.errz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
        // let errvar and errmsg be the same empty string
        sub->errvar[0] = '\0';
        sub->errmsg = sub->errvar;
    }
    return true;
}

void vsub_free(Vsub *sub) {
    // result and error buffers
    free(sub->res);
    free(sub->errvar);
    sub->res = sub->errvar = sub->errmsg = NULL;
    // input text source
    free(sub->aux.tsrc);
    sub->aux.tsrc = NULL;
    // input vars sources
    VsubVarsSrc *vsrc = sub->aux.vsrc;
    while (vsrc != NULL) {
        VsubVarsSrc *prev = vsrc->prev;
        free(vsrc);
        vsrc = prev;
    }
    sub->aux.vsrc = NULL;
}

bool vsub_run(Vsub *sub) {
    if (!vsub_alloc(sub)) {
        return false;
    }
    vsub_clear(sub);
    // todo: run
    return true;
}
