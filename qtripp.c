#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <mosquitto.h>
#include "conf.h"
#include "mongoose.h"
#include "uthash.h"
#include "util.h"
#include "tline.h"

#include "models.h"
#include "devices.h"
#include "reports.h"
#include "ignores.h"

static config cf = {
        .host           = "localhost",
        .port           = 1883
};

/*
 * We'll be hashing connection information using alternate keys: one is
 * the socket (sock) and the other is by imei.
 */

struct conndata {
	int sock;		/* key */
	char *imei;
	char *client_ip;
	struct mg_connection *nc;
	UT_hash_handle hh;		/* makes this hashable for key sock */
	UT_hash_handle hh_imei;		/* makes this hashable for alternate key imei */
};
struct conndata *conns_by_sock = NULL;
struct conndata *conns_by_imei = NULL;

struct conndata *find_conn(int sock)
{
	struct conndata *co;

	HASH_FIND_INT(conns_by_sock, &sock, co);
	return (co);
}

struct conndata *find_imei(char *imei)
{
	struct conndata *co;

	HASH_FIND(hh_imei, conns_by_imei, imei, strlen(imei), co);
	return (co);
}


struct conndata *add_conn(int sock)
{
	struct conndata *co;

	HASH_FIND_INT(conns_by_sock, &sock, co);		/* imei already in hash? */
	if (co == NULL) {
		co = (struct conndata *)malloc(sizeof (struct conndata));
		co->sock = sock;
		HASH_ADD_INT(conns_by_sock, sock, co);
	}
	co->imei	= NULL;
	co->client_ip	= NULL;
	co->nc		= NULL;
	return (co);
}

void delete_conn(struct conndata *co)
{
#if 0
	struct conndata *s;
	if (co->imei) {
		HASH_FIND(hh_imei, conns_by_imei, co->imei, strlen(co->imei), s);
		if (s) {
			HASH_DEL(conns_by_imei, s);
		}
	}
#endif

	if (co->imei) free(co->imei);
	if (co->client_ip) free(co->client_ip);
	HASH_DEL(conns_by_sock, co);
	// free(co);
	co = NULL;
}

void print_conns(struct udata *ud)
{
	struct conndata *co;

	for (co = conns_by_sock; co != NULL; co = (struct conndata *)(co->hh.next)) {
		xlog(ud, "sock %d: %s (%s)\n", co->sock,
			co->imei ? co->imei : "nil",
			co->client_ip ? co->client_ip : "nil");
	}
}

/*
 * We've obtained a "line" of text from a tracker via TCP in `buf' (0-terminated).
 */

char *process(struct udata *ud, char *buf, size_t buflen, struct mg_connection *nc)
{
	char *imei;
	
	buf[buflen] = 0;

	imei = handle_report(ud, buf);
	return (imei);
}


#if 0
void lwt(struct udata *ud, char *imei)
{
	JsonNode *o = json_mkobject();
	char *js;
	char topic[1024];

	if (!imei || !*imei)
		return;

	json_append_member(o, "_type", json_mkstring("lwt"));
	json_append_member(o, "imei", json_mkstring(imei));
	json_append_member(o, "tst", json_mknumber(time(0)));

	sprintf(topic, "owntracks/aplicom/%s", imei);

	if ((js = json_stringify(o, NULL)) != NULL) {
		pub(ud, topic, js, false);
		free(js);
	}
	json_delete(o);
}
#endif

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct mbuf *io = &nc->recv_mbuf;
	char *imei = NULL;
	char buf[512];
	struct conndata *co;
	struct udata *ud = (struct udata *)nc->mgr->user_data;

	/*
	 * On a new connection (EV_ACCEPT), add an an entry hashed by socket number
	 * and store that conndata in mongoose's nc user_data. Then, when required,
	 * obtain nc->user_data which points to conndata structure and use it.
	 * As soon as we receive an IMEI number we add it to conndata structure *and*
	 * add an alternate hash to it.
	 */

	switch (ev) {
		case MG_EV_ACCEPT:
			mg_sock_addr_to_str(ev_data, buf, sizeof(buf), MG_SOCK_STRINGIFY_IP);

			if ((co = find_conn(nc->sock)) == NULL) {
				co = add_conn(nc->sock);
				co->client_ip = strdup(buf);
				co->nc = nc;
			}

			nc->user_data = co;
			break;
			
		case MG_EV_RECV:
			if (ud->cf->debughex) {
				mg_hexdump_connection(nc, ud->cf->debughex, io->buf, io->len, ev);
			}
			// write(1, io->buf, io->len);
			write(ud->datalog, io->buf, io->len);
			write(ud->datalog, "\n", 1);

			imei = process(ud, io->buf, io->len, nc);

			if (imei != NULL) {
				if ((co = (struct conndata *)nc->user_data) != NULL) {
					if (co->imei == NULL) {
						co->imei = strdup(imei);
						HASH_ADD_KEYPTR(hh_imei, conns_by_imei, co->imei, strlen(co->imei), co);
					}
				}
				free(imei);
			}
			mbuf_remove(io, io->len);      // Discard data from recv buffer
			break;
		case MG_EV_CLOSE:
			if ((co = (struct conndata *)nc->user_data) != NULL) {
				xlog(ud, "Disconnected from %s IMEI <%s>\n",
					co->client_ip ? co->client_ip : "unknown",
					co->imei ? co->imei : "");
				// lwt(ud, co->imei);

				delete_conn(co);
				nc->user_data = NULL;
			}
			break;
		default:
			break;
	}
}

