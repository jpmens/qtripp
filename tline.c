/*
 * qtripp
 * Copyright (C) 2017-2020 Jan-Piet Mens <jp@mens.de>
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
#include <assert.h>
#include "util.h"
#include "uthash.h"
#include "conf.h"
#include "udata.h"
#include "tline.h"
#include "constfile.h"
#ifdef WITH_BEAN
# include "bean.h"
#endif

#include "models.h"
#include "devices.h"
#include "reports.h"
#include "ignores.h"

#define MAXLINELEN	(8192 * 2)
#define QOS 		2
#define NAGIOSREPORT	"nagios/qtripp"

/*
 * DBGOUT != 0 means print each line
 * DBGOUT == 1 means print "sent"
 * DBGOUT == 2 means print "devs"
 * DBGOUT == 3 means print "GTHBD"
 */
#define DBGOUT 0
#define DLOG(lev, fmt, ...)  if ((lev) == DBGOUT) fprintf(stderr, fmt, __VA_ARGS__)

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
	double lat, lon, vel;
	long cog;
	time_t tst;
	bool validpos;
	UT_hash_handle hh;
};
static struct my_imeistat *imei_stats = NULL;

void pub(struct udata *ud, char *topic, char *payload, bool retain)
{
	int rc;

	rc = mosquitto_publish(ud->mosq, NULL, topic, strlen(payload), payload, QOS, retain);
	if (rc) {
		xlog(ud, "Publish failed: rc=%d...\n", rc);
#if 1
		if (rc == MOSQ_ERR_NO_CONN) {
			mosquitto_reconnect(ud->mosq);
		}
#endif
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
		is->validpos	= false;
		is->lat		= 0.0L;
		is->lon		= 0.0L;
		is->vel		= 0.0L;
		is->tst		= time(0);
		HASH_ADD_STR(imei_stats, key, is);
	} else {
		is->last_seen	= time(0);
		is->reports	+= reports;
	}
}

