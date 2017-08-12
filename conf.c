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
		if (_eq("listen_port"))         c->listen_port = atoi(val);
	}

#if 0
        if (!strcmp(section, "models")) {
		/*
		 *      [models]
		 *      [models]
		 *	31: GV65,
		 *	36: GV500
		 */

		struct my_model *mo = (struct my_model *)malloc(sizeof (struct my_model));
		mo->type = strdup(key);
		mo->model = strdup(val);
		HASH_ADD_KEYPTR(hh, c->models, mo->type, strlen(mo->type), mo);
        }
#endif

        if (!strcmp(section, "mqtt")) {
                if (_eq("host"))        c->host = strdup(val);
                if (_eq("username"))    c->username = strdup(val);
                if (_eq("password"))    c->password = strdup(val);
                if (_eq("cafile"))      c->cafile = strdup(val);
                if (_eq("certfile"))    c->certfile = strdup(val);
                if (_eq("keyfile"))     c->keyfile = strdup(val);
                if (_eq("port"))        c->port = atoi(val);

/*
                if (!strcmp(key, "subscribe")) {
                        if (c->subscriptions == NULL) {
                                c->subscriptions = json_mkarray();
                        }
                        json_append_element(c->subscriptions, json_mkstring(val));
                }
	*/
        }
	return (1);
}
