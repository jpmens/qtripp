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
