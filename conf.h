#ifndef _CONF_H_INCL_
# define _CONF_H_INCL_

#include <stdio.h>
#include "uthash.h"
#include "json.h"
#include "ini.h"        /* https://github.com/benhoyt/inih */

struct my_device {
	char *did;              /* key, deviceId*/
	char *topic;
	UT_hash_handle hh;
};

typedef struct config {
        const char *listen_port;
        const char *debughex;
        const char *host;
	int port;
	const char *datalog;
	const char *logfile;
	const char *username;
	const char *password;
	const char *cafile;
	const char *certfile;
	const char *keyfile;
	const char *client_id;
	JsonNode *subscriptions;
	struct my_device *devices;
	const char *extra_json;
} config;

int ini_handler(void *cf, const char *section, const char *key, const char *val);

#endif
