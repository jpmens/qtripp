#include <string.h>
#include <stdlib.h>
#include "uthash.h"

/* This enum enumerates the position of the elements in struct _device below */
typedef enum vpos { ID, NUM, IMEI, ACC, ALT, VEL, LON, LAT, UTC, DIST } vpos;

struct _device {
	char *id;		/* "GTFRI-jjmm" */

	int num;
	int imei;
	int acc;
	int alt;
	int vel;
	int lon;
	int lat;
	int utc;
	int dist;

        UT_hash_handle hh;
};

void load_devices();
void free_devices();
struct _device *lookup_devices(char *key, char *jjmm);
