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
#include "uthash.h"
#include "conf.h"

#define _eq(n) (strcmp(key, n) == 0)
int ini_handler(void *cf, const char *section, const char *key, const char *val)
{
	config *c = (config *)cf;

	// printf("section=[%s]  >%s<-->%s\n", section, key, val);
	
	if (!strcmp(section, "defaults")) {
		if (_eq("listen_port"))	c->listen_port = strdup(val);
		if (_eq("datalog"))	c->datalog = strdup(val);
		if (_eq("logfile"))	c->logfile = strdup(val);
		if (_eq("debughex"))	c->debughex = strdup(val);
		if (_eq("dumpdir"))     c->dumpdir = strdup(val);
		if (_eq("cdb_path"))    c->cdb_path = strdup(val);
#ifdef STATSD
		if (_eq("statsdhost"))  c->statsdhost = strdup(val);
#endif
	}

	if (!strcmp(section, "devices")) {
		/*
		 *      [devices]
		 *      imei = topic
		 *      123456789012345 = "owntracks/gv/12345"
		 *      * = "owntracks/dump/"
		 */

		struct my_device *d = (struct my_device *)malloc(sizeof (struct my_device));
		d->did = strdup(key);
		d->topic = strdup(val);
		HASH_ADD_KEYPTR(hh, c->devices, d->did, strlen(d->did), d);


	}

#ifdef WITH_BEAN
	if (!strcmp(section, "bean")) {
		if (_eq("host"))	c->bean_host = strdup(val);
		if (_eq("port"))	c->bean_port = atoi(val);
		if (_eq("tube"))	c->bean_tube = strdup(val);
	}
#endif

	if (!strcmp(section, "mqtt")) {
		if (_eq("host"))	c->host = strdup(val);
		if (_eq("username"))    c->username = strdup(val);
		if (_eq("password"))    c->password = strdup(val);
		if (_eq("client_id")) {
			c->client_id = (val && *val) ? strdup(val) : NULL;
			if (c->client_id == NULL) {
				fprintf(stderr, "client_id must not be NULL\n");
				exit(3);
			}
		}
		if (_eq("cafile"))      c->cafile = strdup(val);
		if (_eq("capath"))      c->capath = strdup(val);
		if (_eq("certfile"))    c->certfile = strdup(val);
		if (_eq("keyfile"))     c->keyfile = strdup(val);
		if (_eq("port"))	c->port = atoi(val);
		if (_eq("reporttopic"))     c->reporttopic = strdup(val);
		if (_eq("rawtopic"))     c->rawtopic = strdup(val);

		if (!strcmp(key, "subscribe")) {
			if (c->subscriptions == NULL) {
				c->subscriptions = json_mkarray();
			}
			json_append_element(c->subscriptions, json_mkstring(val));
		}
	}
	return (1);
}
