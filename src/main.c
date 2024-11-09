#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"


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
    int shortlen = 0;
    for (size_t i = 0; i < VSUB_SYNTAX_COUNT; i++) {
        int len = strlen(VSUB_SYNTAX[i].name);
        shortlen = (len > shortlen) ? len : shortlen;
    }
    // print
    char fmt[24];
    sprintf(fmt, "%%-%ds  %%s\n", shortlen);
    for (size_t i = 0; i < VSUB_SYNTAX_COUNT; i++) {
        printf(fmt, VSUB_SYNTAX[i].name, VSUB_SYNTAX[i].title);
    }
}

static void print_usage() {
    puts("usage: vsub [options] [- | path]");
    puts("  options:");
    puts("    -s, --syntax=STR  set syntax to use; default: 'default'");
    puts("    -e, --env         use environment variables");
    puts("    -E, --envsubst    same as '--env --syntax=ggenv'");
    puts("    -f, --format=STR  set output format; default: 'plain'");
    puts("    -d, --detailed    add extended details");
    puts("    -b, --no-color    turn off color in --detailed mode");
    puts("    -F, --formats     list supported output formats");
    puts("    -S, --syntaxes    list supported syntaxes");
    puts("    -V, --version     show tool name, version, and libvsub version");
    puts("    -h, --help        show this help and exit");
}

static const char *shortopts = "-hVbdeEf:Fs:S";
static struct option longopts[] = {
    {"detailed", no_argument, 0, 'd'},
    {"env", no_argument, 0, 'e'},
    {"envsubst", no_argument, 0, 'E'},
    {"no-color", no_argument, 0, 'b'},
    {"format", required_argument, 0, 'f'},
    {"formats", no_argument, 0, 'F'},
    {"syntax", required_argument, 0, 's'},
    {"syntaxes", no_argument, 0, 'S'},
    // standard
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
};

int main(int argc, char *argv[]) {
    // control flow
    bool result = true;
    char err[128] = "";
    size_t errsz = sizeof(err);
    // options
    bool use_color = false;  // unless --detailed and not --no-color
    bool no_color = false;   // --no-color default
    bool use_detailed = false;
    bool use_env = false;
    char *use_format = "plain";
    char *use_syntax = "default";
    char *path = "-";
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
            case 'V':
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
            case 'F':
                print_formats();
                goto done;
            case 'S':
                print_syntaxes();
                goto done;
            case 's':
                use_syntax = optarg;
                break;
            // positional
            case 1:
                if (pathind > 0) {
                    snprintf(err, errsz, "multiple paths not allowed\n");
                    result = false;
                    goto done;
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            // errors
            case '?':
                snprintf(err, errsz, "unrecognized option '%s'\n", argv[optind - 1]);
                result = false;
                goto done;
            default:
                snprintf(err, errsz, "unexpected getopt character code 0%o\n", o);
                result = false;
                goto done;
        }
    }
    // syntax
    if ((sub.syntax = vsub_syntax_lookup(use_syntax)) == NULL) {
        snprintf(err, errsz, "unsupported syntax %s\n", use_syntax);
        result = false;
        goto done;
    }
    // input
    if (strcmp(path, "-") != 0) {
        if (!(fp = fopen(path, "r"))) {
            snprintf(err, errsz, "unable to read file %s\n", path);
            result = false;
            goto done;
        }
        if (!vsub_use_text_from_file(&sub, fp)) {
            snprintf(err, errsz, "out of memory\n");
            result = false;
            goto done;
        }
    }
    // vars
    if (use_env) {
        if (!vsub_add_vars_from_env(&sub)) {
            snprintf(err, errsz, "out of memory\n");
            result = false;
            goto done;
        }
    }
    // output
    int outfmt;
    if ((outfmt = vsub_format_lookup(use_format)) == -1) {
        snprintf(err, errsz, "unsupported format %s\n", use_format);
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
                snprintf(err, errsz, "output error\n");
                result = false;
                goto done;
            }
            break;
        case VSUB_FMT_JSON:
            if (vsub_fputs_json(&sub, stdout, use_detailed) == EOF) {
                snprintf(err, errsz, "output error\n");
                result = false;
                goto done;
            }
            break;
        default:
            snprintf(err, errsz, "unsupported format %d\n", outfmt);
            result = false;
            goto done;
    }

done:
    vsub_print_error_sub(&sub, use_color);
    vsub_print_error_str(err, use_color);
    // deallocate resources
    vsub_free(&sub);
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
