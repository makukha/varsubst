#ifndef VSUB_AUXIL_H
#define VSUB_AUXIL_H

typedef struct Auxil {
    char (*getchar)(void *auxil);
    void (*append_char)(void *auxil, char c);
    void (*append_value)(void *auxil, char *varname, char *match);
    void (*set_syntax_error)(void *auxil);
} Auxil;

#define PCC_ERROR(auxil) auxil->set_syntax_error(auxil)
#define PCC_GETCHAR(auxil) auxil->getchar(auxil)

#endif  // VSUB_AUXIL_H
