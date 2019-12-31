/*
 * qlog
 * Copyright (C) 2018-2020 Jan-Piet Mens <jp@mens.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <netinet/in.h>
#include "mongoose.h"

#define DATAFILE	"qlog.data"

static char *ev_names[] = {
	"MG_EV_POLL",		// 0    /* Sent to each connection on each mg_mgr_poll() call */
	"MG_EV_ACCEPT",		// 1  /* New connection accepted. union socket_address * */
	"MG_EV_CONNECT",	// 2 /* connect() succeeded or failed. int *  */
	"MG_EV_RECV",		// 3    /* Data has been received. int *num_bytes */
	"MG_EV_SEND",		// 4    /* Data has been written to a socket. int *num_bytes */
	"MG_EV_CLOSE",		// 5   /* Connection is closed. NULL */
	"MG_EV_TIMER",		// 6   /* now >= conn->ev_timer_time. double * */
	NULL };

struct udata {
	int fd;			/* Open data file */
};

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct mbuf *io = &nc->recv_mbuf;
	struct udata *ud = (struct udata *)nc->mgr->user_data;
	struct sockaddr_in *sa;
	size_t ml;      /* mb len */
	int octets, n;
	char buf[128];

	fprintf(stderr, "ev=%d %-15s, sock=%d ",
		ev,
		(ev >= 0 && ev <= 6) ?  ev_names[ev] : "??",
		nc->sock);
	if (ev == MG_EV_RECV)
		fprintf(stderr, " (ed=%d) ", *(int *)ev_data);
	fprintf(stderr, "\n");

	switch (ev) {
		case MG_EV_POLL:
			break;

		case MG_EV_ACCEPT:
			sa = (struct sockaddr_in *)ev_data;
			printf("IP = %s\n", inet_ntoa(sa->sin_addr));
			mg_sock_addr_to_str(ev_data, buf, sizeof(buf), MG_SOCK_STRINGIFY_IP);
			syslog(LOG_INFO, "Connection from socket %s", buf);
			break;

		case MG_EV_RECV:
			octets = *(int *)ev_data;
			for (ml = 0; ml < io->len; ml++) {
				if (io->buf[ml] == '$') {
					size_t nbytes = ml + 1;
					n = write(ud->fd, io->buf, nbytes);
					write(ud->fd, "\n", 1);
					mbuf_remove(io, nbytes);
					if (io->len > 0) {
						ml = -1;	/* Reset; restart loop at zero */
					}
				}
			}
			break;

		case MG_EV_CLOSE:
			syslog(LOG_INFO, "Flushing %ld bytes", io->len);
			if ((n = write(ud->fd, io->buf, io->len)) != io->len) {
				syslog(LOG_ERR, "Short write on flush: %m");
			}
			mbuf_remove(io, io->len);
			break;
	}
}

int main(int argc, char **argv)
{
	struct mg_mgr mgr;
	struct mg_connection *c;
	struct mg_bind_opts bind_opts;
	struct udata udata;
	const char *e = NULL;
	char *port_str;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", *argv);
		return (-2);
	}

	openlog("qlog", LOG_PERROR|LOG_PID, LOG_DAEMON);

	port_str = strdup(argv[1]);

	memset(&udata, 0, sizeof(udata));

	if ((udata.fd = open(DATAFILE, O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1) {
		syslog(LOG_ERR, "Cannot open data file %s: %m", DATAFILE);
		exit(3);
	}

	mg_mgr_init(&mgr, NULL);

	memset(&bind_opts, 0, sizeof(bind_opts));
	bind_opts.error_string = &e;

	syslog(LOG_INFO, "Listening for GPRS on port %s", port_str);

	bind_opts.user_data = "HELLO DOLLY";

	mgr.user_data = &udata;
	c = mg_bind_opt(&mgr, port_str, ev_handler, bind_opts);
	if (c == NULL) {
		syslog(LOG_ERR, "Error starting server: %s: %m", *bind_opts.error_string);
		exit(1);
	}

	while (1) {
		mg_mgr_poll(&mgr, 10000);
	}
	fprintf(stderr, "NOTREACHED\n");

	/* NOTREACHED */
	mg_mgr_free(&mgr);
	return (0);
}
