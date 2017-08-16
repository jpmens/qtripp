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
#include <stdbool.h>
#include <stdio.h>
#include <util.h>

#include "iinfo.h"

static struct _iinfo *myhash = NULL;

void free_iinfo()
{
	struct _iinfo *s, *tmp;

	HASH_ITER(hh, myhash, s, tmp) {
		HASH_DEL(myhash, s);
		free(s);
	}
}

/*
 * This doesn't just look up; it inserts into hash also when,
 * upon initial lookup, the key isn't found.
 */

struct _iinfo *lookup_iinfo(const char *directory, char *key)
{
	char path[BUFSIZ];
	struct _iinfo *s;

	if (!key || !*key || !directory || !*directory)
		return (NULL);

	HASH_FIND_STR(myhash, key, s);
	if (!s) {
		snprintf(path, sizeof(path), "%s/%s", directory, key);
		char *name = slurp_file(path, true);

		s = (struct _iinfo *)malloc(sizeof(struct _iinfo));
		strcpy(s->key, key);
		s->name = (name) ? name : strdup(".");

		HASH_ADD_STR(myhash, key, s);
	}

	return (s);
}

#ifdef TESTING
int main()
{
	struct _iinfo *ip;

	ip = lookup_iinfo("names", "863286023345490");
	if (ip)
		puts(ip->name);


	free_iinfo();
}
#endif
