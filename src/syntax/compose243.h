/* A packrat parser generated by PackCC 2.0.2 */

#ifndef PCC_INCLUDED_COMPOSE243_H
#define PCC_INCLUDED_COMPOSE243_H

#include "../aux.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vsub_sx_compose243_context_tag vsub_sx_compose243_context_t;

vsub_sx_compose243_context_t *vsub_sx_compose243_create(Auxil *auxil);
int vsub_sx_compose243_parse(vsub_sx_compose243_context_t *ctx, const char **ret);
void vsub_sx_compose243_destroy(vsub_sx_compose243_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* !PCC_INCLUDED_COMPOSE243_H */
