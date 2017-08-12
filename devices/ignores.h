#include <string.h>
#include <stdlib.h>
#include "uthash.h"

struct _ignore {
	char *id;		/* "GTSTC" */
	char *reason;		/* "because I'm not interested in this" */
        UT_hash_handle hh;
};

void load_ignores();
void free_ignores();
struct _ignore *lookup_ignores(char *key);
