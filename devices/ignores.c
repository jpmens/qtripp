#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ignores.h"
#include "data_ignores.i"	/* generated from ignores.j2 */

static struct _ignore *myhash = NULL;

void load_ignores()
{
	struct _ignore *rp;

	for (rp = ignores; rp->id != NULL; rp++) {
		HASH_ADD_KEYPTR(hh, myhash, rp->id, strlen(rp->id), rp);
	}
}
void free_ignores()
{
	struct _ignore *s, *tmp;

	HASH_ITER(hh, myhash, s, tmp) {
		HASH_DEL(myhash, s);
		/* nothing to free as the structure itself is static */
		// free(s);
	}
}

struct _ignore *lookup_ignores(char *key)
{
	struct _ignore *s;

	HASH_FIND_STR(myhash, key, s);
	return (s);
}

#ifdef TESTING
int main()
{
	struct _ignore *rp;

	load_ignores();

	rp = lookup_ignores("31");
	if (rp)
		puts(rp->desc);


	free_ignores();
}
#endif
