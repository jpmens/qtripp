#include <stdio.h>
#include "uthash.h"
#include "ini.h"        /* https://github.com/benhoyt/inih */

typedef struct config {
        int listen_port;
        const char *host;
	int port;
	const char *username;
	const char *password;
	const char *cafile;
	const char *certfile;
	const char *keyfile;
	const char *client_id;
	// struct my_model {
	// 	char *type;              /* key, type*/
	// 	char *model;
	// 	UT_hash_handle hh;
	// } *models;
} config;

int ini_handler(void *cf, const char *section, const char *key, const char *val);
// char *protov_to_model(config *cf, char *protov);
