#include <stdio.h>
#include "vsub.h"


// error names

const char *VSUB_ERRORS[] = {
    "success",              // 0 = VSUB_SUCCESS
    "file read error",      // -1 = VSUB_ERR_FILE_READ
    "unable to open file",  // -2 = VSUB_ERR_FILE_OPEN
    "memory error",         // -3 = VSUB_ERR_MEMORY
    "invalid syntax",       // -4 = VSUB_ERR_SYNTAX
    "variable error",       // -5 = VSUB_ERR_VARIABLE
    "parser error",         // -6 = VSUB_ERR_PARSER
};


// error output

// todo: coloring should not be done here

#define VSUB_COLOR_ERROR "\033[31;1m"

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
        case VSUB_ERR_SYNTAX:
            fprintf(stderr, "%sinvalid input syntax on pos %ld%s\n", C, sub->inpc, R);
            break;
        case VSUB_ERR_VARIABLE:
            if (sub->errvar && sub->errmsg) {  // expected
                fprintf(stderr, "%svariable error: %s %s on pos %ld%s\n",
                    C, sub->errvar, sub->errmsg, sub->inpc, R);
            }
            else {  // fallback
                fprintf(stderr, "%svariable error on pos %ld%s\n", C, sub->inpc, R);
            }
            break;
        case VSUB_ERR_PARSER:
            fprintf(stderr, "%sunexpected parser error on pos %ld%s\n", C, sub->inpc, R);
            break;
        case VSUB_ERR_MEMORY:
            fprintf(stderr, "%sout of memory%s\n", C, R);
            break;
        default:
            fprintf(stderr, "%sunknown error%s\n", C, R);
            break;
    }
}
