#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "util.h"
#include "uthash.h"
#include "json.h"
#include "conf.h"
#include "udata.h"

#include "models.h"
#include "devices.h"
#include "reports.h"
#include "ignores.h"

#define MAXLINELEN	(8192 * 2)

static config cf = {
	.host           = "localhost",
	.port           = 1883,
	.listen_port    = 5004
};

void process(struct udata *ud, char *imei, JsonNode *obj)
{
	char *js;
	char topic[BUFSIZ];

	snprintf(topic, sizeof(topic), "owntracks/qtripp/%s", imei);

	if ((js = json_encode(obj)) != NULL) {
		printf("%s %s\n", topic, js);
		free(js);
	}
}

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)

int main()
{
	char line[MAXLINELEN], **parts;
	int n, nparts;
	char *ap, *abr, *subtype;		/* abr= ACK, BUFF, RESP, i.e. the bit before : */
	struct _device *dp;
	struct _ignore *ip;
	long linecounter = 0L;
	struct udata udata, *ud = &udata;

	ud->debugging		= false;

	if (ini_parse("qtripp.ini", ini_handler, &cf) < 0) {
		fprintf(stderr, "Can't load/parse ini file.\n");
		return (1);
	}

	load_models();
	load_reports();
	load_devices();
	load_ignores();

	while (fgets(line, sizeof(line) - 1, stdin) != NULL) {
		++linecounter;
		if (*line == '#')
			continue;

		if ((parts = clean_split(line, &nparts)) == NULL) {
			printf("Can't split\n");
			continue;
		}

		/*
		 * parts[0] contains "RESP:GTFRI". Point `abr' to the initial
		 * portion and subtype to the second; chop at the ':'
		 */

		if ((ap = strchr(abr = parts[0], ':')) != NULL) {
			*ap = 0;
			subtype = ++ap;
		}


		if ((ip = lookup_ignores(subtype)) != NULL) {
			printf("Ignoring %s because %s\n", subtype, ip->reason);
			goto finish;
		}

		if (ud->debugging) {
			for (n = 0; parts[n]; n++) {
				debug(ud, "\t%2d %s\n", n, parts[n]);
			}
		}

		if (strcmp(abr, "ACK") == 0) {
			goto finish;
		}

		char *imei = GET_S(2);
		char *protov = GET_S(1);	/* VVJJMM
						 * VV = model
						 * JJ = major
						 * MM = minor 
						 */
		/*
		 * If we have neither IMEI nor protov forget the rest; impossible to
		 * handle.
		 */

		if (!imei || !*imei || !protov || !*protov) {
			continue;
		}

		if ((dp = lookup_devices(subtype, protov + 2)) == NULL) {
			debug(ud, "MISSING: device definition for %s-%s\n", subtype, protov+2);
			goto finish;
		}

		struct _model *model = lookup_models(protov);
		int rep = 0, nreports = atoi(GET_S(dp->num));	/* "Number" from docs */

		struct _report *rp = lookup_reports(subtype);

		debug(ud, "+++ I=%s M=%s N=%d np=%d P=%s C=%ld T=%s:%s (%s) LINE=%s\n",
			imei,
			(model) ? model->desc : "unknown",
			nreports, nparts, protov,
			linecounter,
			abr, subtype, (rp ? rp->desc : "unknown"),
			line);


		/* handle sub-reports of e.g GTFRI. Even if a subtype
		 * doesn't have sub-reports, we enter this and do it
		 * just once.
		 */

		do {
			double lat, lon, d;
			char *s;
			JsonNode *obj;

			debug(ud, "--> REP==%d dp->num==%d\n", rep, dp->num);

			int pos = (rep * 12); /* 12 elements in green area */

			if ((s = GET_S(pos + dp->utc)) == NULL) {
				/* no fix */
				continue;
			}

			// printf("    pos=%5d UTC =[%s]\n", pos + dp->utc, s);
			// s = GET_S(pos + dp->lat);
			// printf("    pos=%5d LAT =[%s]\n", pos + dp->lat, s);

			lat = GET_D(pos + dp->lat);
			lon = GET_D(pos + dp->lon);

			if (isnan(lat) || isnan(lon)) {
				continue;
			}

			obj = json_mkobject();
			json_append_member(obj, "lat", json_mknumber(lat));
			json_append_member(obj, "lon", json_mknumber(lon));

			if (dp->dist > 0) {
				d = GET_D(pos + dp->dist);
				if (!isnan(d)) {
					json_append_member(obj, "dist", json_mknumber(d));
				}
			}

			if ((s = GET_S(pos + dp->utc)) != NULL) {
				time_t epoch;

				if (str_time_to_secs(s, &epoch) != 1) {
					printf("Cannot convert time\n");
					continue;
				}
				json_append_member(obj, "tst", json_mknumber(epoch));
			}

			json_append_member(obj, "_type", json_mkstring("location"));

			if ((s = GET_S(pos + dp->acc)) != NULL) {
				json_append_member(obj, "acc", json_mknumber(atoi(s)));
			}

			if ((s = GET_S(pos + dp->vel)) != NULL) {
				json_append_member(obj, "vel", json_mknumber(atoi(s)));
			}

			if ((s = GET_S(pos + dp->alt)) != NULL) {
				json_append_member(obj, "alt", json_mknumber(atoi(s)));
			}

			if ((s = GET_S(pos + dp->cog)) != NULL) {
				json_append_member(obj, "cog", json_mknumber(atoi(s)));
			}

			process(ud, imei, obj);
			json_delete(obj);

		} while (++rep < nreports);

	  finish:
		splitterfree(parts);
	}
	return (0);
}
