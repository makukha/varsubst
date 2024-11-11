#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"
#include "util.h"


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
    unsigned namew = 0;
    for (size_t i = 0; i < VSUB_SYNTAXES_COUNT; i++) {
        unsigned w = strlen(VSUB_SYNTAXES[i].name);
        namew = (w > namew) ? w : namew;
    }
    // print
    const char *fmttpl = "%%-%ds  %%s\n";
    char fmt[sizeof(fmttpl) + 11];  // 10 bytes for %d and 1 byte for '\0'
    snprintf(fmt, sizeof(fmt), fmttpl, namew);
    for (size_t i = 0; i < VSUB_SYNTAXES_COUNT; i++) {
        printf(fmt, VSUB_SYNTAXES[i].name, VSUB_SYNTAXES[i].title);
    }
}

static void print_usage() {
    puts(
        "usage: vsub [options] [path]\n"
        "  options:\n"
        "    -e, --env         use environment variables\n"
        "    -E, --envsubst    same as '--env --syntax=ggenv'\n"
        "    -f, --format=STR  set output format; default: pretty if -d else plain\n"
        "    -d, --detailed    add extended details\n"
        "    -s, --syntax=STR  set syntax to use; default: 'default\n'"
        "    -v, --var=KEY=VAL set substitution variable; takes highest priority\n"
        "        --formats     list supported output formats\n"
        "        --syntaxes    list supported syntaxes\n"
        "        --version     show tool name and version\n"
        "    -h, --help        show this help and exit"
    );
}

#define VSUB_OPT_VERSION 1000
#define VSUB_OPT_FORMATS 1001
#define VSUB_OPT_SYNTAXES 1002

static const char *shortopts = "-hdeEf:s:v:";
static struct option longopts[] = {
    {"detailed", no_argument, 0, 'd'},
    {"env", no_argument, 0, 'e'},
    {"envsubst", no_argument, 0, 'E'},
    {"format", required_argument, 0, 'f'},
    {"formats", no_argument, 0, VSUB_OPT_FORMATS},
    {"syntax", required_argument, 0, 's'},
    {"syntaxes", no_argument, 0, VSUB_OPT_SYNTAXES},
    {"var", required_argument, 0, 'v'},
    // standard
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, VSUB_OPT_VERSION},
};

int main(int argc, char *argv[]) {
    // control flow
    bool result = true;
    // options
    bool use_detailed = false;
    bool use_env = false;
    char *use_format = NULL;
    char *use_syntax = "default";
    PtrArray vars;
    arr_init(&vars);
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
            case 's':
                use_syntax = optarg;
                break;
            case 'v':
                if (!arr_append(&vars, optarg)) {
                    printf_error(vsub_ErrMsg(MEMORY));
                    result = false;
                    goto done;
                }
                break;
            case VSUB_OPT_FORMATS:
                print_formats();
                goto done;
            case VSUB_OPT_SYNTAXES:
                print_syntaxes();
                goto done;
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
                printf_error("unexpected getopt code: %d", o);  // non-reproducible guard
                result = false;
                goto done;
        }
    }

    // --- initialize context

    if (!vsub_init(&sub)) {
        printf_error(vsub_ErrMsg(MEMORY));
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
            printf_error("%s: %s", vsub_ErrMsg(FILE_OPEN), path);
            result = false;
            goto done;
        }
    }
    if (!vsub_UseTextFromFile(&sub, fp)) {
        printf_error(vsub_ErrMsg(MEMORY));
        result = false;
        goto done;
    }

    // vars
    if (use_env) {
        if (!vsub_UseVarsFromEnv(&sub)) {
            printf_error(vsub_ErrMsg(MEMORY));
            result = false;
            goto done;
        }
    }
    if (vars.count) {
        if (!vsub_UseVarsFromKvarray(&sub, vars.count, (const char**)vars.items)) {
            printf_error(vsub_ErrMsg(MEMORY));
            result = false;
            goto done;
        }
    }

    // format
    int outfmt;
    if (!use_format) {
        outfmt = use_detailed ? VSUB_FMT_PRETTY : VSUB_FMT_PLAIN;
    }
    else {
        if ((outfmt = vsub_FindFormat(use_format)) == -1) {
            printf_error("unsupported output format: %s", use_format);  // non-reproducible guard
            result = false;
            goto done;
        }
    }

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

    int outres = 0;
    switch (outfmt) {
        case VSUB_FMT_PLAIN:
            outres = vsub_OutputPlain(&sub, stdout);
            break;
        case VSUB_FMT_JSON:
            outres = vsub_OutputJson(&sub, stdout, use_detailed);
            break;
        case VSUB_FMT_PRETTY:
            outres = vsub_OutputPretty(&sub, stdout, result, use_detailed);
            break;
        default:
            printf_error("unsupported output format code: %d", outfmt);  // non-reproducible guard
            result = false;
            goto processing_failed;
    }
    switch (outres) {
        case VSUB_SUCCESS:
            break;  // no processing errors
        case VSUB_ERR_MEMORY:
            printf_error(vsub_ErrMsg(MEMORY));
            result = false;
            goto done;
        case VSUB_ERR_OUTPUT:
            printf_error(vsub_ErrMsg(OUTPUT));
            result = false;
            goto done;
        default:
            printf_error("%s: %d", vsub_ErrMsg(UNKNOWN), outres);  // non-reproducible guard
            result = false;
            goto done;
      }

processing_failed:

    // --- report processing error

    switch (sub.err) {
        case VSUB_SUCCESS:
            break;  // no processing errors
        case VSUB_ERR_MEMORY:
            printf_error(vsub_ErrMsg(MEMORY));
            break;
        case VSUB_ERR_SYNTAX:
            printf_error("%s: position %ld", vsub_ErrMsg(MEMORY), sub.inpc);
            break;
        case VSUB_ERR_VARIABLE:
            if (sub.errvar && sub.errmsg) {
                // expected
                printf_error("%s: %s %s", vsub_ErrMsg(VARIABLE), sub.errvar, sub.errmsg);
            }
            else {
                // non-reproducible guard
                printf_error(vsub_ErrMsg(VARIABLE));
            }
            break;
        case VSUB_ERR_PARSER:
            printf_error("%s: position %ld", vsub_ErrMsg(PARSER), sub.inpc);
            break;
        default:
            printf_error("%s: %d", vsub_ErrMsg(UNKNOWN), sub.err);  // non-reproducible guard
            break;
    }

done:

    // --- finalize

    arr_free(&vars);
    vsub_free(&sub);
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
