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
#ifdef WITH_BEAN
        const char *bean_host;
	int bean_port;
        const char *bean_tube;
#endif
	const char *datalog;
	const char *logfile;
	const char *username;
	const char *password;
	const char *cafile;
	const char *capath;
	const char *certfile;
	const char *keyfile;
	const char *client_id;
	JsonNode *subscriptions;
	struct my_device *devices;
	const char *extra_json;
	const char *reporttopic;
	const char *dumpdir;
	const char *datadir;
	const char *namesdir;
	const char *rawtopic;
} config;

int ini_handler(void *cf, const char *section, const char *key, const char *val);

#endif
