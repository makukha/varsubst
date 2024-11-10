#include <stdlib.h>
#include <string.h>
#include "aux.h"
#include "vsub.h"
#include "vsubio.h"
#include "syntax/default/parser.h"


// syntaxes

#define PARSER(pfx) {\
    .create=(void *(*)(void *))pfx##_create,\
    .parse=(int (*)(void *, void *))pfx##_parse,\
    .destroy=(void (*)(void *))pfx##_destroy\
}

const VsubSyntax VSUB_SYNTAXES[] = {
    {0, "default", "simple direct substitution"},
//    {1, "dc243",   "Docker Compose v2.4.3"},
//    {2, "ggenv",   "GNU gettext envsubst"},
};
const VsubParser VSUB_PARSERS[] = {
    PARSER(vsub_sx_default),
    PARSER(vsub_sx_default),  // todo: replace with PARSER(vsub_sx_dc243)
    PARSER(vsub_sx_default),  // todo: replace with PARSER(vsub_sx_ggenv)
};
const size_t VSUB_SYNTAXES_COUNT = sizeof(VSUB_SYNTAXES) / sizeof(VSUB_SYNTAXES[0]);

const VsubSyntax *vsub_FindSyntax(const char *name) {
    for (size_t i = 0; i < VSUB_SYNTAXES_COUNT; i++) {
        if (strcmp(name, VSUB_SYNTAXES[i].name) == 0) {
            return &VSUB_SYNTAXES[i];
        }
    }
    return NULL;
}


// output formats

const char *VSUB_FORMAT[] = {"plain", "json"};
const size_t VSUB_FORMAT_COUNT = sizeof(VSUB_FORMAT) / sizeof(VSUB_FORMAT[0]);

int vsub_FindFormat(const char *name) {
    for (int i = 0; i < VSUB_FORMAT_COUNT; i++) {
        if (strcmp(name, VSUB_FORMAT[i]) == 0) {
            return i;
        }
    }
    return -1;
}


// memory management

static bool aux_request_resbuf(Auxil *aux, size_t sz) {
    if (sz <= aux->resz) {
        return true;
    }
    size_t newsz = sz + VSUB_BRES_INC;
    char *newbuf = realloc(aux->resbuf, sz);
    if (!newbuf) {
        aux->sub->err = VSUB_ERROR_MEMORY;
        return false;
    }
    aux->resbuf = newbuf;
    aux->resz = newsz;
    return true;
}

static bool aux_request_errbuf(Auxil *aux, size_t sz) {
    if (sz <= aux->errz) {
        return true;
    }
    char *newbuf = realloc(aux->errbuf, sz);
    if (!newbuf) {
        aux->sub->err = VSUB_ERROR_MEMORY;
        return false;
    }
    aux->errbuf = newbuf;
    aux->errz = sz;
    return true;
}


// aux syntax api

static int aux_getchar(Auxil *aux) {
    aux->sub->gcac++;
    if (aux->sub->maxinp == 0 || aux->sub->gcac < aux->sub->maxinp) {
        int c = ((VsubTextSrc*)(aux->sub->tsrc))->getchar(aux->sub->tsrc);
        if (c >= 0) {
            aux->sub->gcbc++;
        }
        return c;
    }
    else {
        aux->sub->trunc = true;
        return -1;
    }
}

