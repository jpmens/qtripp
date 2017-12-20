/*
 * qtripp
 * Copyright (C) 2017 Jan-Piet Mens <jp@mens.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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
#ifdef WITH_BEAN
# include "bean.h"
#endif
#include "iinfo.h"

#include "models.h"
#include "devices.h"
#include "reports.h"
#include "ignores.h"

#define MAXLINELEN	(8192 * 2)
#define QOS 		1

struct my_stat {
	char key[24];		/* key: subtype-protov */
	long counter;
	bool ignored;
	UT_hash_handle hh;
};
static struct my_stat *report_stats = NULL;

struct my_imeistat {
	char key[18];
	long reports;
	time_t last_seen;
	char *name;
	char *last_json;	/* JSON string of last published position */
	UT_hash_handle hh;
};
static struct my_imeistat *imei_stats = NULL;

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

static void imei_incr(char *imei, int reports)
{
	struct my_imeistat *is;

	HASH_FIND_STR(imei_stats, imei, is);
	if (!is) {
		is = (struct my_imeistat *)malloc(sizeof(struct my_imeistat));
		strncpy(is->key, imei, 16);
		is->last_seen	= time(0);
		is->reports	= reports;
		HASH_ADD_STR(imei_stats, key, is);
		// printf("---------------------- add %d\n", reports);
	} else {
		is->last_seen	= time(0);
		is->reports	+= reports;
		// printf("---------------------- repl %d\n", reports);
	}
}

/*
 * If last_json is not null, store it for imei, else return it
 */

static char *imei_last_json(char *imei, char *last_json)
{
	struct my_imeistat *is;
	char *lj = NULL;

	HASH_FIND_STR(imei_stats, imei, is);
	if (!is) {
		is = (struct my_imeistat *)malloc(sizeof(struct my_imeistat));
		strncpy(is->key, imei, 16);
		is->last_seen	= time(0);
		is->reports	= 0;
		lj = is->last_json	= (last_json) ? strdup(last_json) : NULL;
		HASH_ADD_STR(imei_stats, key, is);
	} else {
		if (last_json == NULL) {
			lj = is->last_json;
		} else {
			free(is->last_json);
			lj = is->last_json	= strdup(last_json);
		}
	}
	return (lj);
}

static void stat_incr(char *subtype, char *protov, bool ig)
{
	struct my_stat *ms;
	char key[BUFSIZ];

	snprintf(key, sizeof(key), "%s-%s",
		(subtype && *subtype) ? subtype : "unknown",
		(protov  && *protov ) ? protov  : "unknown");

	HASH_FIND_STR(report_stats, key, ms);
	if (!ms) {
		ms = (struct my_stat *)malloc(sizeof(struct my_stat));
		strncpy(ms->key, key, 20);
		ms->counter = 1L;
		ms->ignored = ig;
		HASH_ADD_STR(report_stats, key, ms);
	} else {
		ms->counter++;
		ms->ignored = ig;
	}
}

void print_stats(struct udata *ud)
{
	struct my_stat *ms, *tmp;
	char buf[BUFSIZ];

	HASH_ITER(hh, report_stats, ms, tmp) {
		snprintf(buf, sizeof(buf), "%s %c %ld",
			ms->key,
			ms->ignored ? 'I' : '-',
			ms->counter);
		xlog(ud, "stats: %s\n", buf);
		if (ud->cf->reporttopic)
			pub(ud, (char *)ud->cf->reporttopic, buf, false);
	}

	/* FIXME: consider deleting keys when they've been listed? */
}

