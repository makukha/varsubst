#include <stdlib.h>
#include <string.h>
#include "vsub.h"

#include "syntax/default/parser.h"


#define PARSER(pfx) {\
    .create=(void *(*)(void *))pfx##_create,\
    .parse=(int (*)(void *, void *))pfx##_parse,\
    .destroy=(void (*)(void *))pfx##_destroy\
}


// syntaxes

const VsubSyntax VSUB_SYNTAX[] = {
    {.id=0, .name="default", .title="simple direct substitution"},
    {.id=1, .name="dc243", .title="Docker Compose v2.4.3"},
    {.id=2, .name="ggenv", .title="GNU gettext envsubst"},
};
const VsubParser VSUB_PARSER[] = {
    PARSER(vsub_sx_default),
    PARSER(vsub_sx_default),  // todo: replace with PARSER(vsub_sx_dc243)
    PARSER(vsub_sx_default),  // todo: replace with PARSER(vsub_sx_ggenv)
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

static int aux_getchar(Auxil *aux) {
    Vsub *sub = aux->sub;
    if (sub->maxinp == 0 || sub->getc < sub->maxinp) {
        int c = aux->tsrc->getchar(aux->tsrc);
        sub->getc++;
        return c;
    }
    else {
        sub->trunc = true;
        return -1;
    }
}

static const char *aux_getvalue(Auxil *aux, const char *var) {
    VsubVarsSrc *vsrc = aux->vsrc;
    while (vsrc) {
        const char *value = vsrc->getvalue(vsrc, var);
        if (value) {
            return value;
        }
        else {
            vsrc = vsrc->prev;
        }
    }
    return NULL;
}

static bool vsub_append(Vsub *sub, int epos, char *str) {
    sub->inpc = epos;
    size_t current = sub->resc;
    size_t required = sub->resc + strlen(str);
    size_t allowed = required;
    if (sub->maxres > 0 && sub->maxres < required) {
        allowed = sub->maxres;
    }
    int delta = allowed - current;
    if (allowed < required) {
        sub->trunc = true;
    }
    if (delta <= 0) {
        return false;
    }
    if (!aux_request_res_size(&(sub->aux), allowed + 1)) {
        return false;
    }
    char *end = stpncpy(sub->res + current, str, delta);
    *end = '\0';
    sub->resc += delta;
    return true;
}

static bool aux_append_orig(Auxil *aux, int epos, char *str) {
    return vsub_append(aux->sub, epos, str);
}

static bool aux_append_subst(Auxil *aux, int epos, char *str) {
    if (!vsub_append(aux->sub, epos, str)) {
        return false;
    }
    ((Vsub *)(aux->sub))->substc++;
    return true;
}

static bool aux_append_error(Auxil *aux, int epos, char *var, char *msg) {
    Vsub *sub = aux->sub;
    sub->inpc = epos;
    if (!aux_request_err_size(aux, strlen(var) + strlen(msg) + 2)) {
        return false;
    }
    sub->err = VSUB_VAR_ERROR;
    sub->errmsg = stpcpy(sub->errvar, var) + 1;  // skip term zero
    strcpy(sub->errmsg, msg);
    return true;
}


// vsub management

static void vsub_prepare_to_run(Vsub *sub) {
    if (sub->res != NULL) {     // if allocated, set to empty string
        *(sub->res) = '\0';
    }
    sub->err = VSUB_SUCCESS;
    if (sub->errvar != NULL) {  // if allocated, set to empty string
        *(sub->errvar) = '\0';
    }
    sub->errmsg = sub->errvar;  // set to empty string or NULL
    sub->trunc = false;
    sub->getc = 0;
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
    sub->res = NULL;     // tell vsub_prepare_to_run() that it is unallocated
    sub->errvar = NULL;
    vsub_prepare_to_run(sub);

    // aux
    sub->aux.sub = sub;
    // aux syntax methods
    sub->aux.getchar = (int (*)(void *))aux_getchar;
    sub->aux.getvalue = (const char *(*)(void *, const char *))aux_getvalue;
    sub->aux.append_orig = (bool (*)(void *, int, const char *))aux_append_orig;
    sub->aux.append_subst = (bool (*)(void *, int, const char *))aux_append_subst;
    sub->aux.append_error = (bool (*)(void *, int, const char *, const char *))aux_append_error;
    // data
    sub->aux.tsrc = NULL;
    sub->aux.vsrc = NULL;
    sub->aux.resz = VSUB_BRES_MIN;
    sub->aux.errz = VSUB_BERR_MIN;
    // parser
    sub->aux.parser = NULL;
    sub->aux.pctx = NULL;
}

bool vsub_alloc(Vsub *sub) {
    // result and error buffers
    if (!sub->res) {
        if (!(sub->res = malloc(sub->aux.resz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
    }
    if (!sub->errvar) {
        if (!(sub->errvar = malloc(sub->aux.errz))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
        // let errvar and errmsg be the same empty string
        sub->errvar[0] = '\0';
        sub->errmsg = sub->errvar;
    }
    // parser
    sub->aux.parser = &VSUB_PARSER[sub->syntax->id];
    if (!sub->aux.pctx) {
        if (!(sub->aux.pctx = sub->aux.parser->create(&(sub->aux)))) {
            sub->err = VSUB_MEMORY_ERROR;
            return false;
        }
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
    // parser context
    sub->aux.parser->destroy(sub->aux.pctx);
}

bool vsub_run(Vsub *sub) {
    vsub_prepare_to_run(sub);
    // first pass
    int ret = sub->aux.parser->parse(sub->aux.pctx, NULL);
    if (sub->err != VSUB_SUCCESS) {  // failed
        return false;
    }
    else if (ret == 0) {  // all consumed
        return true;
    }
    // second pass
    int resc = sub->resc;
    ret = sub->aux.parser->parse(sub->aux.pctx, NULL);  // second pass
    if (ret > 0 || resc < sub->resc || sub->err != VSUB_SUCCESS) {
        sub->err = VSUB_PARSER_ERROR;
        return false;
    }
    return true;
}
