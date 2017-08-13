#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <mosquitto.h>
#include "util.h"
#include "uthash.h"
#include "conf.h"
#include "udata.h"
#include "tline.h"

#include "models.h"
#include "devices.h"
#include "reports.h"
#include "ignores.h"

#define MAXLINELEN	(8192 * 2)
#define QOS 		1

void pub(struct udata *ud, char *topic, char *payload, bool retain)
{
	int rc;

	rc = mosquitto_publish(ud->mosq, NULL, topic, strlen(payload), payload, QOS, retain);
	if (rc) {
		xlog(ud, "Publish failed: rc=%d...\n", rc);
		if (rc == MOSQ_ERR_NO_CONN) {
			mosquitto_reconnect(ud->mosq);
		}
	}
}

/*
 * The JSON object we obtained from the tracker is complete and
 * can be published. Check if we have extra JSON stuff we want
 * to add to it.
 */

void transmit_json(struct udata *ud, char *imei, JsonNode *obj)
{
	JsonNode *e, *extra;
	char *js;
	char *topic;

	topic = device_to_topic(ud->cf, imei);

	if ((extra = extra_json(ud->cf, imei)) != NULL) {
		json_foreach(e, extra) {
			if (e->tag == JSON_STRING)
				json_append_member(obj, e->key, json_mkstring(e->string_));
			else if (e->tag == JSON_NUMBER)
				json_append_member(obj, e->key, json_mknumber(e->number_));
			else if (e->tag == JSON_BOOL)
				json_append_member(obj, e->key, json_mkbool(e->bool_));
		}
	}

	if ((js = json_encode(obj)) != NULL) {
		xlog(ud, "PUBLISH: %s %s\n", topic, js);
		pub(ud, topic, js, true);
		free(js);
	}
}

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)


/*
 * `line' contains a line of text from a tracker. Do what is necessary,
 * and return the IMEI string if there is one.
 */

static long linecounter = 0L;

char *handle_report(struct udata *ud, char *line)
{
	char **parts, *tparts[4], *imei_dup = NULL;
	int n, nparts;
	char abr[24], subtype[24];	/* abr= ACK, BUFF, RESP, i.e. the bit before : */
	struct _device *dp;
	struct _ignore *ip;

	++linecounter;

	if ((parts = clean_split(ud, line, &nparts)) == NULL) {
		xlog(ud, "Cannot split line from csv: %s\n", line);
		return (NULL);
	}

	/*
	 * parts[0] contains "RESP:GTFRI". Point `abr' to the initial
	 * portion and subtype to the second; chop at the ':'
	 */
	if ((n = splitter(parts[0], ":", tparts)) != 2) {
		xlog(ud, "Cannot split type from parts[0]\n");
		goto finish;
	}
	strcpy(abr, tparts[0]);
	strcpy(subtype, tparts[1]);
	splitterfree(tparts);


	if ((ip = lookup_ignores(subtype)) != NULL) {
		xlog(ud, "Ignoring %s because %s\n", subtype, ip->reason);
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
		goto finish;
	}

	imei_dup = strdup(imei);

	struct _model *model = lookup_models(protov);
	struct _report *rp = lookup_reports(subtype);

	xlog(ud, "+++ I=%s M=%s np=%d P=%s C=%ld T=%s:%s (%s) LINE=%s\n",
		imei,
		(model) ? model->desc : "unknown",
		nparts, protov,
		linecounter,
		abr, subtype, (rp ? rp->desc : "unknown"),
		line);


	if (strcmp(abr, "ACK") == 0) {
		goto finish;
	}


	if ((dp = lookup_devices(subtype, protov + 2)) == NULL) {
		xlog(ud, "MISSING: device definition for %s-%s\n", subtype, protov+2);
		goto finish;
	}

	int rep = 0, nreports = atoi(GET_S(dp->num));	/* "Number" from docs */

	xlog(ud, "++. N=%d\n", nreports);

	if (ud->debugging) {
		for (n = 0; parts[n]; n++) {
			xlog(ud, "\t%2d %s\n", n, parts[n]);
		}
	}


	/* handle sub-reports of e.g GTFRI. Even if a subtype
	 * doesn't have sub-reports, we enter this and do it
	 * just once.
	 */

	do {
		double lat, lon, d;
		char *s;
		JsonNode *obj;

		// xlog(ud, "--> REP==%d dp->num==%d\n", rep, dp->num);

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

		transmit_json(ud, imei, obj);
		json_delete(obj);

	} while (++rep < nreports);

  finish:
	splitterfree(parts);
	return (imei_dup);
}

int handle_file_reports(struct udata *ud, FILE *fp)
{
	char line[MAXLINELEN];

	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		if (*line == '#' || *line == '\n')
			continue;

		handle_report(ud, line);
	}
	return (0);
}