void dump_stats(struct udata *ud)
{
	char path[BUFSIZ];
	FILE *fp;

	if (!ud->cf->dumpdir) {
		xlog(ud, "Cannot dump_stats because dumpdir not configured\n");
		return;
	}
	snprintf(path, sizeof(path), "%s/stats.json", ud->cf->dumpdir);

	if ((fp = fopen(path, "w")) != NULL) {
		JsonNode *obj = json_mkobject(), *o;
		struct my_stat *ms, *tmp;
		char *js;

		HASH_ITER(hh, report_stats, ms, tmp) {
			o = json_mkobject();
			json_append_member(o, "counter", json_mknumber(ms->counter));
			json_append_member(o, "ignored", json_mkbool(ms->ignored));

			json_append_member(obj, ms->key, o);
		}

		if ((js = json_stringify(obj, "  ")) != NULL) {
			fprintf(fp, "%s\n", js);
			free(js);
		}
		json_delete(obj);
		fclose(fp);
	}

	/* IMEI_STATS */

	snprintf(path, sizeof(path), "%s/imei.json", ud->cf->dumpdir);

	if ((fp = fopen(path, "w")) != NULL) {
		JsonNode *obj = json_mkobject(), *o;
		struct my_imeistat *is, *tmp;
		char *js;

		HASH_ITER(hh, imei_stats, is, tmp) {
			o = json_mkobject();
			json_append_member(o, "last_seen", json_mknumber(is->last_seen));
			json_append_member(o, "reports", json_mknumber(is->reports));

			json_append_member(obj, is->key, o);
		}

		if ((js = json_stringify(obj, "  ")) != NULL) {
			fprintf(fp, "%s\n", js);
			free(js);
		}
		json_delete(obj);
		fclose(fp);
	}
	xlog(ud, "Statistics dumped.\n");
}


/*
 * The JSON object we obtained from the tracker is complete and
 * can be published. Check if we have extra JSON stuff we want
 * to add to it.
 */

// #include "mongoose.h"	// FIXME: remove
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
		JsonNode *j;

		xlog(ud, "PUBLISH: %s %s\n", topic, js);
		pub(ud, topic, js, true);

		/* We have a full JSON string. Is this a type location? If so, store it
		 * in IMEI cache for using later on upon heartbeat (HBD).
		 */

		if ((j = json_find_member(obj, "_type")) != NULL) {
			if (j->tag == JSON_STRING && !strcmp(j->string_, "location")) {
				imei_last_json(imei, js);
			}
		}

//		if (ud->cocorun) mg_printf(ud->coco, "%s", js);	// FIXME remove
//		fprintf(stderr, "@@@@@@@@@ %d\n", ud->coco->sock);

		free(js);
	}
}

/*
 * A connection has been closed, probably by a device. Publish a
 * pseudo LWT for this device. (Pseudo b/c we have the central
 * connection to a broker and are going to pretend the device
 * actually has that.)
 */

void pseudo_lwt(struct udata *ud, char *imei)
{
	JsonNode *o = json_mkobject();

	if (!imei || !*imei)
		return;

	json_append_member(o, "_type", json_mkstring("lwt"));
	json_append_member(o, "imei", json_mkstring(imei));
	json_append_member(o, "tst", json_mknumber(time(0)));

	transmit_json(ud, imei, o);
#ifdef WITH_BEAN
	bean_put(ud, o);
#endif
	json_delete(o);
}

#define GET_D(n)	((n <= nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n <= nparts && *parts[n]) ? parts[n] : NULL)

/*
 * `line' contains a line of text from a tracker. Do what is necessary,
 * and return the IMEI string if there is one.
 */

static long linecounter = 0L;

/*
 * If anything is to be written back to the device, do so by
 * allocating a string and putting it at `*response'; the caller
 * will write to device and free the string.
 */

