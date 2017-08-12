#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "reports.h"
#include "data_reports.i"	/* generated from reports.j2 */

static struct _report *myhash = NULL;

void load_reports()
{
	struct _report *rp;

	for (rp = reports; rp->id != NULL; rp++) {
		// s = (struct _report *)malloc(sizeof(struct _report));
		// s->id = rp->id;
		// s->desc = rp->desc;
		// HASH_ADD_KEYPTR(hh, myhash, s->id, strlen(s->id), s);
		HASH_ADD_KEYPTR(hh, myhash, rp->id, strlen(rp->id), rp);
	}
}
void free_reports()
{
	struct _report *s, *tmp;

	HASH_ITER(hh, myhash, s, tmp) {
		HASH_DEL(myhash, s);
		/* nothing to free as the structure itself is static */
		// free(s);
	}
}

struct _report *lookup_reports(char *key)
{
	struct _report *s;

	HASH_FIND_STR(myhash, key, s);
	return (s);
}

#ifdef TESTING
int main()
{
	struct _report *rp;

	load_reports();

	rp = lookup_reports("GTFRI");
	if (rp)
		puts(rp->desc);


	free_reports();
}
#endif
