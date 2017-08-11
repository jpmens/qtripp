#ifndef _UTIL_H_INCL_
# define  _UTIL_H_INCL_

#include "json.h"

#ifndef MAXSPLITPARTS
# define MAXSPLITPARTS 400
#endif

int splitter(char *s, char *sep, char **parts);
void splitterfree(char **parts);
char *slurp_file(char *filename, int fold_newlines);
// JsonNode *load_types(char *filename);
// char *get_model(JsonNode *models, char *type);

#endif
