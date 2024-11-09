#include <stdio.h>
#include "vsub.h"


// error titles

const VsubError VSUB_ERRORS[] = {
    {VSUB_SUCCESS, "success"},
    {VSUB_INVALID_SYNTAX, "invalid syntax"},
    {VSUB_VAR_ERROR, "variable error"},
    {VSUB_PARSER_ERROR, "parser error"},
    {VSUB_MEMORY_ERROR, "memory error"},
};


// error output

void vsub_print_error_str(const char *str, bool use_color) {
    if (str[0] == '\0') {
        return;
    }
    char *C = use_color ? VSUB_COLOR_ERROR : "";
    char *R = use_color ? "\033[0m" : "";
    fprintf(stderr, "%s%s%s", C, str, R);
}

void vsub_print_error_sub(const Vsub *sub, bool use_color) {
    char *C = use_color ? VSUB_COLOR_ERROR : "";
    char *R = use_color ? "\033[0m" : "";
    switch (sub->err) {
        case VSUB_SUCCESS:
            break;  // no errors
        case VSUB_INVALID_SYNTAX:
            fprintf(stderr, "%sinvalid input syntax on pos %ld%s\n", C, sub->inpc, R);
        break;
        case VSUB_VAR_ERROR:
            if (sub->errvar && sub->errmsg) {  // expected
                fprintf(stderr, "%svariable error: %s %s on pos %ld%s\n",
                    C, sub->errvar, sub->errmsg, sub->inpc, R);
            }
            else {  // fallback
                fprintf(stderr, "%svariable error on pos %ld%s\n", C, sub->inpc, R);
            }
        break;
        case VSUB_PARSER_ERROR:
            fprintf(stderr, "%sunexpected parser error on pos %ld%s\n", C, sub->inpc, R);
        break;
        case VSUB_MEMORY_ERROR:
            fprintf(stderr, "%sout of memory%s\n", C, R);
        break;
        default:
            fprintf(stderr, "%sundefined error%s\n", C, R);
        break;
    }
}
