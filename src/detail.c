#include <stdio.h>
#include <string.h>

#include <cjson/cJSON.h>
#include "vsub.h"


static bool vsub_add_result(cJSON *root, const Vsub *vsub);
static bool vsub_add_details(cJSON *root, const Vsub *vsub);

cJSON *vsub_results(const Vsub *sub, bool include_details) {
    if (!sub) {
        return NULL;
    }
    cJSON *root;
    if (!(root = cJSON_CreateObject())) {
        goto err;
    }
    if (!vsub_add_result(root, sub)) {
        goto err;
    }
    if (include_details) {
        if (!vsub_add_details(root, sub)) {
            goto err;
        }
    }
    return root;
err:
    cJSON_Delete(root);
    return NULL;
}


// helpers

#define CREATE_NODE(name, val) \
    cJSON *name = cJSON_Create##val; \
    if (!name) return false;

#define ADD_KEY(parent, name, val) \
    CREATE_NODE(name, val)\
    cJSON_AddItemToObject(parent, #name, name);

#define ADD_ITEM(parent, name, val) \
    CREATE_NODE(name, val)\
    cJSON_AddItemToArray(parent, name);

#define METRIC(metric_name, metric_desc, condition) \
    CREATE_NODE(metric, Object())\
    cJSON_AddItemToObject(root, metric_name, metric);\
    ADD_KEY(metric, desc, StringReference(metric_desc))\
    if (!condition) { ADD_KEY(metric, value, Null()) } else

#define require(cond) if (!cond) {  } else


// result and details

bool vsub_add_result(cJSON *root, const Vsub *sub) {
    {METRIC("res", "result string", sub->res) {
        ADD_KEY(metric, value, StringReference(sub->res));
    }}
    return true;
}

bool vsub_add_details(cJSON *root, const Vsub *sub) {

    // params

    {METRIC("syntax", "substitution syntax", sub->syntax) {
        ADD_KEY(metric, value, StringReference(sub->syntax->name));
        ADD_KEY(metric, hint, StringReference(sub->syntax->title));
    }}
    {METRIC("tsrc", "input text source", sub->aux.tsrc) {
        ADD_KEY(metric, value, StringReference(sub->aux.tsrc->name));
    }}
    {METRIC("vsrc", "input vars sources", sub->aux.vsrc) {
        ADD_KEY(metric, value, Array())
        for (VsubVarsSrc *vsrc = sub->aux.vsrc; vsrc != NULL; vsrc = vsrc->prev) {
            ADD_ITEM(value, item, StringReference(vsrc->name))
        }
    }}
    {METRIC("maxinp", "max input length", true) {
        ADD_KEY(metric, value, Number(sub->maxinp));
        if (sub->maxinp == 0) {
            ADD_KEY(metric, hint, String("unlimited"));
        }
    }}
    {METRIC("maxres", "max result length", true) {
        ADD_KEY(metric, value, Number(sub->maxres));
        if (sub->maxres == 0) {
            ADD_KEY(metric, hint, String("unlimited"));
        }
    }}
    {METRIC("depth", "max allowed depth", true) {
        ADD_KEY(metric, value, Number(sub->depth));
        ADD_KEY(metric, hint, String(
            (sub->depth == 0) ? "processing disabled" :
            (sub->depth == 1) ? "nesting disabled" :
            (sub->depth >= 2) ? "nesting enabled" :
            "invalid"
        ));
    }}

    // results
    {METRIC("trunc", "result was truncated", true) {
        ADD_KEY(metric, value, Bool(sub->trunc));
    }}
    {METRIC("gcac", "input chars requested", true) {
        ADD_KEY(metric, value, Number(sub->gcac));
    }}
    {METRIC("gcbc", "input chars returned", true) {
        ADD_KEY(metric, value, Number(sub->gcbc));
    }}
    {METRIC("inpc", "parsed char count", true) {
        ADD_KEY(metric, value, Number(sub->inpc));
    }}
    {METRIC("resc", "result char count", true) {
        ADD_KEY(metric, value, Number(sub->resc));
    }}
    {METRIC("subc", "substitutions count", true) {
        ADD_KEY(metric, value, Number(sub->subc));
        if (sub->subc == 0) {
            ADD_KEY(metric, hint, StringReference("input unchanged"));
        }
    }}
    {METRIC("iterc", "iterations count", true) {
        ADD_KEY(metric, value, Number(sub->iterc));
    }}
    {METRIC("err", "error code", true) {
        ADD_KEY(metric, value, Number(sub->err));
        ADD_KEY(metric, hint, StringReference(VSUB_ERRORS[(int)sub->err].name));
    }}
    {METRIC("errvar", "error var name", sub->errvar) {
        ADD_KEY(metric, value, StringReference(sub->errvar));
    }}
    {METRIC("errmsg", "error var msg", sub->errmsg) {
        ADD_KEY(metric, value, StringReference(sub->errmsg));
    }}

    return true;
}
