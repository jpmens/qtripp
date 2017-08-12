#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "util.h"
#include "uthash.h"
#include "conf.h"

#include "models.h"
#include "devices.h"
#include "reports.h"

#define MAXLINELEN	8192

static config cf = {
	.host           = "localhost",
	.port           = 1883,
	.listen_port    = 5004
};

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)

#if 0
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
#endif /* 0 */

int main()
{
	char line[MAXLINELEN], **parts, *typeparts[4];
	int n, nparts;
	char *abr, *subtype;		/* abr= ACK, BUFF, RESP, i.e. the bit before : */
	struct _device *dp;

	if (ini_parse("qtripp.ini", ini_handler, &cf) < 0) {
		fprintf(stderr, "Can't load/parse ini file.\n");
		return (1);
	}

	load_models();
	load_reports();
	load_devices();

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

		struct _report *rp = lookup_reports(subtype);

		printf("%s:%s %s\n", abr, subtype, rp ? rp->desc : "unknown");

		if (strcmp(abr, "ACK") == 0) {
			continue;
		}

		char *protov = GET_S(1);	/* VVJJMM
						 * VV = model
						 * JJ = major
						 * MM = minor 
						 */
		char tmpmodel[3];

		if ((dp = lookup_devices(subtype, protov + 2)) == NULL) {
			printf("MISSING: device definition for %s-%s\n", subtype, protov+2);
			continue;
		}

		tmpmodel[0] = protov[0];
		tmpmodel[1] = protov[1];
		tmpmodel[2] = 0;
		struct _model *model = lookup_models(tmpmodel);
		int rep = 0, nreports = atoi(GET_S(dp->num));	/* "Number" from docs */


		printf("**** model=%s special %d ** (nparts=%d, proto=%s) LINE=%s\n",
			(model) ? model->desc : "unknown", nreports, nparts, protov, line);


		/* handle sub-reports of e.g GTFRI. Even if a subtype
		 * doesn't have sub-reports, we enter this and do it
		 * just once.
		 */

		do {
			double d;
			char *s;

			printf("--> REP==%d dp->num==%d\n", rep, dp->num);

			// for (int slab = 0; slab < __LASTONE; slab++) {
				// int pos = (rep * 12) + -1 + dp->num; /* 12 elements in green area */
				int pos = (rep * 12); /* 12 elements in green area */

				pos = (pos < 0) ? 0 : pos;
				printf("    pos=%5d UTC=%d\n", pos, dp->utc);//  UTC);

				s = GET_S(pos + dp->utc);
				printf("    pos=%5d UTC =[%s]\n", pos + dp->utc, s);

				s = GET_S(pos + dp->lat);
				printf("    pos=%5d LAT =[%s]\n", pos + dp->lat, s);

				/*
				switch (slab) {
					case UTC:
						s = GET_S(pos);
						printf("    pos=%5d UTC =[%s]\n", pos, s);
						break;
					// default: printf("HALP!\n"); break;
				}
				*/
			// }


		} while (++rep < nreports);


#if 0
				do {
					for (ta = &tabs[n].tab[0]; ta->fieldname; ta++) {
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
#endif /* 000 */

		splitterfree(typeparts);
		splitterfree(parts);
	}
	return (0);
}
