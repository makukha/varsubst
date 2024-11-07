#include <stdio.h>
#include <string.h>
#include "vsub.h"


// metrics

typedef struct Metric {
    const char *name;
    bool available_before_run;
    void (*print_value)(const Vsub *);
    void (*print_note)(const Vsub *);
    const char *desc;
} Metric;

#define PRINT(...) fprintf(stderr, __VA_ARGS__)

static void pv_syntax(const Vsub *sub)  { PRINT("%s", sub->syntax->name); }
static void pn_syntax(const Vsub *sub)  { PRINT("%s", sub->syntax->title); }
static void pv_tsrc(const Vsub *sub)    { PRINT("%s", sub->aux.tsrc->name); }
static void pv_vsrc(const Vsub *sub)    {
    char *sep = "";
    for (VsubVarsSrc *vsrc = sub->aux.vsrc; vsrc != NULL; vsrc = vsrc->prev) {
        PRINT("%s%s", sep, vsrc->name);
        sep = " ";
    }
}
static void pv_maxinp(const Vsub *sub)  { PRINT("%ld", sub->maxinp); }
static void pn_maxinp(const Vsub *sub)  { PRINT("%s", sub->maxinp == 0 ? "unlimited" : ""); }
static void pv_maxres(const Vsub *sub)  { PRINT("%ld", sub->maxres); }
static void pn_maxres(const Vsub *sub)  { PRINT("%s", sub->maxres == 0 ? "unlimited" : ""); }
static void pv_depth(const Vsub *sub)   { PRINT("%d", sub->depth); }
static void pv_res(const Vsub *sub)     { PRINT("%s", sub->res); }
static void pv_trunc(const Vsub *sub)   { PRINT("%s", sub->trunc ? "yes" : "no"); }
static void pv_gcac(const Vsub *sub)    { PRINT("%ld", sub->gcac); }
static void pv_gcbc(const Vsub *sub)    { PRINT("%ld", sub->gcbc); }
static void pv_inpc(const Vsub *sub)    { PRINT("%ld", sub->inpc); }
static void pv_resc(const Vsub *sub)    { PRINT("%ld", sub->resc); }
static void pv_substc(const Vsub *sub)  { PRINT("%ld", sub->substc); }
static void pn_substc(const Vsub *sub)  { PRINT("%s", sub->substc == 0 ? "input unchanged" : ""); }
static void pv_iterc(const Vsub *sub)   { PRINT("%d", sub->iterc); }
static void pv_err(const Vsub *sub)     { PRINT("%d", sub->err); }
static void pn_err(const Vsub *sub)     { PRINT("%s", VSUB_ERRORS[(int)sub->err].title); }
static void pv_errvar(const Vsub *sub)  { PRINT("%s", sub->errvar); }
static void pv_errmsg(const Vsub *sub)  { PRINT("%s", sub->errmsg); }

static const Metric METRICS[] = {
    // before run
    {"syntax", true,  pv_syntax, pn_syntax, "substitution syntax"},
    {"tsrc",   true,  pv_tsrc,   NULL,      "input text source"},
    {"vsrc",   true,  pv_vsrc,   NULL,      "input vars sources"},
    {"maxinp", true,  pv_maxinp, pn_maxinp, "max input length"},
    {"maxres", true,  pv_maxres, pn_maxres, "max result length"},
    {"depth",  true,  pv_depth,  NULL,      "max allowed depth"},
    // after run
    {"res",    false, pv_res,    NULL,      "result string"},
    {"trunc",  false, pv_trunc,  NULL,      "result was truncated"},
    {"gcac",   false, pv_gcac,   NULL,      "input chars requested"},
    {"gcbc",   false, pv_gcbc,   NULL,      "input chars returned"},
    {"inpc",   false, pv_inpc,   NULL,      "parsed char count"},
    {"resc",   false, pv_resc,   NULL,      "result char count"},
    {"substc", false, pv_substc, pn_substc, "substitutions count"},
    {"iterc",  false, pv_iterc,  NULL,      "iterations count"},
    {"err",    false, pv_err,    pn_err,    "error code"},
    {"errvar", false, pv_errvar, NULL,      "error var name"},
    {"errmsg", false, pv_errmsg, NULL,      "error var msg"},
};
static const size_t METRICS_COUNT = sizeof(METRICS) / sizeof(METRICS[0]);


// styles

#define ST_TITLE "\033[30;1;106m"  // title
#define ST_SUCCESS "\033[92;1m"    // success
#define ST_ERROR VSUB_COLOR_ERROR  // error
#define ST_NAME "\033[36m"         // metric name
#define ST_DESC "\033[2m"          // metric description
#define ST_VALUE "\033[1m"         // metric value
#define ST_RESULT "\033[96;1m"     // metric value - result
#define ST_NOTE "\033[2m"          // metric note
#define R "\033[0m"                // reset
#define C(s) use_color ? s : ""


// detail output

void vsub_print_detail_metrics(const Vsub *sub, bool before, bool use_color) {
    // determine col widths
    int namew = 0, descw = 0;
    for (int w, i = 0; i < METRICS_COUNT; i++) {
        w = strlen(METRICS[i].name);
        namew = (w > namew) ? w : namew;
        w = strlen(METRICS[i].desc);
        descw = (w > descw) ? w : descw;
    }
    // print metrics
    char fmt[64];
    snprintf(fmt, 64, "%s%%-%ds%s  %s%%-%ds%s  ", C(ST_NAME), namew, C(R), C(ST_DESC), descw, C(R));
    for (size_t i = 0; i < METRICS_COUNT; i++) {
        const Metric *m = &METRICS[i];
        if (m->available_before_run != before) {
            continue;
        }
        fprintf(stderr, fmt, m->name, m->desc);
        // value
        fputs((strcmp(m->name, "res") == 0) ? C(ST_RESULT) : C(ST_VALUE), stderr);
        m->print_value(sub);
        fputs(C(R), stderr);
        // note
        if (m->print_note) {
            fprintf(stderr, "  %s", C(ST_NOTE));
            m->print_note(sub);
            fputs(C(R), stderr);
        }
        fputs("\n", stderr);
    }
}

void vsub_print_detail_title(bool use_color) {
    char *fmt = use_color ? "%s vsub " VSUB_VERSION " %s\n" : "%svsub " VSUB_VERSION "%s\n";
    fprintf(stderr, fmt, C(ST_TITLE), C(R));
}

void vsub_print_detail_result_status(bool result, bool use_color) {
    if (result) {
        fprintf(stderr, "%sSucceeded!%s\n", C(ST_SUCCESS), C(R));
    }
    else {
        fprintf(stderr, "%sFailed.%s\n", C(ST_SUCCESS), C(R));
    }
}
