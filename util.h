#ifndef _UTIL_H_INCL_
# define  _UTIL_H_INCL_

#ifndef MAXSPLITPARTS
# define MAXSPLITPARTS 400
#endif

int splitter(char *s, char *sep, char **parts);
void splitterfree(char **parts);

#endif
