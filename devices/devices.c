#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "devices.h"
#include "data_devices.i"	/* generated from devices.j2 */

static struct _device *myhash = NULL;

void load_devices()
{
	struct _device *rp;

	for (rp = devices; rp->id != NULL; rp++) {
		// s = (struct _report *)malloc(sizeof(struct _report));
		// s->id = rp->id;
		// s->desc = rp->desc;
		// HASH_ADD_KEYPTR(hh, myhash, s->id, strlen(s->id), s);
		HASH_ADD_KEYPTR(hh, myhash, rp->id, strlen(rp->id), rp);
	}
}
void free_devices()
{
	struct _device *s, *tmp;

	HASH_ITER(hh, myhash, s, tmp) {
		HASH_DEL(myhash, s);
		/* nothing to free as the structure itself is static */
		// free(s);
	}
}

/*
 * Look up device subtype (e.g. GTFRI) with major/minor. If
 * that particular JJMM doesn't exist, return the data for 0000
 * if existent.
 */

struct _device *lookup_devices(char *key, char *jjmm)
{
	struct _device *s;
	char id[64];

	snprintf(id, sizeof(id), "%s-%s", key, jjmm);
	HASH_FIND_STR(myhash, id, s);
	if (s)
		return (s);

	snprintf(id, sizeof(id), "%s-0000", key);
	HASH_FIND_STR(myhash, id, s);
	return (s);
}

#ifdef TESTING
int main()
{
	struct _device *rp;

	load_devices();

	rp = lookup_devices("GTFRI", "0201");
	if (rp) {
		printf("%s -> %d\n", rp->id, rp->num);
	}
	rp = lookup_devices("GTFRI", "8891");
	if (rp) {
		printf("%s -> %d\n", rp->id, rp->num);
	}


	free_devices();
}
#endif
