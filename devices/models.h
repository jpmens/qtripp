#include <string.h>
#include <stdlib.h>
#include "uthash.h"

struct _model {
	char *id;		/* "31" */
	char *desc;		/* "GV65" */
        UT_hash_handle hh;
};

void load_models();
void free_models();
struct _model *lookup_models(char *key);
