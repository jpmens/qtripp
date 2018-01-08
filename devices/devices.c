/*
 * qtripp
 * Copyright (C) 2017 Jan-Piet Mens <jp@mens.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "devices.h"
#include "devices.i"	/* generated from devices.j2 */

static struct _device *myhash = NULL;

void load_devices()
{
	struct _device *rp, *s;

	for (rp = devices; rp->id != NULL; rp++) {
		HASH_FIND_STR(myhash, rp->id, s);
		if (s != NULL) {
			fprintf(stderr, "Fatal: device hash for %s already in hash\n", rp->id);
			exit(7);
		}

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
 * Look up device subtype (e.g. GTFRI) with model/major/minor.
 * If that particular MOMAMI doesn't exist, return the data for MO0000 if existent.
 * If that doesn't exist, return the data for 00MAMI if existent.
 * If that doesn't exist, return the data for 00000 if existent.
 */

struct _device *lookup_devices(char *key, char *momami)
{
	struct _device *s;
	char id[64];

	snprintf(id, sizeof(id), "%s-%s", key, momami);
	HASH_FIND_STR(myhash, id, s);
	if (s)
		return (s);

	snprintf(id, sizeof(id), "%s-%.2s0000", key, momami);
	HASH_FIND_STR(myhash, id, s);
	if (s)
		return (s);

	snprintf(id, sizeof(id), "%s-00%s", key, momami + 2);
	HASH_FIND_STR(myhash, id, s);
	if (s)
		return (s);

	snprintf(id, sizeof(id), "%s-000000", key);
	HASH_FIND_STR(myhash, id, s);

	return (s);
}

#ifdef TESTING
int main()
{
	struct _device *rp;

	load_devices();

	rp = lookup_devices("GTFRI", "2C0600");
	if (rp) {
		printf("%s -> %d\n", rp->id, rp->num);
	}
	rp = lookup_devices("GTFRI", "308891");
	if (rp) {
		printf("%s -> %d\n", rp->id, rp->num);
	}


	free_devices();
}
#endif
