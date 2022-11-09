/*
 * qtripp
 * Copyright (C) 2017-2020 Jan-Piet Mens <jp@mens.de>
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

#include "models.h"
#include "models.i"	/* generated from models.j2 */

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

	// the key consists of a model identifier (2 or 6 bytes) plus 4 bytes of version string
	// e.g. 380D01 for GV65Plus (x38) version 13.1
	// e.g. 8020030100 for GV58CEU (x802003) version 1.0
	char t[6+1];

	if (!key || strlen(key) < 5)
		return (NULL);

	strncpy(t, key, strlen(key) - 4);

	HASH_FIND_STR(myhash, t, s);
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
