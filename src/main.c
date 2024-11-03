#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsub.h"


const char *version = "vsub " VSUB_VERSION;

static void print_version() {
    puts(version);
}

static void print_usage() {
    puts("usage: vsub [options] [- | path]");
    puts("  options:");
    puts("    -s, --syntax=id  syntax to use if not 'default'");
    puts("    -e, --env        use environment variables");
    puts("        --syntaxes   print list of supported syntaxes");
    puts("        --version    show tool name, version, and libvsub version");
    puts("    -h, --help       show this help and exit");
}

static void print_syntaxes() {
    // measure first column width
    int shortlen = 0;
    for (size_t i = 0; i < VSUB_SXS_COUNT; i++) {
        int len = strlen(vsub_syntax_short_str(VSUB_SXS_LIST[i]));
        shortlen = (len > shortlen) ? len : shortlen;
    }
    // print
    char fmt[24];
    sprintf(fmt, "%%-%ds  %%s\n", shortlen);
    printf(fmt, "ID", "DESC");
    for (size_t i = 0; i < VSUB_SXS_COUNT; i++) {
        printf(fmt, vsub_syntax_short_str(VSUB_SXS_LIST[i]),
            vsub_syntax_long_str(VSUB_SXS_LIST[i]));
    }
}

static const char *shortopts = "-hVeS";
static struct option longopts[] = {
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"env", no_argument, 0, 'e'},
    {"syntaxes", no_argument, 0, 'S'},
};


int main(int argc, char *argv[]) {
    bool use_env = false;
    char *path = "-";
    int pathind = 0;

    int o;

    while ((o = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (o) {
            case 'h':
                print_usage();
                exit(EXIT_SUCCESS);
            case 'V':
                print_version();
                exit(EXIT_SUCCESS);
            case 'S':
                print_syntaxes();
                exit(EXIT_SUCCESS);
            case 'e':
                use_env = true;
                break;
            case 1:
                if (optind > 0) {
                    fprintf(stderr, "Multiple path args not allowed");
                    exit(EXIT_FAILURE);
                }
                pathind = optind - 1;
                path = argv[pathind];
                break;
            default:
                fprintf(stderr, "Unexpected getopt character code 0%o", o);
                exit(EXIT_FAILURE);
        }
    }

//    if (cmd == NULL) {
//        log_error("Missing command");
//        success = false;
//    }
//    else if (subcmd == NULL) {
//        log_error("Missing subcommand");
//        success = false;
//    }
//    else if (strcmp(cmd, "img") == 0 && strcmp(subcmd, "ls") == 0) {
//        int pathc = 1;
//        char *default_path = ".";
//        char **pathv = &default_path;
//        if (first_arg_index > 0) {
//            pathc = argc - first_arg_index;
//            pathv = &argv[first_arg_index];
//        }
//        success = dockerfi_img_ls(pathc, pathv, recurse);
//    }
//    else {
//        log_error("Unsupported command: %s %s", cmd, subcmd);
//    }

    exit(EXIT_SUCCESS);
}