static const char *aux_getvalue(Auxil *aux, const char *var) {
    VsubVarsSrc *vsrc = aux->sub->vsrc;
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

static bool aux_append(Auxil *aux, int epos, char *str) {
    Vsub *sub = aux->sub;
    sub->res = aux->resbuf;  // make non-NULL on first append
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
    if (!aux_request_resbuf(sub->aux, allowed + 1)) {
        return false;
    }
    char *end = stpncpy(sub->res + current, str, delta);
    *end = '\0';
    sub->resc += delta;
    return true;
}

static bool aux_append_orig(Auxil *aux, int epos, char *str) {
    return aux_append(aux, epos, str);
}

static bool aux_append_subst(Auxil *aux, int epos, char *str) {
    if (!aux_append(aux, epos, str)) {
        return false;
    }
    aux->sub->subc++;
    return true;
}

static bool aux_append_error(Auxil *aux, int epos, char *var, char *msg) {
    aux->sub->errvar = aux->errbuf;  // make non-NULL when error is set
    aux->sub->inpc = epos;
    if (!aux_request_errbuf(aux, strlen(var) + strlen(msg) + 2)) {
        return false;
    }
    aux->sub->err = VSUB_ERROR_VARIABLE;
    aux->sub->errmsg = stpcpy(aux->sub->errvar, var) + 1;  // skip term zero
    strcpy(aux->sub->errmsg, msg);
    return true;
}


// vsub management

static void vsub_clear_results(Vsub *sub) {
    sub->res = NULL;
    sub->err = VSUB_SUCCESS;
    sub->errvar = NULL;
    sub->errmsg = NULL;
    sub->trunc = false;
    sub->gcac = 0;
    sub->gcbc = 0;
    sub->inpc = 0;
    sub->resc = 0;
    sub->subc = 0;
    sub->iterc = 0;
}

bool vsub_init(Vsub *sub) {
    // vsub params
    sub->syntax = &VSUB_SYNTAXES[VSUB_SX_DEFAULT];
    sub->depth = 1;
    sub->maxinp = 0;
    sub->maxres = 0;
	// sources
    sub->tsrc = NULL;
    sub->vsrc = NULL;
    // vsub result
    vsub_clear_results(sub);

    // aux
    Auxil *aux = malloc(sizeof(Auxil));
    if (!aux) {
        return false;
    }
    aux->sub = sub;
    sub->aux = aux;
    // aux syntax methods
    aux->getchar = (int (*)(void *))aux_getchar;
    aux->getvalue = (const char *(*)(void *, const char *))aux_getvalue;
    aux->append_orig = (bool (*)(void *, int, const char *))aux_append_orig;
    aux->append_subst = (bool (*)(void *, int, const char *))aux_append_subst;
    aux->append_error = (bool (*)(void *, int, const char *, const char *))aux_append_error;
    // data
    aux->resbuf = NULL;
    aux->resz = VSUB_BRES_MIN;
    aux->errbuf = NULL;
    aux->errz = VSUB_BERR_MIN;
    // parser
    aux->parser = NULL;
    aux->pctx = NULL;

    return true;
}

bool vsub_alloc(Vsub *sub) {
    // result and error buffers
    Auxil *aux = sub->aux;
    if (!aux->resbuf) {
        if (!(aux->resbuf = malloc(aux->resz))) {
            sub->err = VSUB_ERROR_MEMORY;
            return false;
        }
    }
    if (!aux->errbuf) {
        if (!(aux->errbuf = malloc(aux->errz))) {
            sub->err = VSUB_ERROR_MEMORY;
            return false;
        }
    }
    // parser
    aux->parser = &VSUB_PARSERS[sub->syntax->id];
    if (!aux->pctx) {
        if (!(aux->pctx = aux->parser->create(aux))) {
            sub->err = VSUB_ERROR_MEMORY;
            return false;
        }
    }
    return true;
}

void vsub_free(Vsub *sub) {
    Auxil *aux = sub->aux;
    if (aux) {
        // parser context
        if (aux->parser) {
            aux->parser->destroy(aux->pctx);
        }
        // aux
        free(aux->resbuf);
        sub->res = NULL;
        free(aux->errbuf);
        sub->errvar = sub->errmsg = NULL;
        free(aux);
        sub->aux = NULL;
    }
    // input text source
    free(sub->tsrc);
    sub->tsrc = NULL;
    // input vars sources
    VsubVarsSrc *vsrc = sub->vsrc;
    while (vsrc != NULL) {
        VsubVarsSrc *prev = vsrc->prev;
        free(vsrc);
        vsrc = prev;
    }
    sub->vsrc = NULL;
}

bool vsub_run(Vsub *sub) {
    Auxil *aux = sub->aux;
    vsub_clear_results(sub);
    // first pass
    int ret = aux->parser->parse(aux->pctx, NULL);
    if (sub->err != VSUB_SUCCESS) {  // failed
        return false;
    }
    else if (ret == 0) {  // all consumed
        return true;
    }
    // second pass
    int resc = sub->resc;
    ret = aux->parser->parse(aux->pctx, NULL);  // second pass
    if (ret > 0 || resc < sub->resc || sub->err != VSUB_SUCCESS) {
        sub->err = VSUB_ERROR_PARSER;
        return false;
    }
    return true;
}
