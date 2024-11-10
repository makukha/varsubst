#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"


#define printf_error(...) fprintf(stderr, __VA_ARGS__)

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
    vsub_init(&sub);
    FILE *fp = stdin;

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
                    printf_error("multiple paths not allowed\n");
                    result = false;
                    goto done;
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            // errors
            case '?':
                printf_error("unrecognized option '%s'\n", argv[optind - 1]);
                result = false;
                goto done;
            default:
                printf_error("unexpected getopt character code 0%o\n", o);
                result = false;
                goto done;
        }
    }
    // syntax
    if ((sub.syntax = vsub_syntax_lookup(use_syntax)) == NULL) {
        printf_error("unsupported syntax %s\n", use_syntax);
        result = false;
        goto done;
    }
    // input
    if (!path) {
        fp = stdin;
    }
    else {
        if (!(fp = fopen(path, "r"))) {
            printf_error("failed to open file %s\n", path);
            result = false;
            goto done;
        }
    }
    if (!vsub_use_text_from_file(&sub, fp)) {
        printf_error("out of memory\n");
        result = false;
        goto done;
    }
    // vars
    if (use_env) {
        if (!vsub_add_vars_from_env(&sub)) {
            printf_error("out of memory\n");
            result = false;
            goto done;
        }
    }
    // output
    int outfmt;
    if ((outfmt = vsub_format_lookup(use_format)) == -1) {
        printf_error("unsupported format %s\n", use_format);
        result = false;
        goto done;
    }
    // color
    use_color = (use_detailed && !no_color) ? true : false;

    // run
    if (!vsub_alloc(&sub)) {
        result = false;
        goto done;
    }
    if (!vsub_run(&sub)) {
        result = false;
        goto done;
    }

    // print result
    switch (outfmt) {
        case VSUB_FMT_PLAIN:
            if (vsub_fputs_plain(&sub, stdout, result, use_color, use_detailed) == EOF) {
                printf_error("failed to show results\n");  // todo: add granularity: file/memory error
                result = false;
                goto done;
            }
            break;
        case VSUB_FMT_JSON:
            if (vsub_fputs_json(&sub, stdout, use_detailed) == EOF) {
                printf_error("failed to show results\n");  // todo: add granularity: file/memory error
                result = false;
                goto done;
            }
            break;
        default:
            printf_error("unsupported output format %d\n", outfmt);
            result = false;
            goto done;
    }
//    vsub_print_error_sub(&sub, use_color); // todo: this should be closer to output
//    vsub_print_error_str(err, use_color); // todo: this should be closer to output

done:
    vsub_free(&sub);
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
