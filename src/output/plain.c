#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include "../vsub.h"
#include "../util.h"


// styles

#define ST_TITLE "\033[30;1;106m"  // title
#define ST_SUCCESS "\033[92;1m"    // success
#define ST_ERROR "\033[31;1m"      // error
#define ST_NAME "\033[36m"         // metric name
#define ST_DESC "\033[2m"          // metric description
#define ST_VALUE "\033[1m"         // metric value
#define ST_RESULT "\033[96;1m"     // metric value - result
#define ST_HINT "\033[2m"          // metric note
#define ST_NULL "\033[36;2m"       // metric null value
#define R "\033[0m"                // reset

#define C(s) use_color ? s : ""

#define fputs_title(fp) {\
    fprintf(fp,\
        use_color\
        ? "%s vsub " VSUB_VERSION " %s\n"\
        : "%svsub " VSUB_VERSION "%s\n",\
        C(ST_TITLE), C(R)\
    );\
}
#define fprintf_result(fp, fmt, ...) {\
    fputs(C(ST_RESULT), fp);\
    fprintf(fp, fmt, __VA_ARGS__);\
    fputs(C(R), fp);\
}
#define fputs_null(fp) {\
    fprintf(fp, "%snull%s", C(ST_NULL), C(R));\
}
#define fprintf_value(fp, fmt, ...) {\
    fputs(C(ST_VALUE), fp);\
    fprintf(fp, fmt, __VA_ARGS__);\
    fputs(C(R), fp);\
}
#define fprintf_hint(fp, fmt, ...) {\
    fputs(C(ST_HINT), fp);\
    fprintf(fp, fmt, __VA_ARGS__);\
    fputs(C(R), fp);\
}



// todo: this function is not that useful
#include <stdarg.h>

int fprintfcerr(FILE *fp, bool use_color, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret;

    if ((ret = fputs(use_color ? ST_ERROR : "", fp) == EOF)) {
        goto done;
    };
    if ((ret = vfprintf(fp, format, args)) == EOF) {
        goto done;
    }
    if ((ret = fputs(use_color ? "\033[0m" : "", fp)) == EOF) {
        goto done;
    }
done:
    return ret;
}





// output

int vsub_fputs_plain(Vsub *sub, FILE *fp, bool success, bool use_color, bool detailed) {
    if (!sub->res) {
        return EOF;
    }
    if (!detailed) {
        return fprintf(fp, "%s\n", sub->res);
    }

    // detailed

    int ret = 0;
    char *rfmt = NULL;
    cJSON *root = NULL;
    if (!(root = vsub_results(sub, detailed))) {
        ret = EOF;
        goto done;
    }

    // determine header col lengths
    int namew = 0, descw = 0;
    for(cJSON *item = root->child; item != NULL; item = item->next) {
        int w;
        w = strlen(item->string);
        namew = (w > namew) ? w : namew;
        cJSON *desc = cJSON_GetObjectItemCaseSensitive(item, "desc");
        w = strlen(desc->valuestring);
        descw = (w > descw) ? w : descw;
    }

    // print metrics

    fputs_title(fp);

    if (!(rfmt = asprintf("%s%%-%ds%s  %s%%-%ds%s  ", C(ST_NAME), namew, C(R), C(ST_DESC), descw, C(R)))) {
        fprintfcerr(fp, use_color, "out of memory");
        goto done;
    }

    for(cJSON *item = root->child; item != NULL; item = item->next) {
        char *name = item->string;
        cJSON *desc = cJSON_GetObjectItemCaseSensitive(item, "desc");
        cJSON *val = cJSON_GetObjectItemCaseSensitive(item, "value");
        cJSON *hint = cJSON_GetObjectItemCaseSensitive(item, "hint");

        // row header
        fprintf(fp, rfmt, name, desc->valuestring);
        // value
        if (cJSON_IsNull(val)) {
            fputs_null(fp);
        }
        else if (cJSON_IsFalse(val)) {
            fprintf_value(fp, "%s", "false");
        }
        else if (cJSON_IsTrue(val)) {
            fprintf_value(fp, "%s", "true");
        }
        else if (cJSON_IsNumber(val)) {
            if (val->valueint == val->valuedouble) {
                fprintf_value(fp, "%d", val->valueint);
            }
            else {
                fprintf_value(fp, "%f", val->valuedouble);
            }
        }
        else if (cJSON_IsString(val)) {
            fprintf_value(fp, "%s", val->valuestring);
        }
        else if (cJSON_IsArray(val)) {
            bool first = true;
            for (cJSON *item = val->child; item != NULL; item = item->next) {
                fprintf_value(fp, "%s%s", (first ? "" : ", "), item->valuestring);
                first = false;
            }
        }
        else {
            fprintfcerr(fp, use_color, "unknown cJSON object type %d", val->type);
            ret = EOF;
            goto done;
        }
        // hint
        if (hint) {
            fprintf_hint(fp, " - %s", hint->valuestring);
        }
        // eol
        fputs("\n", fp);
    }

    // print result status

    if (success) {
        fprintf(fp, "%sSucceeded!%s\n", C(ST_SUCCESS), C(R));
    }
    else {
        fprintf(fp, "%sFailed.%s\n", C(ST_ERROR), C(R));
    }

done:
    cJSON_Delete(root);
    free(rfmt);
    return ret;
}
