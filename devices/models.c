#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "models.h"
#include "data_models.i"	/* generated from models.j2 */

static struct _model *myhash = NULL;

void load_models()
{
	struct _model *rp;

	for (rp = models; rp->id != NULL; rp++) {
		// s = (struct _report *)malloc(sizeof(struct _report));
		// s->id = rp->id;
		// s->desc = rp->desc;
		// HASH_ADD_KEYPTR(hh, myhash, s->id, strlen(s->id), s);
		HASH_ADD_KEYPTR(hh, myhash, rp->id, strlen(rp->id), rp);
	}
}
void free_models()
{
	struct _model *s, *tmp;

	HASH_ITER(hh, myhash, s, tmp) {
		HASH_DEL(myhash, s);
		/* nothing to free as the structure itself is static */
		// free(s);
	}
}

struct _model *lookup_models(char *key)
{
	struct _model *s;

	HASH_FIND_STR(myhash, key, s);
	return (s);
}

#ifdef TESTING
int main()
{
	struct _model *rp;

	load_models();

	rp = lookup_models("31");
	if (rp)
		puts(rp->desc);


	free_models();
}
#endif
