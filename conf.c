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
		if (_eq("extra_json"))	c->extra_json = strdup(val);
	}

	if (!strcmp(section, "mqtt")) {
		if (_eq("host"))	c->host = strdup(val);
		if (_eq("username"))    c->username = strdup(val);
		if (_eq("password"))    c->password = strdup(val);
		if (_eq("cafile"))      c->cafile = strdup(val);
		if (_eq("certfile"))    c->certfile = strdup(val);
		if (_eq("keyfile"))     c->keyfile = strdup(val);
		if (_eq("port"))	c->port = atoi(val);

		if (!strcmp(key, "subscribe")) {
			if (c->subscriptions == NULL) {
				c->subscriptions = json_mkarray();
			}
			json_append_element(c->subscriptions, json_mkstring(val));
		}
	}
	return (1);
}