static bool imei_last_position(char *imei, double *lat, double *lon, time_t *tst, double *vel, long *cog, bool set)
{
	struct my_imeistat *is;
	bool rc = false;

	HASH_FIND_STR(imei_stats, imei, is);
	if (!is) {
		is = (struct my_imeistat *)malloc(sizeof(struct my_imeistat));
		strncpy(is->key, imei, 16);
		is->last_seen	= time(0);
		is->reports	= 0;
		is->validpos	= false;

		if (set) {
			is->validpos	= true;
			is->lat		= *lat;
			is->lon		= *lon;
			is->vel		= *vel;
			is->cog		= *cog;
			is->tst		= *tst;
		}
		HASH_ADD_STR(imei_stats, key, is);
	} else {
		if (set) {
			is->validpos	= true;
			is->lat		= *lat;
			is->lon		= *lon;
			is->vel		= *vel;
			is->cog		= *cog;
			is->tst		= *tst;
		}
		*lat 	= is->lat;
		*lon	= is->lon;
		*vel	= is->vel;
		*cog	= is->cog;
		*tst	= is->tst;
		if (is->validpos)
			rc = true;
	}
	return (rc);
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

void pong(struct udata *ud)
{
	STATSD_INC(ud->cf->sd, "pong");
	pub(ud, NAGIOSREPORT, "pong", false);
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

void transmit_json(struct udata *ud, char *imei, JsonNode *obj, bool retain)
{
	JsonNode *e, *extra;
	char *js;
	char *topic;

	topic = device_to_topic(ud->cf, imei);

	if ((extra = extra_json(ud->ef, imei)) != NULL) {
		json_foreach(e, extra) {
			JsonNode *j;

			/* The object we're going to transmit _might_ already have
			 * these extra variables; remove the elements before
			 * adding them in again.
			 */

			if ((j = json_find_member(obj, e->key)) != NULL) {
				json_remove_from_parent(j);
			}

			if (e->tag == JSON_STRING)
				json_append_member(obj, e->key, json_mkstring(e->string_));
			else if (e->tag == JSON_NUMBER)
				json_append_member(obj, e->key, json_mknumber(e->number_));
			else if (e->tag == JSON_BOOL)
				json_append_member(obj, e->key, json_mkbool(e->bool_));
		}
	}

	/*
	 * Ensure TID is a string for Traccar owntracks decoder; we had a case
	 * of tid being a number (53). Also forget about checking for number and
	 * formatting that; we just add last two chars of IMEI as tid.
	 */

	if ((e = json_find_member(obj, "tid")) != NULL) {
		if (e->tag != JSON_STRING) {
			json_remove_from_parent(e);
			json_append_member(obj, "tid", json_mkstring(imei + (strlen(imei) - 2)));
		}
	}

	if ((js = json_encode(obj)) != NULL) {
		xlog(ud, "PUBLISH: %s %s\n", topic, js);
		STATSD_INC(ud->cf->sd, "mqtt.message.publish");
		pub(ud, topic, js, retain);

		free(js);
	}

	if (extra != NULL)
		json_delete(extra);

}

/*
 * Find `elem' in the JSON `obj'. In particular find NUMBERs and convert
 * to our special JSON DOUBLE type with the specified precision.
 */

void precision(JsonNode *obj, char *elem, int prec)
{
	JsonNode *j;
	double d;

	if ((j = json_find_member(obj, elem)) != NULL) {
		assert(j->tag == JSON_NUMBER || j->tag == JSON_DOUBLE);
		d = j->number_;
		json_remove_from_parent(j);
		json_append_member(obj, elem, json_mkdouble(d, prec));
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

	STATSD_INC(ud->cf->sd, "mqtt.message.lwt");

	json_append_member(o, "_type", json_mkstring("lwt"));
	json_append_member(o, "imei", json_mkstring(imei));
	json_append_member(o, "tst", json_mknumber(time(0)));

	transmit_json(ud, imei, o, false);
#ifdef WITH_BEAN
	bean_put(ud, o);
#endif
	json_delete(o);
}

#define GET_D(n)	((n > 0 && n < nparts && *parts[n]) ? atof(parts[n]) : NAN)
#define GET_S(n)	((n > 0 && n < nparts && *parts[n]) ? parts[n] : NULL)

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
        char id[64];
	struct _device *dp;
	struct _ignore *ip;
	bool subtype_ignored = false;
	char cbuf[BUFSIZ], *cname;

	STATSD_INC(ud->cf->sd, "reports");

	++linecounter;

#if DBGOUT != 0
	fprintf(stderr, "DEBUG line #%ld (%lu) %s\n",
		linecounter, line != NULL ? strlen(line) : 0, line != NULL ? line : "NULL");
#endif
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
		if (n != -1)
			splitterfree(tparts);
		goto finish;
	}
	strcpy(abr, tparts[0]);
	strcpy(subtype, tparts[1]);
	splitterfree(tparts);


	char *imei = GET_S(2);

	// the protocol version consists of a model identifier (2 or 6 bytes) plus 4 bytes of version string
	// e.g. 380D01 for GV65Plus (x38) version 13.1
	// e.g. 8020030100 for GV58CEU (x802003) version 1.0
	char *protov = GET_S(1);	/* MOMAMI or MMMOOOMAMI
					 * MO = model 2 hex digits
					 * MMMOOO = model 6 hex digits
					 * MA = major 2 hex digits
					 * MI = minor 2 hex digits
					 */
	/*
	 * If we have neither IMEI nor protov forget the rest; impossible to
	 * handle.
	 */

	if (!imei || !*imei || !protov || !*protov) {
		STATSD_INC(ud->cf->sd, "reports.bad");
		goto finish;
	}

	struct _report *rp = lookup_reports(subtype);
	if ((ip = lookup_ignores(subtype)) != NULL) {
		subtype_ignored = true;
	}
	stat_incr(subtype, protov, subtype_ignored);

	if (subtype_ignored) {
		STATSD_INC(ud->cf->sd, "reports.subtype.ignored");
		xlog(ud, "Ignoring %s: %s (%s)\n",
			(*subtype) ? subtype : "<nil>",
			(ip->reason && *ip->reason) ? ip->reason : "<nil>",
			(rp && rp->desc && *rp->desc) ? rp->desc : "unknown report type");
		xlog(ud, "+++ I=%s Ignored LINE=%s\n", imei, line);
		goto finish;
	}

	imei_dup = strdup(imei);

	struct _model *model = lookup_models(protov);

	cname = constfile_stab(ud->ef, imei, cbuf, sizeof(cbuf));

	xlog(ud, "+++ I=%s (%s) M=%s np=%d P=%s C=%ld T=%s:%s (%s) LINE=%s\n",
		imei,
		(cname) ? cname : ".",
		(model) ? model->desc : "unknown",
		nparts, protov,
		linecounter,
		abr, subtype, (rp ? rp->desc : "unknown"),
		line);


	if (strcmp(abr, "ACK") == 0) {
		char rr[BUFSIZ];

		if (!strcmp(subtype, "GTHBD")) {
			double last_lat, last_lon, last_vel;
			long last_cog;
			time_t last_tst;

			STATSD_INC(ud->cf->sd, "reports.gthbd");

			snprintf(rr, sizeof(rr), "+SACK:GTHBD,,%s$", GET_S(5) ? GET_S(5) : "0000");
			*response = strdup(rr);

			/*
			 * If we have a last valid lat/lon, we create a small "p"ing type
			 * publish with our last valid position and the timestamp from the
			 * heartbeat, adding "sent" as the current time(0) to identify
			 * when this ping originated.
			 */

			if (imei_last_position(imei, &last_lat, &last_lon, &last_tst, &last_vel, &last_cog, false) == true) {
				JsonNode *obj = json_mkobject();
				json_append_member(obj, "_type", json_mkstring("location"));
				json_append_member(obj, "lat", json_mkdouble(last_lat, 6));
				json_append_member(obj, "lon", json_mkdouble(last_lon, 6));
				json_append_member(obj, "vel", json_mkdouble(last_vel, 1));
				json_append_member(obj, "cog", json_mknumber(last_cog));
				json_append_member(obj, "tst", json_mknumber(last_tst));
				json_append_member(obj, "sent", json_mknumber(time(0)));
				json_append_member(obj, "t", json_mkstring("p"));

				transmit_json(ud, imei, obj, true);
				json_delete(obj);
			}
		}
		goto finish;
	}


	/* 
	 * lookup device definition
	 */
	//fprintf(stderr, "lookup_devices %s %s\n", subtype, protov ? protov : "NULL");
	if ((dp = lookup_devices(subtype, protov)) == NULL) {
		xlog(ud, "MISSING: device definition for %s-%s\n", subtype, protov);
		goto finish;
	}

        snprintf(id, sizeof(id), "%s-%s", subtype, protov);
	if (strcmp(dp->id, id)) {
		xlog(ud, "DEFAULT: device definition for %s used %s\n", id, dp->id);
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

	/* "vin" is the optional vehicle identification number */
	if (dp->vin > 0) {
		char *vin = GET_S(dp->vin);
		if (vin != NULL) {
			json_append_member(jmerge, "vin", json_mkstring(vin));
		}
	}

	/* "name" is the optional device name */
	if (dp->name > 0) {
		char *name = GET_S(dp->name);
		if (name != NULL) {
			json_append_member(jmerge, "name", json_mkstring(name));
		}
	}

	/* "rit" is Record ID and Report Type combined */
	double rit = 0.0;
        int rid = 0;
	int rty = 0;

	if (dp->rid > 0) {
		double frid = GET_D(dp->rid);
		if (!isnan(frid)) {
			rid = frid;
			json_append_member(jmerge, "rid", json_mknumber(rid));
		}
	}

	if (dp->rty > 0) {
		double frty = GET_D(dp->rty);
		if (!isnan(frty)) {
			rty = frty;
			json_append_member(jmerge, "rty", json_mknumber(rty));
		}
	}

	if (dp->rit > 0) {
		rit = GET_D(dp->rit);
		if (!isnan(rit)) {
			json_append_member(jmerge, "rit", json_mknumber(rit));
			/* determine "t" from subtype, report id and report type */
			rid = floor(rit / 10.0);
			json_append_member(jmerge, "rid", json_mknumber(rid));
			rty = fmod(rit, 10.0);
			json_append_member(jmerge, "rty", json_mknumber(rty));
		}
	}

	/* "mst" is the motion state*/
	int mst = 0;

	if (dp->mst > 0) {
		mst = floor(GET_D(dp->mst));
		json_append_member(jmerge, "mst", json_mknumber(mst));
	}

	/* io status (ios) is present if indicated for one protocol version */
	bool iospresent = true;
	if (!strcmp(subtype, "GTFRI") && (!strcmp(protov, "300800") || !strcmp(protov, "301500"))) {
		if ((rid & 0x01) == 0) {
			iospresent = false;
		}
	}

        /* some messages have an erim state. if so, two bits indicate the presence of other optional parts */
	bool ac100present = true;
	int ac100number = 1;
	bool canpresent = true;
	bool xyzpresent = false;
	int xyznumber = 1;
	if (dp->erim > 0) {
		char *erimstring = GET_S(dp->erim);
		if (erimstring != NULL) {
			unsigned long erim = strtoul(erimstring, NULL, 16);
			//fprintf(stderr, "erim string %s long %08lx\n", erimstring, erim);
			ac100present = ((erim & 0x02) != 0);
			//fprintf(stderr, "ac100present bool %d\n", ac100present);
			canpresent = ((erim & 0x04) != 0);
			//fprintf(stderr, "canpresent bool %d\n", canpresent);
			xyzpresent = ((erim & 0x10) != 0);
			//fprintf(stderr, "xyzpresent bool %d\n", xyzpresent);
		}
	}

	/* "ubatt" is battery voltage in V */
	if (dp->ubatt > 0) {
		double ubatt = GET_D(dp->ubatt);
		if (!isnan(ubatt)) {
			json_append_member(jmerge, "ubatt", json_mkdouble(ubatt, 1));
		}
	}

	/* "don" is duration since ignition on in seconds*/
	if (dp->don > 0) {
		double don = GET_D(dp->don);
		if (!isnan(don)) {
			json_append_member(jmerge, "don", json_mknumber(don));
		}
	}

	/* "doff" is duration since ignition off in seconds */
	if (dp->doff > 0) {
		double doff = GET_D(dp->doff);
		if (!isnan(doff)) {
			json_append_member(jmerge, "doff", json_mknumber(doff));
		}
	}

	/* "uext" is external power voltage in mV */
	if (dp->uext > 0) {
		double uext = GET_D(dp->uext);
		if (!isnan(uext)) {
			uext = uext / 1000.0;
			json_append_member(jmerge, "uext", json_mknumber(uext));
		}
	}

	/* "uart" indicates the possible optional components for analog sensor data */
	double uart = GET_D(((nreports - 1) * 12) + dp->uart);
	//fprintf(stderr, "uart double %g\n", uart);
	if (!isnan(uart) && uart == 2) {
		/* "ac100present" was set from the erimask and indicates if there are any ac100 data following */
		if (ac100present) {
			/* "anum" up to 19 analog data readings may be present */
			double anum = GET_D(((nreports - 1) * 12) + dp->anum);
			//fprintf(stderr, "anum double %g\n", anum);
			if (!isnan(anum) && anum > 0) {
				int a;

				json_append_member(jmerge, "anum", json_mknumber(anum));
				ac100number = anum;
				for (a = 0; a < anum; a++) {
					/* "adid", "adty", "adda" for each item we have id, type and data*/
					//fprintf(stderr, "adid offset %d\n", ((nreports - 1) * 12) + a * 3 + dp->adid);
					char *adid = GET_S(((nreports - 1) * 12) + a * 3 + dp->adid);
					//fprintf(stderr, "adty offset %d\n", ((nreports - 1) * 12) + a * 3 + dp->adty);
					char *adty = GET_S(((nreports - 1) * 12) + a * 3 + dp->adty);
					//fprintf(stderr, "adda offset %d\n", ((nreports - 1) * 12) + a * 3 + dp->adda);
					char *adda = GET_S(((nreports - 1) * 12) + a * 3 + dp->adda);
					//fprintf(stderr, "adid %s adty %s adda %s\n", adid ? adid : "NULL", adty ? adty : "NULL", adda ? adda : "NULL");
					if (adid != NULL && adty != NULL && adda != NULL) {
						static char identifier[16+1];
						/* "adid-xx" we append the item number to the name */
						sprintf(identifier, "adid-%02d", a);
						//fprintf(stderr, "identifier %s\n", identifier);
						json_append_member(jmerge, identifier, json_mkstring(adid));
						sprintf(identifier, "adty-%02d", a);
						json_append_member(jmerge, identifier, json_mkstring(adty));
						sprintf(identifier, "adda-%02d", a);
						json_append_member(jmerge, identifier, json_mkstring(adda));
						/* if the data type is 1, this means temperature in celsius as
						 * 2-complement shifted 4 (divided by 16)
						 */
						if (!strcmp(adty, "1")) {
							long temp = strtol(adda, NULL, 16);	
							if (temp > 32535) {
								temp -= 65536;
							}
							double dtemp = temp;
							dtemp *= 0.0625;
							sprintf(identifier, "temp_c-%02d", a);
							json_append_member(jmerge, identifier, json_mkdouble(dtemp, 1));
						}
					}
				}
			}
		}
	}

	/* "can" data is an optional component indicated by the erim mask */
	if (canpresent) {
                if (dp->can > 0) {
                        char *can = GET_S(((nreports - 1) * 12)
				+ (ac100present ? ((ac100number - 1) * 3) : 0)
				+ dp->can);
                        if (can != NULL) {
                                json_append_member(jmerge, "can", json_mkstring(can));
                        }
                }
	}

	/* "xyzpresent" was set from the erimask and indicates if there are any xyz data following */
	if (xyzpresent) {
		/* "dgn" up to 3 xyz data readings may be present */
		double dgn = GET_D(((nreports - 1) * 12) + dp->dgn);
		//fprintf(stderr, "dgn double %g\n", dgn);
		if (!isnan(dgn) && dgn > 0) {
			int x;

			json_append_member(jmerge, "dgn", json_mknumber(dgn));
			xyznumber = dgn;
			for (x = 0; x < dgn; x++) {
				/* "da", "xyz" for each item we have data-attribute and xyz-axix-data*/
				//fprintf(stderr, "da offset %d\n", ((nreports - 1) * 12) + x * 2 + dp->da);
				double da = GET_D(((nreports - 1) * 12) + x * 2 + dp->da);
				//fprintf(stderr, "adty offset %d\n", ((nreports - 1) * 12) + x * 2 + dp->xyz);
				char *xyz = GET_S(((nreports - 1) * 12) + x * 2 + dp->xyz);
				//fprintf(stderr, "da %g xyz %s\n", da, xyz ? xyz : "NULL");
				if (xyz != NULL) {
					static char identifier[16+1];
					/* "da-xx" we append the item number to the name */
					sprintf(identifier, "da-%02d", x);
					//fprintf(stderr, "identifier %s\n", identifier);
					json_append_member(jmerge, identifier, json_mkdouble(da, 0));
					sprintf(identifier, "xyz-%02d", x);
					json_append_member(jmerge, identifier, json_mkstring(xyz));
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

		/* "hmc" is hour meter count as HHHHH:MM:SS  */
		if (dp->hmc > 0) {
			char *hmcstring = GET_S(dp->hmc);
			if (hmcstring != NULL && strlen(hmcstring) == 11) {
				double hmc = atof(hmcstring) * 3600.0 + atof(hmcstring + 6) * 60.0 + atof(hmcstring + 9);
				json_append_member(jmerge, "hmc", json_mknumber(hmc));
			}
		}

		/* "aiv" is analog input voltage in mV */
		if (dp->aiv > 0) {
			double aiv = GET_D(dp->aiv);
			if (!isnan(aiv)) {
				aiv = aiv / 1000.0;
				json_append_member(jmerge, "aiv", json_mknumber(aiv));
			}
		}

		/* "rpm" is engine round per minute */
		if (dp->rpm > 0) {
			double rpm = GET_D(((nreports - 1) * 12) + dp->rpm);
			if (!isnan(rpm)) {
				json_append_member(jmerge, "rpm", json_mknumber(rpm));
			}
		}

		/* "fcon" is fuel consumption in l/100km */
		if (dp->fcon > 0) {
			double fcon = GET_D(((nreports - 1) * 12) + dp->fcon);
			//fprintf(stderr, "fcon double %g\n", fcon);
			if (!isnan(fcon) && !isinf(fcon)) {
				json_append_member(jmerge, "fcon", json_mkdouble(fcon, 1));
			}
		}

		/* "flvl" is fuel level percentage */
		if (dp->flvl > 0) {
			double flvl = GET_D(((nreports - 1) * 12) + dp->flvl);
			if (!isnan(flvl)) {
				json_append_member(jmerge, "flvl", json_mknumber(flvl));
			}
		}

		/* "batt" is backup battery percentage */
		if (dp->batt > 0) {
			double d = GET_D(((nreports - 1) * 12) + dp->batt);
			if (!isnan(d)) {
				json_append_member(jmerge, "batt", json_mknumber(d));
			}
		}

		/* "ios" is io status as 4 characters hex*/
		if (iospresent && dp->ios > 0) {
		char *iosstring = GET_S(dp->ios);
			if (iosstring != NULL) {
				unsigned long ios = strtoul(iosstring, NULL, 16);
				json_append_member(jmerge, "din1",	json_mkbool(ios & 0x0001));
				json_append_member(jmerge, "ign",	json_mkbool(ios & 0x0002));
			}
		}

		/* "devs" is device status as 10 characters hex*/
		DLOG(2, "DEBUG devs %d [%d]\n", dp->devs, ((nreports - 1) * 12) + dp->devs);
		if (dp->devs > 0) {
			char *devsstring = GET_S(((nreports - 1) * 12) + dp->devs);
			if (devsstring != NULL) {
				DLOG(2, "DEBUG devs %d [%d] %s\n", dp->devs, ((nreports - 1) * 12) + dp->devs, devsstring ? devsstring : "NULL");
				unsigned long devs = strtoul(devsstring, NULL, 16);
				json_append_member(jmerge, "dout1",	json_mkbool(devs & 0x000001));
				json_append_member(jmerge, "dout2",	json_mkbool(devs & 0x000002));
				json_append_member(jmerge, "din1",	json_mkbool(devs & 0x000200));
				json_append_member(jmerge, "din2",	json_mkbool(devs & 0x000400));
				json_append_member(jmerge, "motion",	json_mkbool(devs & 0x020000));
				json_append_member(jmerge, "tow",	json_mkbool(devs & 0x040000));
				json_append_member(jmerge, "fake",	json_mkbool(devs & 0x080000));
				json_append_member(jmerge, "ign",	json_mkbool(devs & 0x200000));
				json_append_member(jmerge, "sens",	json_mkbool(devs & 0x400000));
			}
		}

		/* "din" is digital input as 2 characters hex */
		if (dp->din > 0) {
			char *dinstring = GET_S(dp->din);
			if (dinstring != NULL) {
				unsigned long din = strtoul(dinstring, NULL, 16);
				json_append_member(jmerge, "din1",	json_mkbool(din & 0x01));
				json_append_member(jmerge, "din2",	json_mkbool(din & 0x02));
			}
		}

		/* "dout" is digital output as 2 characters hex */
		if (dp->dout > 0) {
			char *doutstring = GET_S(dp->dout);
			if (doutstring != NULL) {
				unsigned long dout = strtoul(doutstring, NULL, 16);
				json_append_member(jmerge, "dout1",	json_mkbool(dout & 0x01));
				json_append_member(jmerge, "dout2",	json_mkbool(dout & 0x02));
			}
		}

		/* "sent" sent time at device*/
		DLOG(1, "DEBUG sent %d\n", dp->sent);
		if (dp->sent > 0) {
			DLOG(1, "DEBUG sent %d [%d]\n",
					dp->sent,
					dp->sent
					+ ((nreports - 1) * 12) 
					+ (iospresent ? 0 : -1)
					+ (ac100present ? 0 : -1)
					+ (ac100present ? ((ac100number - 1) * 3) : 0)
					+ (canpresent ? 0 : -1)
					+ (xyzpresent ? ((xyznumber - 1) * 2) : 0)
					);
			char *sent = GET_S(dp->sent
					+ ((nreports - 1) * 12) 
					+ (iospresent ? 0 : -1)
					+ (ac100present ? 0 : -1)
					+ (ac100present ? ((ac100number - 1) * 3) : 0)
					+ (canpresent ? 0 : -1)
					+ (xyzpresent ? ((xyznumber - 1) * 2) : 0)
					);
		        DLOG(1, "DEBUG sent %d [%d] %s\n",
					dp->sent,
					dp->sent
					+ ((nreports - 1) * 12) 
					+ (iospresent ? 0 : -1)
					+ (ac100present ? 0 : -1)
					+ (ac100present ? ((ac100number - 1) * 3) : 0)
					+ (canpresent ? 0 : -1)
					+ (xyzpresent ? ((xyznumber - 1) * 2) : 0),
					sent ? sent : "NULL");

			if (sent != NULL) {
				time_t epoch;

				if (str_time_to_secs(sent, &epoch) != 1) {
					xlog(ud, "Cannot convert sent time from [%s]\n", sent);
				} else {
					json_append_member(jmerge, "sent", json_mknumber(epoch));
				}
			}
		}

		/* "count" is counter for sent messages */
		if (dp->count > 0) {
			char *count = GET_S(dp->count 
					+ ((nreports - 1) * 12) 
					+ (iospresent ? 0 : -1)
					+ (ac100present ? 0 : -1)
					+ (ac100present ? ((ac100number - 1) * 3) : 0)
					+ (canpresent ? 0 : -1)
					+ (xyzpresent ? ((xyznumber - 1) * 2) : 0)
					);
			if (count != NULL) {
				json_append_member(jmerge, "count", json_mkstring(count));
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
		double vel, alt;
		double mcc, mnc;
		long cog;
		char *s;
		JsonNode *obj;

		// xlog(ud, "--> REP==%d dp->num==%d\n", rep, dp->num);

		int pos = (rep * 12); /* 12 elements in green area */

		//fprintf(stderr, "DEBUG nofix (%d)\n", pos + dp->utc);
		// TODO mnc etc. might be interesting though
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

		vel = GET_D(pos + dp->vel);
		if (!isnan(vel)) {
			json_append_member(obj, "vel", json_mkdouble(vel, 1));
		}

		if ((s = GET_S(pos + dp->cog)) != NULL) {
			cog = atoi(s);
			json_append_member(obj, "cog", json_mknumber(cog));
		}

		if ((s = GET_S(pos + dp->utc)) != NULL) {
			time_t epoch;

			if (str_time_to_secs(s, &epoch) != 1) {
				xlog(ud, "Cannot convert time from [%s]\n", s);
				continue;
			}
			json_append_member(obj, "tst", json_mknumber(epoch));
		        imei_last_position(imei, &lat, &lon, &epoch, &vel, &cog, true);
		}

		mcc = GET_D(pos + dp->mcc);
		if (!isnan(mcc)) {
			json_append_member(obj, "mcc", json_mknumber(mcc));
		}

		mnc = GET_D(pos + dp->mnc);
		if (!isnan(mnc)) {
			json_append_member(obj, "mnc", json_mknumber(mnc));
		}

		if ((s = GET_S(pos + dp->lac)) != NULL) {
			json_append_member(obj, "lac", json_mkstring(s));
		}

		if ((s = GET_S(pos + dp->cid)) != NULL) {
			json_append_member(obj, "cid", json_mkstring(s));
		}

		json_append_member(obj, "_type", json_mkstring("location"));

		if ((s = GET_S(pos + dp->acc)) != NULL) {

			// Converting the HDOP value to accuracy in +- meters
                        // There is no formula, so we use an estimate based on the occurances of the HDOP values:
			// $ grep GTFRI data-louie.log | cut -d ',' -f 8-8 | sort | uniq -c | sort -n
			//   109 4
			//   654 3
			//   677 2
			//   730 5
			//   974 0
			// 35903 1

			double acc;
			int hdop = atoi(s);

			switch (hdop) {
				case 1:
					acc = 10.0;
					break;
				case 2:
					acc = 50.0;
					break;
				case 3:
					acc = 100.0;
					break;
				case 4:
					acc = 500.0;
					break;
				case 5:
					acc = 1000.0;
					break;
				case 6:
					acc = 5000.0;
					break;
				default:
					acc = -1.0;
					break;
			}
			json_append_member(obj, "acc", json_mknumber(acc));
		}

		alt = GET_D(pos + dp->alt);
		if (!isnan(alt)) {
			json_append_member(obj, "alt", json_mkdouble(alt, 1));
		}

		if (!strcmp(subtype, "GTFRI") || !strcmp(subtype, "GTERI")) {
			switch (rid) {
				default:
					switch (rty) {
						case 0:
							json_append_member(obj, "t", json_mkstring("t"));
							break;
						case 1:
						case 3:
							json_append_member(obj, "t", json_mkstring("o")); /* corner report */
							break;
						case 2:
							json_append_member(obj, "t", json_mkstring("c"));
							break;
						case 4:
						case 6:
							json_append_member(obj, "t", json_mkstring("M")); /* mileage */
							break;
						case 5:
						default:
							json_append_member(obj, "t", json_mkstring("GTFRI"));
							break;
					}
					break;
			}
		} else if (!strcmp(subtype, "GTSTT") || !strcmp(subtype, "GTGSS")) {
			switch (mst) {
				case 16:
				case 12:
					json_append_member(obj, "t", json_mkstring("!"));
					break;
				case 11:
					json_append_member(obj, "t", json_mkstring("L"));
					break;
				case 21:
				case 41:
					json_append_member(obj, "t", json_mkstring("a"));
					break;
				case 22:
				case 42:
					json_append_member(obj, "t", json_mkstring("v"));
					break;
				default:
					json_append_member(obj, "t", json_mkstring("GTSTT"));
					break;
			}
		} else if (!strcmp(subtype, "GTDOG")) {
			json_append_member(obj, "t", json_mkstring("f"));
		} else if (!strcmp(subtype, "GTPNL")) {
			json_append_member(obj, "t", json_mkstring("1"));
		} else if (!strcmp(subtype, "GTBTC")) {
			json_append_member(obj, "t", json_mkstring("3"));
		} else if (!strcmp(subtype, "GTSTC")) {
			json_append_member(obj, "t", json_mkstring("2"));
		} else if (!strcmp(subtype, "GTBPL")) {
			json_append_member(obj, "t", json_mkstring("9"));
		} else if (!strcmp(subtype, "GTRTL")) {
			json_append_member(obj, "t", json_mkstring("u"));
		} else if (!strcmp(subtype, "GTIGN")) {
			json_append_member(obj, "t", json_mkstring("i"));
		} else if (!strcmp(subtype, "GTVGN")) {
			json_append_member(obj, "t", json_mkstring("i"));
		} else if (!strcmp(subtype, "GTIGF")) {
			json_append_member(obj, "t", json_mkstring("I"));
		} else if (!strcmp(subtype, "GTVGF")) {
			json_append_member(obj, "t", json_mkstring("I"));
		} else if (!strcmp(subtype, "GTEPN")) {
			json_append_member(obj, "t", json_mkstring("E"));
		} else if (!strcmp(subtype, "GTEPF")) {
			json_append_member(obj, "t", json_mkstring("e"));
		} else if (!strcmp(subtype, "GTMPN")) {
			json_append_member(obj, "t", json_mkstring("E"));
		} else if (!strcmp(subtype, "GTMPF")) {
			json_append_member(obj, "t", json_mkstring("e"));
		} else if (!strcmp(subtype, "GTSPD")) {
			json_append_member(obj, "t", json_mkstring("s"));
		} else if (!strcmp(subtype, "GTHBM")) {
			json_append_member(obj, "t", json_mkstring("h"));
		} else if (!strcmp(subtype, "GTIGL")) {
			json_append_member(obj, "t", json_mkstring(rty == 1 ? "I" : "i"));
		} else if (!strcmp(subtype, "GTNMD")) {
			/* "nmds" is the non movement detection status*/
			if (dp->nmds > 0) {
				double nmds = GET_D(dp->nmds);
				json_append_member(jmerge, "nmds", json_mkbool(nmds == 1.0));
			} else {
				json_append_member(jmerge, "nmds", json_mkbool(false));
			}
		} else {
			json_append_member(obj, "t", json_mkstring(subtype));
		}

		//fprintf(stderr, "lastlat\n");
		if (!isnan(lastlat)) {
			double meters = haversine_dist(lastlat, lastlon, lat, lon);

#ifdef NOTREQUIRED
			// FIXME: Experimental if we haven't moved, skip the publish

			if (meters < 0.1) {
				lastlat = lat;
				lastlon = lon;
				json_delete(obj);
				xlog(ud, "Dropping segment %2d/%d because %.2lf distance covered\n",
					rep + 1, nreports, meters);
				continue;
			}
#endif /* NOTREQUIRED */

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

		transmit_json(ud, imei, obj, true);


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
	char *response = NULL, *r;

	while (fgets(line, sizeof(line) - 1, fp) != NULL) {
		if (*line == '#' || *line == '\n')
			continue;

		line[strcspn(line, "\r\n")] = '\0';

		response = NULL;
		r = handle_report(ud, line, &response);
		if (r)
			free(r);
		if (response && *response)
			free(response);
		mosquitto_loop(ud->mosq, 0, 1);
	}
	return (0);
}
