#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsub.h"


#define debug(...) fprintf(stderr, __VA_ARGS__)
#define error(...) fprintf(stderr, __VA_ARGS__)

const char *version = "vsub " VSUB_VERSION;

static void print_version() {
    puts(version);
}

static void print_usage() {
    puts("usage: vsub [options] [- | path]");
    puts("  options:");
    puts("    -s, --syntax=name  syntax to use; default: 'default'");
    puts("    -e, --env          use environment variables");
    puts("        --envsubst     use environment variables and 'ggenv' syntax");
    puts("        --syntaxes     print list of supported syntaxes");
    puts("        --debug        print verbose log to stderr");
    puts("        --version      show tool name, version, and libvsub version");
    puts("    -h, --help         show this help and exit");
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

static const char *shortopts = "-hVdeEs:S";
static struct option longopts[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"debug", no_argument, 0, 'D'},
    {"env", no_argument, 0, 'e'},
    {"envsubst", no_argument, 0, 'E'},
    {"syntax", required_argument, 0, 's'},
    {"syntaxes", no_argument, 0, 'S'},
};

int main(int argc, char *argv[]) {
    bool result = true;
    Vsub sub;
    vsub_init(&sub);
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
                exit(EXIT_SUCCESS);
            case 'V':
                print_version();
                exit(EXIT_SUCCESS);
            case 'D':
                use_debug = true;
                debug("%s\n", version);
                break;
            case 'S':
                print_syntaxes();
                exit(EXIT_SUCCESS);
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
                    error("multiple path args not allowed\n");
                    exit(EXIT_FAILURE);
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            default:
                error("unexpected getopt character code 0%o\n", o);
                exit(EXIT_FAILURE);
        }
    }

    // syntax
    if ((sub.syntax = vsub_find_syntax(use_syntax)) == NULL) {
        error("unsupported syntax %s\n", use_syntax);
        result = false;
        goto done;
    }
    // input
    if (strcmp(path, "-") != 0) {
        if (!(fp = fopen(path, "r"))) {
            error("unable to read file %s\n", path);
            result = false;
            goto done;
        }
        if (!vsub_use_text_from_file(&sub, fp)) {
            result = false;
            goto done;
        }
    }
    // vars
    if (use_env) {
        if (!vsub_add_vars_from_env(&sub)) {
            result = false;
            goto done;
        }
    }

    // run
    if (use_debug) {
        debug("input file: %s\n", (fp == stdin) ? "<stdin>" : path);
        debug("syntax: %s (%s)\n", sub.syntax->name, sub.syntax->title);
        debug("text source: %s\n", sub.aux.tsrc->name);
        debug("vars sources:");
        for (VsubVarsSrc *vsrc = sub.aux.vsrc; vsrc != NULL; vsrc = vsrc->prev) {
            debug(" %s", vsrc->name);
        }
        debug("\n");
        debug("max input length: %ld%s\n", sub.maxinp, sub.maxinp == 0 ? " (unlimited)" : "");
        debug("max result length: %ld%s\n", sub.maxres, sub.maxres == 0 ? " (unlimited)" : "");
        debug("depth: %d\n", sub.depth);
    }
    if (!(result = vsub_run(&sub))) {
        result = false;
        goto done;
    }
    puts(sub.res);

done:
    if (use_debug) {
        debug("result: %s\n", sub.res);
        debug("plain: %s\n", (sub.substc == 0) ? "yes" : "no");
        debug("truncated: %s\n", sub.trunc ? "yes" : "no");
        debug("read characters: %ld\n", sub.inpc);
        debug("result characters: %ld\n", sub.resc);
        debug("substitutions made: %ld\n", sub.substc);
        debug("nesting level: %d\n", sub.depth);
        debug("parser error code: %d\n", sub.err);
        debug("first error variable name: %s\n", sub.errvar);
        debug("first error variable message: %s\n", sub.errmsg);
    }
    switch (sub.err) {
        case VSUB_SUCCESS:
            // no parser errors
            break;
        case VSUB_INVALID_SYNTAX:
            error("invalid input syntax on pos %ld\n", sub.inpc);
            break;
        case VSUB_VAR_ERROR:
            error("variable \"%s\" on pos %ld: %s\n", sub.errvar, sub.inpc, sub.errmsg);
            break;
        case VSUB_PARSER_ERROR:
            error("unexpected parser error on pos %ld\n", sub.inpc);
            break;
        case VSUB_MEMORY_ERROR:
            error("out of memory\n");
            break;
        default:
            break;
    }
    if (fp != stdin && fp != NULL) {
        fclose(fp);
    }
    vsub_free(&sub);
    if (use_debug) {
        if (result) {
            debug("Done.\n");
        }
        else {
            debug("Failed.\n");
        }
    }
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}
