#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cdb.h>
#include "constfile.h"

/*
 * constfile
 * Copyright (C) 2018-2020 Jan-Piet Mens <jp@mens.de>
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

cofi *constfile_open(char *filename)
{
	cofi *f;
        struct stat sb;
	int fd;

	if ((fd = open(filename, O_RDONLY)) == -1) {
		perror(filename);
		return (NULL);
	}

	assert((f = malloc(sizeof(struct cofi))) != NULL);
	f->filename = strdup(filename);
	f->fd = fd;
	assert(fstat(fd, &sb) != -1);
	f->mtime = sb.st_mtime;

	return (f);
}

void constfile_close(cofi *f)
{
	assert(f);
	free(f->filename);
	close(f->fd);
	free(f);
}

/*
 * Check mtime of file at `f' and re-open if necessary.
 */

void constfile_checkfile(cofi *f)
{
        struct stat sb;

	assert(f);
	if (stat(f->filename, &sb) != -1) {
		// fprintf(stderr, "mtimes: old %ld new %ld\n", f->mtime, sb.st_mtime);
		if (sb.st_mtime != f->mtime) {
			close(f->fd);
			assert((f->fd = open(f->filename, O_RDONLY)) != -1);
			f->mtime = sb.st_mtime;
		}
	}
}

/*
 * Read CDB of `key' into `buf' of `buflen` - 1 bytes, and null-terminate it.
 * If `buf' is NULL, allocate the space which caller must then free.
 * Return a pointer to buf or allocated space
 */

char *constfile_stab(cofi *f, char *key, char *buf, unsigned buflen)
{
	char *data;
        int rc;
	unsigned dlen;

	if (!key || !*key)
		return (NULL);

        constfile_checkfile(f);

        if ((rc = cdb_seek(f->fd, key, strlen(key), &dlen)) == 0)
		return (NULL);
	assert(rc > 0);

	if (buf == NULL) {
		if ((data = malloc(dlen + 1)) == NULL) {
			return (NULL);
		}
	} else {
		data = buf;
		dlen = (dlen > (buflen - 1)) ? buflen - 1 : dlen;
	}

	if (cdb_bread(f->fd, (void *)data, dlen) == 0) {
		data[dlen] = 0;
	} else {
		perror("cdb_read");
		free(data);
		data = NULL;
	}
        return (data);
}
