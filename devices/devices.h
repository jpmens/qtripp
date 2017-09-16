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

#include <string.h>
#include <stdlib.h>
#include "uthash.h"

struct _device {
	char *id;		/* "GTFRI-jjmm" */

	int num;
	int imei;
	int acc;
	int cog;
	int alt;
	int vel;
	int lon;
	int lat;
	int utc;
	int odometer;
	int batt;
	int add;
	char *add_name;

        UT_hash_handle hh;
};

void load_devices();
void free_devices();
struct _device *lookup_devices(char *key, char *jjmm);
