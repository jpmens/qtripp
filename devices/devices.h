#include <string.h>
#include <stdlib.h>
#include "uthash.h"

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
