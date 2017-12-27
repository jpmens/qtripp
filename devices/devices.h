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

	int imei;		/* imei string */
	int name;		/* device name string */
	int uext;		/* external power voltage number in mV */
	int rit;		/* report id / report type combined */
	int rid;		/* report id */
	int rty;		/* report type */
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
	int sent;		/* sent time */
	int count;		/* count of messages */

	int din;		/* digital input */
	int dout;		/* digital output */
	int jst;		/* jamming status */
	int mst;		/* motion status */
	int ios;		/* io status */
	int ubatt;		/* backup battery voltage */
	int don;		/* duration ignition on */
	int doff;		/* duration ignition off */
	int nmds;		/* non motion detection status */

	int ftyp;		/* fix type */
	int rx;			/* receive strength */
	int mcc1;		
	int mnc1;	
	int lac1;
	int cid1;		
	int rx1;		
	int mcc2;		
	int mnc2;		
	int lac2;		
	int cid2;		
	int rx2;		
	int mcc3;		
	int mnc3;		
	int lac3;		
	int cid3;		
	int rx3;		
	int mcc4;		
	int mnc4;		
	int lac4;		
	int cid4;		
	int rx4;		
	int mcc5;		
	int mnc5;		
	int lac5;		
	int cid5;		
	int rx5;		
	int mcc6;		
	int mnc6;		
	int lac6;		
	int cid6;		
	int rx6;		

	int erim;
	int can;
	int uart;
	int anum;
	int adid;
	int adty;
	int adda;
	int rst;
	int eant;
	int woid;
	int woa;
	int cwjv;
	int gjst;

	int alty;	/* alarm type (temperature) 1 below, 2 within, 3 above limits */
	int temp;	/* temperature degrees Celsius */
	int vin;	/* Vehicle Identification Number */
	int rpm;	/* engine rpm */
	int fcon;	/* fuel consumption L / 100 km or Inf or NaN */
	int flvl;	/* fuel level in percent 0-100 */

	char *add_name;

        UT_hash_handle hh;
};

void load_devices();
void free_devices();
struct _device *lookup_devices(char *key, char *monami);
