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

#include "ignores.h"
#include "ignores.i"	/* generated from ignores.j2 */

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
