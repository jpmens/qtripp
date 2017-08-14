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

#ifndef _UTIL_H_INCL_
# define  _UTIL_H_INCL_

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif
#define __USE_XOPEN
#define __GNU_SOURCE
#include <time.h>
#include "udata.h"
#include "conf.h"

#ifndef MAXSPLITPARTS
# define MAXSPLITPARTS 400
#endif

int splitter(char *s, char *sep, char **parts);
void splitterfree(char **parts);
char *slurp_file(char *filename, int fold_newlines);
char **clean_split(struct udata *, char *line, int *nparts);
int str_time_to_secs(char *s, time_t *secs);
const char *tstamp(time_t t);
void xlog(struct udata *ud, char *fmt, ...);
void chomp(char *s);
char *device_to_topic(config *cf, char *did);
JsonNode *extra_json(config *cf, char *did);
double temp(char *hexs);

#endif
