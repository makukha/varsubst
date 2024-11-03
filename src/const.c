#include "vsub.h"


// syntax dialects

const int VSUB_SXS_LIST[] = {
    VSUB_SX_DEFAULT,
    VSUB_SX_DC243,
};

const size_t VSUB_SXS_COUNT = sizeof(VSUB_SXS_LIST) / sizeof(VSUB_SXS_LIST[0]);

const char *vsub_syntax_short_str(int sx) {
    switch (sx) {
        case VSUB_SX_DEFAULT: return "default";
        case VSUB_SX_DC243: return "dc243";
        default: return NULL;
    }
}

const char *vsub_syntax_long_str(int sx) {
    switch (sx) {
        case VSUB_SX_DEFAULT: return "Simple direct substitution";
        case VSUB_SX_DC243: return "Docker Compose v2.4.3";
        default: return NULL;
    }
}


// missing variable types

const char *vsub_missing_str(char mv) {
    switch (mv) {
        case VSUB_MV_VALID: return "valid";
        case VSUB_MV_UNSET: return "unset";
        case VSUB_MV_EMPTY: return "empty";
        default: return NULL;
    }
}
