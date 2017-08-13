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

#endif
