#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

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

char **clean_split(char *line, int *nparts)
{
	static char *parts[MAXSPLITPARTS];
	char *lp;
	int llen = strlen(line) - 1;


	if (line[llen] == '\n')
		line[llen--] = 0;

	// printf("LINE: [%s]\n", line);
	if (line[0] != '+' || line[llen] != '$') {
		printf("expecting + .. $ on LINE [%s]", line);
		return (NULL);
	}

	lp = &line[1];
	line[llen] = 0;	/* chop $ */

	// printf("[%s]\n", lp);

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
