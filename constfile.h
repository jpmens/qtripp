#ifndef _CONSTFILE_H_
#define  _CONSTFILE_H_
#include <sys/time.h>

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

typedef struct cofi {
        char *filename;
        int fd;
        time_t mtime;
} cofi;

cofi *constfile_open(char *filename);
void constfile_close(cofi *f);
void constfile_checkfile(cofi *f);
char *constfile_stab(cofi *f, char *key, char *buf, unsigned buflen);

#endif /* _CONSTFILE_H_ */
