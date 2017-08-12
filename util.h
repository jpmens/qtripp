#ifndef _UTIL_H_INCL_
# define  _UTIL_H_INCL_

#include <time.h>
#include "json.h"

#ifndef MAXSPLITPARTS
# define MAXSPLITPARTS 400
#endif

int splitter(char *s, char *sep, char **parts);
void splitterfree(char **parts);
char *slurp_file(char *filename, int fold_newlines);
char **clean_split(char *line, int *nparts);
int str_time_to_secs(char *s, time_t *secs);
const char *tstamp(time_t t);
void debug(char *fmt, ...);

#endif
