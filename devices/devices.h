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
	char *id;		/* e.g. "GTFRI-MOMAMI" for MOdel, MAjor, MInor */

	int imei;
	int name;		/* device name */
	int uext;		/* external power voltage */
	int rit;		/* report id / report type */
	int num;		/* number of position entries */
	int acc;			/* repeats */
	int cog;			/* repeats */
	int alt;			/* repeats */
	int vel;			/* repeats */
	int lon;			/* repeats */
	int lat;			/* repeats */
	int utc;			/* repeats */
	int mcc;			/* repeats */
	int mnc;			/* repeats */
	int lac;			/* repeats */
	int cid;			/* repeats */
	int odometer;		/* milage */
	int hmc;		/* hour meter count */
	int aiv;		/* analog input voltage */
	int batt;		/* backup battery percentage */
	int devs;		/* device status */
	char *add_name;

        UT_hash_handle hh;
};

void load_devices();
void free_devices();
struct _device *lookup_devices(char *key, char *monami);