char *handle_report(struct udata *ud, char *line, char **response)
{
	char **parts, *tparts[4], *imei_dup = NULL;
	int n, nparts;
	char abr[24], subtype[24];	/* abr= ACK, BUFF, RESP, i.e. the bit before : */
	struct _device *dp;
	struct _ignore *ip;
	bool subtype_ignored = false;

	++linecounter;

	if (*line == '*') {
		// xlog(ud, "Control: %s\n", line);

		if (!strncmp(line, "*PING", 5)) {
			*response = strdup("*PONG");
		}
		return (NULL);
	}

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

	struct _report *rp = lookup_reports(subtype);
	if ((ip = lookup_ignores(subtype)) != NULL) {
		subtype_ignored = true;
	}
	stat_incr(subtype, protov, subtype_ignored);

	if (subtype_ignored) {
		xlog(ud, "Ignoring %s: %s (%s)\n",
			(*subtype) ? subtype : "<nil>",
			(ip->reason && *ip->reason) ? ip->reason : "<nil>",
			(rp && rp->desc && *rp->desc) ? rp->desc : "unknown report type");
		xlog(ud, "+++ I=%s Ignored LINE=%s\n", imei, line);
		goto finish;
	}

	imei_dup = strdup(imei);

	struct _model *model = lookup_models(protov);
	struct _iinfo *iinfo = lookup_iinfo(ud->cf->namesdir, imei);

	xlog(ud, "+++ I=%s (%s) M=%s np=%d P=%s C=%ld T=%s:%s (%s) LINE=%s\n",
		imei,
		(iinfo) ? iinfo->name : ".",
		(model) ? model->desc : "unknown",
		nparts, protov,
		linecounter,
		abr, subtype, (rp ? rp->desc : "unknown"),
		line);


	if (strcmp(abr, "ACK") == 0) {
		char rr[BUFSIZ];

		if (!strcmp(subtype, "GTHBD")) {
			char *js;

			snprintf(rr, sizeof(rr), "+SACK:GTHBD,,%s$", GET_S(5) ? GET_S(5) : "0000");
			*response = strdup(rr);

			/* If we have a "last_json" for this IMEI, obtain it from cache and
			 * modify it to replace "t:" with a "p:ing" and update the tst to
			 * now(). Publish to MQTT in order to pass the heartbeat along to MQTT
			 * with the last-known good values.
			 */

			if ((js = imei_last_json(imei, NULL)) != NULL) {
				JsonNode *obj, *j;

				if ((obj = json_decode(js)) != NULL) {
					if ((j = json_find_member(obj, "t")) != NULL) {
						json_remove_from_parent(j);
						json_append_member(obj, "t", json_mkstring("p"));
					}
					if ((j = json_find_member(obj, "tst")) != NULL) {
						j->number_ = time(0);
						transmit_json(ud, imei, obj);
					}
					json_delete(obj);
				}
			}
		}
		goto finish;
	}


	if ((dp = lookup_devices(subtype, protov + 2)) == NULL) {
		xlog(ud, "MISSING: device definition for %s-%s\n", subtype, protov+2);
		goto finish;
	}

	char *dpn = GET_S(dp->num);
	int nreports = atoi(dpn ? dpn : "0");	/* "Number" from docs */

	imei_incr(imei, nreports);
	xlog(ud, "++. N=%d\n", nreports);

	if (ud->debugging) {
		for (n = 0; parts[n]; n++) {
			xlog(ud, "\t%2d %s\n", n, parts[n]);
		}
	}

	JsonNode *jmerge = json_mkobject(), *jm;

	/* "add" is additional data, e.g. GTSWG */
	if (dp->add > 0) {
		double d = GET_D(dp->add);
		if (!isnan(d)) {
			json_append_member(jmerge, "add", json_mknumber(d));
		}
	}

	if (!strcmp(subtype, "GTERI")) {
		/*
		 * According to GV65, page 35, et.al (search for "Eri mask")
		 * if bit 1 is set in the mask, it means AC100 data is
		 * available (which I interpret to be 1-Wire data).
		 *
		 * So, we check for that bit and then grab the temperature
		 * from our 1-Wire DS18B20.
		 */

		char *erimask = GET_S(4);
		unsigned long eribits = strtoul(erimask ? erimask : "0", NULL, 16);

		// FIXME: this is not accurate; bits are set if temp sensor not connected
		// what seems to be ok is check if field "UART Device Type" is 0 which means
		// "no device connected" (if DS18B20 is connected the field is "1")
		//
		if (eribits & 0x0002) {
#define D_UART (6)
			int offset = ((nreports - 1) * 12) + dp->odometer + D_UART;
			char *val = GET_S(offset);
			int uart_type  = atoi(val && *val ? val : "0");
			xlog(ud, "UART Type: %d: %s temperature\n",
					uart_type, (uart_type) ? "Use" : "Skip");

			if (uart_type != 0) {
				char *t = GET_S(offset + 3);

				if (t && *t) {
					json_append_member(jmerge, "temp_c", json_mknumber(temp(t)));
				}
			}
		}
	}

	/*
	 * Some reports contain "blocks" of data which can repeat (GTFI, GTERI
	 * as shown below), and we want some of the data which is *behind* the
	 * repetitions. Grab it now.
	 */

	if (nreports > 0) {
		if (dp->odometer > 0) {
			double odo = GET_D(((nreports - 1) * 12) + dp->odometer);
			if (!isnan(odo)) {
				json_append_member(jmerge, "odometer", json_mkdouble(odo, 1));
			}
		}
		if (dp->batt > 0) {
			double d = GET_D(((nreports - 1) * 12) + dp->batt);
			if (!isnan(d)) {
				json_append_member(jmerge, "batt", json_mknumber(d));
			}
		}
	}

	/* handle sub-reports of e.g GTFRI / GTERI. Even if a subtype
	 * doesn't have sub-reports, we enter this and do it
	 * just once.
	 */

	double lastlat = NAN, lastlon = NAN;
	int rep = 0;
	do {
		double lat, lon;
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
		json_append_member(obj, "lat", json_mkdouble(lat, 6));
		json_append_member(obj, "lon", json_mkdouble(lon, 6));

		if ((s = GET_S(pos + dp->utc)) != NULL) {
			time_t epoch;

			if (str_time_to_secs(s, &epoch) != 1) {
				xlog(ud, "Cannot convert time from [%s]\n", s);
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

		json_append_member(obj, "t", json_mkstring(subtype));

		if (!isnan(lastlat)) {
			double meters = haversine_dist(lastlat, lastlon, lat, lon);

			// FIXME: Experimental if we haven't moved, skip the publish

			if (meters < 0.1) {
				lastlat = lat;
				lastlon = lon;
				json_delete(obj);
				xlog(ud, "Dropping segment %2d/%d because %.2lf distance covered\n",
					rep + 1, nreports, meters);
				continue;
			}

			json_append_member(obj, "meters", json_mkdouble(meters, 1));
		}
		lastlat = lat;
		lastlon = lon;

		/* Add the merge if we have any */
		json_foreach(jm, jmerge) {
			if (jm->tag == JSON_STRING)
				json_append_member(obj, jm->key, json_mkstring(jm->string_));
			else if (jm->tag == JSON_NUMBER)
				json_append_member(obj, jm->key, json_mknumber(jm->number_));
			else if (jm->tag == JSON_DOUBLE)
				json_append_member(obj, jm->key, json_mkdouble(jm->number_, jm->width));
			else if (jm->tag == JSON_BOOL)
				json_append_member(obj, jm->key, json_mkbool(jm->bool_));
		}

		transmit_json(ud, imei, obj);


#ifdef WITH_BEAN
		json_append_member(obj, "imei", json_mkstring(imei));
		json_append_member(obj, "raw_line", json_mkstring(line));
		bean_put(ud, obj);
#endif
		json_delete(obj);

	} while (++rep < nreports);

	if (jmerge != NULL) {
		json_delete(jmerge);
	}

  finish:
	splitterfree(parts);
	return (imei_dup);
}

int handle_file_reports(struct udata *ud, FILE *fp)
{
	char line[MAXLINELEN];
	char *response = NULL;

	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		if (*line == '#' || *line == '\n')
			continue;

		line[strcspn(line, "\r\n")] = '\0';

		handle_report(ud, line, &response);
	}
	return (0);
}