/*
 * Return a pointer to a malloced device name contained in
 * the bit before "/cmd" of an MQTT topic. Caller must free.
 *
 * "owntracks/gv/92939391/cmd" => "92939391"
 */

char *topic_to_device(char *topic)
{
        char *copy = strdup(topic);
        char *end = copy + strlen(copy) - strlen("/cmd");
        char *ptr;

        *end = 0;       /* Chop copy at last slash, cutting off "/cmd" */

        /* put ptr past last slash or on begin of string if no slash */
        if ((ptr = strrchr(copy, '/')) != NULL)
                ++ptr;
        else ptr = copy;

	ptr = strdup(ptr);
	free(copy);
	return (ptr);
}

void write_to_connection(struct mg_mgr *mgr, char *imei, char *payload)
{
	struct conndata *co;
	struct mg_connection *c;
	struct udata *ud = (struct udata *)mgr->user_data;

	if ((co = find_imei(imei)) == NULL) {
		xlog(ud, "Can't stab for imei %s\n", imei);
		return;
	}

	if ((c = co->nc) == NULL) {
		xlog(ud, "No connection information for imei %s\n", imei);
		return;
	}

	xlog(ud, "sock=%d = %s. Sending %s\n", c->sock, co->imei, payload);
	mg_printf(c, "%s", payload);
}

void on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *m)
{
	struct udata *ud = (struct udata *)userdata;
	struct mg_mgr *mgr = ud->mgr;
	char *device_id;


        /*
         * mosquitto_message->
         *       int mid;
         *       char *topic;
         *       void *payload;
         *       int payloadlen;
         *       int qos;
         *       bool retain;
         */

        xlog(ud, "MQTT SUBSCRIBE: %s %s\n", (char *)m->topic, (char *)m->payload);
	device_id = topic_to_device((char *)m->topic);

	if (!strcmp(device_id, "*")) {
		print_conns(ud);
		return;
	}
	write_to_connection(mgr, device_id, (char *)m->payload);
}
int main(int argc, char **argv)
{
	struct mg_mgr mgr;
	struct mg_connection *c;
	struct mg_bind_opts bind_opts;
	struct udata udata, *ud = &udata;
	struct mosquitto *mosq;
	char err[1024];
	const char *e = NULL;
	int mid, rc;
	struct my_device *d, *tmp;
	JsonNode *j;

        if (ini_parse("qtripp.ini", ini_handler, &cf) < 0) {
		xlog(NULL, "Can't load/parse ini file.\n");
                return (1);
        }

	memset(&udata, 0, sizeof(udata));
        ud->debugging           = true;
	ud->logfp		= fopen(cf.logfile, "a");

        load_models();
        load_reports();
        load_devices();
        load_ignores();

	mosquitto_lib_init();

	mosq = mosquitto_new(cf.client_id, true, &udata);
	if (!mosq) {
		fprintf(stderr, "Error: Out of memory.\n");
		mosquitto_lib_cleanup();
		return (-1);
	}

	if (cf.username || cf.password) {
		mosquitto_username_pw_set(mosq, cf.username, cf.password);
	}

	mosquitto_message_callback_set(mosq, on_message);

	rc = mosquitto_connect(mosq, cf.host, cf.port, 60);
	if (rc) {
		if (rc == MOSQ_ERR_ERRNO) {
			strerror_r(errno, err, 1024);
			xlog(ud, "Error: %s\n", err);
		} else {
			xlog(ud, "Unable to connect (%d).\n", rc);
		}
		return (-2);
	}

	json_foreach(j, cf.subscriptions) {
		xlog(ud, "subscribing to %s\n", j->string_);
		mosquitto_subscribe(mosq, &mid, j->string_, 0);
	}

	mosquitto_loop_start(mosq);

	udata.mosq	= mosq;
	udata.datalog	= open(cf.datalog, O_WRONLY | O_APPEND | O_CREAT, 0666);

	puts("starting..");

	mg_mgr_init(&mgr, NULL);

	memset(&bind_opts, 0, sizeof(bind_opts));
#if 0
	bind_opts.ssl_cert = certfile;
	bind_opts.ssl_key = keyfile;
	bind_opts.ssl_ca_cert = NULL;
#endif
	bind_opts.error_string = &e;
	bind_opts.user_data = NULL;

	c = mg_bind_opt(&mgr, cf.listen_port, ev_handler, bind_opts);
	if (c == NULL) {
		xlog(ud, "Error starting server: %s\n", *bind_opts.error_string);
		exit(1);
	}

	udata.mgr = &mgr;
	udata.cf  = &cf;

	mgr.user_data = &udata; // experiment

	while (1) {
		mg_mgr_poll(&mgr, 1000);
	}

	mg_mgr_free(&mgr);

	HASH_ITER(hh, cf.devices, d, tmp) {
		// printf("\t%s => %s\n", d->did, d->topic);
		free(d->did);
		free(d->topic);
		HASH_DEL(cf.devices, d);
		free(d);
	}

	json_delete(cf.subscriptions);
	return (0);
}
