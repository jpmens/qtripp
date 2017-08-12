#include <string.h>
#include <stdlib.h>
#include "uthash.h"

struct _report {
	char *id;		/* GTFRI */
	char *desc;		/* bla bla report description */
        UT_hash_handle hh;
};

void load_reports();
void free_reports();
struct _report *lookup_reports(char *key);
