#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"


#define printf_error(...) { fprintf(stderr, __VA_ARGS__); fputs("\n", stderr); }

static void print_version() {
    puts("vsub " VSUB_VERSION);
}

static void print_formats() {
    for (size_t i = 0; i < VSUB_FORMAT_COUNT; i++) {
        puts(VSUB_FORMAT[i]);
    }
}

static void print_syntaxes() {
    // measure name column width
    int namew = 0;
    for (size_t i = 0; i < VSUB_SYNTAXES_COUNT; i++) {
        int w = strlen(VSUB_SYNTAXES[i].name);
        namew = (w > namew) ? w : namew;
    }
    // print
    char *fmttpl = "%%-%ds  %%s\n";
    char fmt[sizeof(fmttpl) + 11];  // 10 bytes for %d and 1 byte for '\0'
    snprintf(fmt, sizeof(fmt), fmttpl, namew);
    for (size_t i = 0; i < VSUB_SYNTAXES_COUNT; i++) {
        printf(fmt, VSUB_SYNTAXES[i].name, VSUB_SYNTAXES[i].title);
    }
}

static void print_usage() {
    puts("usage: vsub [options] [path]");
    puts("  options:");
    puts("    -s, --syntax=STR  set syntax to use; default: 'default'");
    puts("    -e, --env         use environment variables");
    puts("    -E, --envsubst    same as '--env --syntax=ggenv'");
    puts("    -f, --format=STR  set output format; default: 'plain'");
    puts("    -d, --detailed    add extended details");
    puts("    -b, --no-color    turn off color in --detailed mode");
    puts("        --formats     list supported output formats");
    puts("        --syntaxes    list supported syntaxes");
    puts("        --version     show tool name, version, and libvsub version");
    puts("    -h, --help        show this help and exit");
}

#define VSUB_OPT_VERSION 1000
#define VSUB_OPT_FORMATS 1001
#define VSUB_OPT_SYNTAXES 1002

static const char *shortopts = "-hbdeEf:s:";
static struct option longopts[] = {
    {"detailed", no_argument, 0, 'd'},
    {"env", no_argument, 0, 'e'},
    {"envsubst", no_argument, 0, 'E'},
    {"no-color", no_argument, 0, 'b'},
    {"format", required_argument, 0, 'f'},
    {"formats", no_argument, 0, VSUB_OPT_FORMATS},
    {"syntax", required_argument, 0, 's'},
    {"syntaxes", no_argument, 0, VSUB_OPT_SYNTAXES},
    // standard
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, VSUB_OPT_VERSION},
};

int main(int argc, char *argv[]) {
    // control flow
    bool result = true;
    // options
    bool use_color = false;  // unless --detailed and not --no-color
    bool no_color = false;   // --no-color default
    bool use_detailed = false;
    bool use_env = false;
    char *use_format = "plain";
    char *use_syntax = "default";
    char *path = NULL;
    int pathind = 0;
    // parser
    Vsub sub;
    FILE *fp = stdin;

    // --- parse options

    opterr = 0;
    int o;
    while ((o = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (o) {
            // standard
            case 'h':
                print_usage();
                goto done;
            case VSUB_OPT_VERSION:
                print_version();
                goto done;
            // specific
            case 'b':
                no_color = true;
                break;
            case 'd':
                use_detailed = true;
                break;
            case 'e':
                use_env = true;
                break;
            case 'E':
                use_env = true;
                use_syntax = "ggenv";
                break;
            case 'f':
                use_format = optarg;
                break;
            case VSUB_OPT_FORMATS:
                print_formats();
                goto done;
            case VSUB_OPT_SYNTAXES:
                print_syntaxes();
                goto done;
            case 's':
                use_syntax = optarg;
                break;
            // positional
            case 1:
                if (pathind > 0) {
                    printf_error("multiple paths not allowed");
                    result = false;
                    goto done;
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            // errors
            case '?':
                printf_error("invalid option: %s", argv[optind - 1]);
                result = false;
                goto done;
            default:
                printf_error("unexpected getopt code: 0%o", o);  // non-reproducible guard
                result = false;
                goto done;
        }
    }

    // --- initialize context

    if (!vsub_init(&sub)) {
        printf_error(VSUB_ERRORS[-VSUB_ERR_MEMORY]);
        result = false;
        goto done;
    }

    // --- set context parameters

    // syntax
    if ((sub.syntax = vsub_FindSyntax(use_syntax)) == NULL) {
        printf_error("unsupported syntax: %s", use_syntax);
        result = false;
        goto done;
    }
    // input
    if (path) {
        if (!(fp = fopen(path, "r"))) {
            printf_error("%s: %s", VSUB_ERRORS[-VSUB_ERR_FILE_OPEN], path);
            result = false;
            goto done;
        }
    }
    if (!vsub_UseTextFromFile(&sub, fp)) {
        printf_error(VSUB_ERRORS[-VSUB_ERR_MEMORY]);
        result = false;
        goto done;
    }
    // vars
    if (use_env) {
        if (!vsub_UseVarsFromEnv(&sub)) {
            printf_error(VSUB_ERRORS[-VSUB_ERR_MEMORY]);
            result = false;
            goto done;
        }
    }
    // output
    int outfmt;
    if ((outfmt = vsub_FindFormat(use_format)) == -1) {
        printf_error("unsupported output format: %s", use_format);  // non-reproducible guard
        result = false;
        goto done;
    }

    // --- set other options

    use_color = (use_detailed && !no_color) ? true : false;

    // --- process

    if (!vsub_alloc(&sub)) {
        result = false;
        goto done;
    }
    if (!vsub_run(&sub)) {
        result = false;
        goto done;
    }

    // --- output result

    switch (outfmt) {
        case VSUB_FMT_PLAIN:
            if (vsub_OutputPlain(&sub, stdout, result, use_color, use_detailed) == EOF) {
                printf_error("failed to show results");  // todo: add granularity: file/memory error
                result = false;
                goto processing_failed;
            }
            break;
        case VSUB_FMT_JSON:
            if (vsub_OutputJson(&sub, stdout, use_detailed) == EOF) {
                printf_error("failed to show results");  // todo: add granularity: file/memory error
                result = false;
                goto processing_failed;
            }
            break;
        default:
            printf_error("unsupported output format code: %d", outfmt);
            result = false;
            goto processing_failed;
    }

processing_failed:

    // --- report processing error

    switch (sub.err) {
        case VSUB_SUCCESS:
            break;  // no proccesing errors
        case VSUB_ERR_MEMORY:
            printf_error(VSUB_ERRORS[-VSUB_ERR_MEMORY]);
            break;
        case VSUB_ERR_SYNTAX:
            printf_error("%s: position %ld", VSUB_ERRORS[-VSUB_ERR_MEMORY], sub.inpc);
            break;
        case VSUB_ERR_VARIABLE:
            if (sub.errvar && sub.errmsg) {  // expected
                printf_error("%s: %s %s", VSUB_ERRORS[-VSUB_ERR_VARIABLE], sub.errvar, sub.errmsg);
            }
            else {  // non-reproducible guard
                printf_error(VSUB_ERRORS[-VSUB_ERR_VARIABLE]);
            }
            break;
        case VSUB_ERR_PARSER:
            printf_error("%s: position %ld", VSUB_ERRORS[-VSUB_ERR_PARSER], sub.inpc);
            break;
        default:
            printf_error(VSUB_ERRORS[-VSUB_ERR_UNKNOWN]);  // non-reproducible guard
            break;
    }

done:

    // --- finalize

    vsub_free(&sub);
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
