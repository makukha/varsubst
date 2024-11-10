#ifndef VSUB_AUX_H
#define VSUB_AUX_H

#include "vsub.h"


// --- packcc configuration

// parser generator settings
#define PCC_ERROR(auxil) { ((Vsub*)auxil->sub)->err = VSUB_ERROR_SYNTAX; return 0; }
#define PCC_GETCHAR(auxil) auxil->getchar(auxil)

// actions
#define use_Input auxil->append_orig(auxil, _0e, _0)
#define use_Value auxil->append_subst(auxil, _0e, __val)
#define use_Other auxil->append_subst(auxil, _0e, other)
#define use_Error { auxil->append_error(auxil, _0e, var, errmsg); return 0; }
#define USE(a) use_ ## a;

// rules
#define __get_Value const char *__val = auxil->getvalue(auxil, var)
#define __is_Set (__val != NULL)
#define __is_Empty (__val != NULL && strlen(__val) == 0)
#define __is_Filled (__val != NULL && strlen(__val) >= 1)
#define __is_Missing (__val == NULL || strlen(__val) == 0)
#define IF(s) { __get_Value; if __is_ ## s
#define THEN(a) use_ ## a;
#define ELSE(a) else use_ ## a; }


#endif  // VSUB_AUX_H
