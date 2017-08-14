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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "udata.h"
#include <math.h>

#ifndef LINESIZE
# define LINESIZE 8192
#endif


/*
 * Split the string at `s', separated by characters in `sep'
 * into individual strings, in array `parts'. The caller must
 * free `parts'.
 * Returns -1 on error, or the number of parts.
 */

int splitter(char *s, char *sep, char **parts)
{
        char *token, *p, *ds, *dsc;
        int nt = 0;

	if (!s)
		return (-1);

	if ((dsc = ds = strdup(s)) == NULL)
		return (-1);

	while ((token = strsep(&dsc, sep)) != NULL) {
		if (nt >= (MAXSPLITPARTS - 1))
			break;

		if ((p = strdup(token)) == NULL)
			return (-1);
		parts[nt++] = p;
        }
	parts[nt] = NULL;

        free(ds);
        return (nt);
}

void splitterfree(char **parts)
{
	int n;

	for (n = 0; parts[n] != NULL; n++)
		free(parts[n]);
}


char *slurp_file(char *filename, int fold_newlines)
{
	FILE *fp;
	char *buf, *bp;
	off_t len;
	int ch;

	if ((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	if (fseeko(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return (NULL);
	}
	len = ftello(fp);
	fseeko(fp, 0, SEEK_SET);

	if ((bp = buf = malloc(len + 1)) == NULL) {
		fclose(fp);
		return (NULL);
	}
	while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n') {
			if (!fold_newlines)
				*bp++ = ch;
		} else *bp++ = ch;
	}
	*bp = 0;
	fclose(fp);

	return (buf);
}

/*
 * Normalize `line', ensure it's bounded by + and $
 * then split CSV into array and return that or NULL.
 */

char **clean_split(struct udata *ud, char *line, int *nparts)
{
	static char *parts[MAXSPLITPARTS];
	char *lp;
	int llen;

	chomp(line);
	llen = strlen(line) - 1;

	// printf("LINE: [%s]\n", line);
	if (line[0] != '+' || line[llen] != '$') {
		xlog(ud, "expecting + .. $ on LINE [%s]\n", line);
		return (NULL);
	}

	lp = &line[1];
	line[llen] = 0;	/* chop $ */

	if ((*nparts = splitter(lp, ",", parts)) < 1)
		return (NULL);

	return (parts);
}

/*
 * `s' has a time string in it. Try to convert into time_t
 * using a variety of formats from higher to lower precision.
 * Return 1 on success, 0 on failure.
 */

int str_time_to_secs(char *s, time_t *secs)
{
	static char **f, *formats[] = {
			"%Y%m%d%H%M%S",
			"%Y-%m-%dT%H:%M",
			"%Y-%m-%dT%H",
			"%Y-%m-%dt%H:%M:%S",
			"%Y-%m-%dt%H:%M",
			"%Y-%m-%dt%H",
			"%Y-%m-%d",
			"%Y-%m",
			NULL
		};
	struct tm tm;
	int success = 0;

	memset(&tm, 0, sizeof(struct tm));
	for (f = formats; f && *f; f++) {
		if (strptime(s, *f, &tm) != NULL) {
			success = 1;
			// fprintf(stderr, "str_time_to_secs succeeds with %s\n", *f);
			break;
		}
	}

	if (!success)
		return (0);

	tm.tm_mday = (tm.tm_mday < 1) ? 1 : tm.tm_mday;
	tm.tm_isdst = -1; 		/* A negative value for tm_isdst causes
					 * the mktime() function to attempt to
					 * divine whether summer time is in
					 * effect for the specified time. */

	*secs = mktime(&tm);
	// fprintf(stderr, "str_time_to_secs: %s becomes %04d-%02d-%02d %02d:%02d:%02d\n",
	// 	s,
	// 	tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	// 	tm.tm_hour, tm.tm_min, tm.tm_sec);

	return (1);
}

void xlog(struct udata *ud, char *fmt, ...)
{
	va_list ap;
	time_t now = time(0);
	FILE *fp;

	fp = (ud == NULL) ? stderr : ud->logfp;

	fprintf(fp, "%s %ld ", tstamp(now), now);
	va_start(ap, fmt);

	vfprintf(fp, fmt, ap);
	fflush(fp);
	va_end(ap);
}

const char *tstamp(time_t t) {
        static char buf[] = "YYYY-MM-DDTHH:MM:SSZ";

        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
        return(buf);
}

void chomp(char *s)
{
	char *bp;

	for (bp = s + strlen(s) - 1; bp >= s; bp--) {
		if (isspace(*bp)) {
			*bp = 0;
		} else {
			return;
		}
	}
}

/*
 * Find a topic name to use for publishing to for device `did`.
 * We stab at the hash and a match uses that topic. If no match
 * is found but the wildcard `*' exists, we use that topic, appending
 * the `did' to it.
 */

char *device_to_topic(config *cf, char *did)
{
	static char buf[BUFSIZ];
	struct my_device *d;

	HASH_FIND_STR(cf->devices, did, d);
	if (!d) {
		HASH_FIND_STR(cf->devices, "*", d);
		if (!d)
			return (NULL);
		snprintf(buf, sizeof(buf), "%s%s", d->topic, did);
		return (buf);
	}

	return (d->topic);
}

JsonNode *extra_json(config *cf, char *did)
{
	char path[BUFSIZ];
	char *bp = NULL;
	JsonNode *j = NULL;

	snprintf(path, sizeof(path), "%s/%s", cf->extra_json, did);

	if ((bp = slurp_file(path, true)) != NULL) {
		j = json_decode(bp);
		free(bp);
	}
	return (j);
}


double temp(char *hexs)
{
        double celsius;
        long l;

        l = strtol((hexs && *hexs) ? hexs : "00", NULL, 16);
        if (l & 0xF800) {
		l &= 0x07FF;
		l += 1;
		l *= -1;
	}

        celsius = (double)(l * 0.0625);
        return (celsius);
}

/* http://rosettacode.org/wiki/Haversine_formula#C */
/* Changed to return meters instead of KM (* 1000) */

#define R 6371
#define TO_RAD (3.1415926536 / 180.0)

double haversine_dist(double th1, double ph1, double th2, double ph2)
{
        double dx, dy, dz;
        ph1 -= ph2;
        ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;

        dz = sin(th1) - sin(th2);
        dx = cos(ph1) * cos(th1) - cos(th2);
        dy = sin(ph1) * cos(th1);

        return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R * 1000;
}

