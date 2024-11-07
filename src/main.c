#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"


static void print_version() {
    puts("vsub " VSUB_VERSION);
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
    puts("    -s, --syntax=name  syntax to use; default: 'default'");
    puts("    -e, --env          use environment variables");
    puts("        --envsubst     use environment variables and 'ggenv' syntax");
    puts("        --syntaxes     print list of supported syntaxes");
    puts("    -n, --no-color     turn off color in debug mode");
    puts("        --debug        print verbose log to stderr");
    puts("        --version      show tool name, version, and libvsub version");
    puts("    -h, --help         show this help and exit");
}

static const char *shortopts = "-hVdneEs:S";
static struct option longopts[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"debug", no_argument, 0, 'D'},
    {"no-color", no_argument, 0, 'n'},
    {"env", no_argument, 0, 'e'},
    {"envsubst", no_argument, 0, 'E'},
    {"syntax", required_argument, 0, 's'},
    {"syntaxes", no_argument, 0, 'S'},
};

int main(int argc, char *argv[]) {
    bool result = true;
    char err[64] = "";
    Vsub sub;
    vsub_init(&sub);
    bool use_color = false;  // unless --debug and not --no-color
    bool no_color = false;  // --no-color default
    bool use_debug = false;
    bool use_env = false;
    char *use_syntax = "default";
    char *path = "-";
    int pathind = 0;
    FILE *fp = stdin;

    int o;
    while ((o = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (o) {
            case 'h':
                print_usage();
                goto done;
            case 'V':
                print_version();
                goto done;
            case 'D':
                use_debug = true;
                break;
            case 'n':
                no_color = true;
                break;
            case 'S':
                print_syntaxes();
                goto done;
            case 's':
                use_syntax = optarg;
                break;
            case 'e':
                use_env = true;
                break;
            case 'E':
                use_env = true;
                use_syntax = "ggenv";
                break;
            case 1:
                if (pathind > 0) {
                    snprintf(err, 64, "multiple path args not allowed\n");
                    result = false;
                    goto done;
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            default:
                snprintf(err, 64, "unexpected getopt character code 0%o\n", o);
                result = false;
                goto done;
        }
    }
    // color
    use_color = (use_debug && !no_color) ? true : false;
    // title
    if (use_debug) {
        vsub_print_debug_title(use_color);
    }
    // syntax
    if ((sub.syntax = vsub_syntax_lookup(use_syntax)) == NULL) {
        snprintf(err, 64, "unsupported syntax %s\n", use_syntax);
        result = false;
        goto done;
    }
    // input
    if (strcmp(path, "-") != 0) {
        if (!(fp = fopen(path, "r"))) {
            snprintf(err, 64, "unable to read file %s\n", path);
            result = false;
            goto done;
        }
        if (!vsub_use_text_from_file(&sub, fp)) {
            snprintf(err, 64, "out of memory\n");
            result = false;
            goto done;
        }
    }
    // vars
    if (use_env) {
        if (!vsub_add_vars_from_env(&sub)) {
            snprintf(err, 64, "out of memory\n");
            result = false;
            goto done;
        }
    }

    // run
    if (use_debug) {
        vsub_print_debug_metrics(&sub, true, use_color);
    }
    if (!vsub_alloc(&sub)) {
        result = false;
        goto done;
    }
    if (!vsub_run(&sub)) {
        result = false;
        goto done;
    }
    if (!use_debug) {
        puts(sub.res);
    }

done:
    vsub_print_error_str(err, use_color);
    vsub_print_error_sub(&sub, use_color);
    if (use_debug) {
        vsub_print_debug_metrics(&sub, false, use_color);
    }
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    vsub_free(&sub);
    if (use_debug) {
        vsub_print_debug_result_status(result, use_color);
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
