#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "util.h"
#include "uthash.h"
#include "ini.h"        /* https://github.com/benhoyt/inih */

#define MAXLINELEN	8192

typedef struct config {
        const char *host;
        int port;
        struct my_model {
		char *type;              /* key, type*/
		char *model;
		UT_hash_handle hh;
	} *models;
        const char *listen_port;
} config;

static config cf = {
	.host           = "localhost",
	.port           = 1883
};

#define _eq(n) (strcmp(key, n) == 0)
static int ini_handler(void *cf, const char *section, const char *key, const char *val)
{
	config *c = (config *)cf;

	printf("section=[%s]  >%s<-->%s\n", section, key, val);


        if (!strcmp(section, "models")) {
                /*
                 *      [models]
                 *      [models]
		 *	31: GV65,
		 *	36: GV500
                 */

                struct my_model *d = (struct my_model *)malloc(sizeof (struct my_model));
                d->type = strdup(key);
                d->model = strdup(val);
                HASH_ADD_KEYPTR(hh, c->models, d->type, strlen(d->type), d);


        }

	return (1);
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

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)

typedef enum vtype { D, S } vtype;

static struct table {
	char *subtype;
	int multi;		/* Has multiple reports (e.g. GTFRI); 0=no, >0 field number */
	struct tab {
		char *fieldname;
		vtype vt;
		int slab;
	} tab[10];
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

/* protov is VVJJMM; use VV. */
static char *protov_to_model(char *protov)
{
	struct my_model *mo;
	char dev[3];

	dev[0] = protov[0];
	dev[1] = protov[1];
	dev[2] = 0;

	HASH_FIND_STR(cf.models, dev,  mo);
	return(mo->model);
}

int main()
{
	char line[MAXLINELEN], **parts, *typeparts[4];
	int n, nparts;
	char *abr, *subtype;		/* abr= ACK, BUFF, RESP, i.e. the bit before : */
	// JsonNode *device_models;
	// device_models = load_types("device-types.json");

	if (ini_parse("qtripp.ini", ini_handler, &cf) < 0) {
		fprintf(stderr, "Can't load/parse ini file.\n");
		return (1);
	}


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
				char *device_model = protov_to_model(protov);

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
		}

		splitterfree(typeparts);
		splitterfree(parts);
	}
	return (0);
}
