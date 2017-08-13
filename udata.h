#ifndef _UDATA_H_INCL_
# define  _UDATA_H_INCL_

#include <stdbool.h>

struct udata {
	bool debugging;
	FILE *logfp;			/* open logfile */
        const char *mqtt_host;
        int mqtt_port;
        struct mosquitto *mosq;
        int datalog;
        struct mg_mgr *mgr;     /* mongoose manager */
        struct config *cf;
};

#endif
