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
	int acc;			/* gps accuracy - repeats */
	int cog;			/* azimuth 		- repeats */
	int alt;			/* altitude 	- repeats */
	int vel;			/* velocity 	- repeats */
	int lon;			/* longitude 	- repeats */
	int lat;			/* latitude 	- repeats */
	int utc;			/* utc time		- repeats */
	int mcc;			/* MCC 			- repeats */
	int mnc;			/* MNC 			- repeats */
	int lac;			/* LAC 			- repeats */
	int cid;			/* cell ID 		- repeats */
	int odometer;		/* milage */
	int hmc;		/* hour meter count */
	int aiv;		/* analog input voltage */
	int batt;		/* backup battery percentage */
	int devs;		/* device status */
	int sent;		/* sent time */
	int count;		/* count of messages */

	int din;		/* digital input */
	int dout;		/* digital output */
	int mst;		/* motion status */
	int ios;		/* io status */
	int ubatt;		/* backup battery voltage */
	int don;		/* duration ignition on */
	int doff;		/* duration ignition off */
	int nmds;		/* non motion detection status */
	int erim;		/* ERI mask */
	int can;		
	int uart;		/* UART device type */
	int anum;		/* 1wire devices number */
	int adid;		/* 1wire devices id  */
	int adty;		/* 1wire devices type */
	int adda;		/* 1wire devices DATA */
	int dgn;		
	int da;			
	int xyz;		

	int vin;		/* Vehicle Identification Number */
	int rpm;		/* engine rpm */
	int fcon;		/* fuel consumption L / 100 km or Inf or NaN */
	int flvl;		/* fuel level in percent 0-100 */

	int d_type;		/* String representation of the device type e.g. gl300m */
	int ver_fw;		/* HEX firmware version MAMI for MAjor MInor */
	int ver_hw;		/* HEX hardware version MAMI for MAjor MInor */

	char *add_name;

	UT_hash_handle hh;
};

void load_devices();
void free_devices();
struct _device *lookup_devices(char *key, char *monami);
