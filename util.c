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

#if 0
JsonNode *load_types(char *filename)
{
	char *data = slurp_file(filename, true);
	JsonNode *obj = NULL;

	if (data != NULL) {
		obj = json_decode(data);
	}
	return (obj);
}

char *get_model(JsonNode *models, char *type)
{
	JsonNode *o = json_find_member(models, type);

	if (o == NULL)
		return ("huh?");

	return o->string_;
}
#endif
