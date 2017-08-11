#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "util.h"
#define MAXLINELEN	8192

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

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)

typedef enum vtype { D, S } vtype;
struct tab {
	char *fieldname;
	vtype vt;
	int slab;
};

static struct table {
	char *subtype;
	int multi;		/* Has multiple reports (e.g. GTFRI); 0=no, >0 field number */
	struct tab tab[10];
} tabs[] = {
	"GTSTT", 0,	{
				{ "imei",	S,	2 },
				{ "acc",	D,	5 },
				{ "vel",	D,	6 },
				{ "alt",	D,	8 },
				{ "lon",	D,	9 },
				{ "lat",	D,	10 },
				{ "utc",	S,	11 },
				{ NULL, 	D, 	-1 }
			},
	"GTFRI", 6,	{
				{ "imei",	S,	 2 },
				{ "vel",	D,	 8 },
				{ "alt",	D,	10 },
				{ "lon",	D,	11 },
				{ "lat",	D,	12 },
				{ "utc",	S,	13 },
				{ NULL, 	D, 	-1 }
			},
	NULL, 0,		{ }
};

static struct device_type {
	char *type;
	char *model;
} device_types[] = {
	{ "31",		"GV65" 		},
	{ "36",		"GV500"		},
	{ "38",		"GV65+"		},
	{ "42",		"GMT200N"	},
	{ NULL,		NULL		}
};

int main()
{
	char line[MAXLINELEN], **parts, *typeparts[4];
	int n, nparts;
	char *abr, *subtype;		/* abr= ACK, BUFF, RESP, i.e. the bit before : */

	while (fgets(line, sizeof(line) - 1, stdin) != NULL) {
		if (*line == '#')
			continue;
		parts = clean_split(line, &nparts);
		if (parts == NULL) {
			printf("Can't split\n");
			continue;
		}

		if ((n = splitter(parts[0], ":", typeparts)) != 2) {
			printf("Can't split ABR:SUBTYPE\n");
			continue;
		}
		abr = typeparts[0];
		subtype = typeparts[1];

		if (strcmp(abr, "ACK") == 0) {
			continue;
		}

		for (n = 0; tabs[n].subtype != NULL; n++) {
			struct tab *ta;
	
			if (strcmp(subtype, tabs[n].subtype) == 0) {
				int rep = 0, nreports = atoi(parts[tabs[n].multi]);	/* "Number" from docs */
				char *protov = GET_S(1);	/* VVJJMM
								 * VV = model
								 * JJ = major
								 * MM = minor 
								 */
				char *device_model = "unknown";
				struct device_type *dt;

				for (dt = device_types; dt->type != NULL; dt++) {
					if (!strncmp(dt->type, protov, 2)) {
						device_model = dt->model;
						break;
					}
				}


				if (nparts == 31) {  // FIXME: GV500 
					nreports = atoi(parts[tabs[n].multi + 1]);
				}

				printf("**** model=%s special %d ** (nparts=%d, proto=%s) LINE=%s\n", device_model, nreports, nparts, protov, line);
				printf("[[%s]]\n", tabs[n].subtype);

				/* handle sub-reports of e.g GTFRI. Even if a subtype
				 * doesn't have sub-reports, we enter this and do it
				 * just once.
				 */

				do {
					for (ta = &tabs[n].tab[0]; ta->fieldname; ta++) {
						double d;
						char *s;
						int pos = (rep * 12) + ta->slab;
	
						printf("** pos=%d  %s: ", pos, ta->fieldname);
						if (ta->vt == D) {
							d = GET_D(pos);
							printf("%.6lf\n", d);
						} else if (ta->vt == S) {
							s = GET_S(pos);
							printf("%s\n", s);
						}
					}
				} while (++rep < nreports);
				break;
			}

#if 0

			/* multiple positions per report */

			if (strcmp(subtype, "GTFRI") == 0) {
				int rep, nreports = atoi(parts[6]);	/* "Number" from docs */
				printf("**** special %d **\n", nreports);

				for (rep = 0; rep < nreports; rep++) {
					int pos = (rep * 12) + 13;

					double lon = GET_D((rep * 12) + 11);
					double lat = GET_D((rep * 12) + 12);
					char *utc  = GET_S((rep * 12) + 13);
					//

					printf("** pos=%d utc: %s %.6lf %.6lf\n", pos, utc, lat, lon);
				}

			}
#endif
		}

		splitterfree(typeparts);
		splitterfree(parts);
	}
	return (0);
}
