#include <stdio.h>
#include <stdlib.h>

#include <cjson/cJSON.h>
#include "../vsub.h"


int vsub_OutputJson(Vsub *sub, FILE *fp, bool detailed) {
    int ret = 0;
    cJSON *data = NULL;
    char *text = NULL;
    if (!(data = vsub_results(sub, detailed))) {
        ret = EOF;
        goto done;
    }
    if (!(text = cJSON_PrintUnformatted(data))) {
        ret = EOF;
        goto done;
    }
    fprintf(fp, "%s\n", text);

done:
    cJSON_Delete(data);
    if (text) {
        free(text);
    }
    return ret;
}
