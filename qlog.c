/*
 * qlog
 * Copyright (C) 2018 Jan-Piet Mens <jp@mens.de>
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
#include "mongoose.h"

#define DATAFILE	"qlog.data"

struct udata {
	int fd;			/* Open data file */
	void *mb;		/* Will hold mbuf */
};

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
	struct mbuf *io = &nc->recv_mbuf;
	struct udata *ud = (struct udata *)nc->mgr->user_data;
	size_t ml;      /* mb len */
	struct mbuf *mb = (struct mbuf *)ud->mb;
	char buf[BUFSIZ];

	switch (ev) {
		case MG_EV_POLL:

			/*
			 * A record is +...$
			 * Search for the '$', cut there, process, and remove what we've
			 * done thus far from `mb' and await more data.
			 */

			for (ml = 0; ml < mb->len; ml++) {
				if (mb->buf[ml] == '$') {
					off_t pos;
					size_t nbytes = ml + 1;

					write(ud->fd, mb->buf, nbytes);
					write(ud->fd, "\n", 1);

					pos = lseek(ud->fd, 0, SEEK_CUR);
					if (pos > (1024*1024)) {
						char path[BUFSIZ];

						close(ud->fd);
						snprintf(path, BUFSIZ, "%s.%ld", DATAFILE, time(0));
						link(DATAFILE, path);
						unlink(DATAFILE);
						ud->fd = open(DATAFILE, O_WRONLY | O_CREAT, 0666);
					}

					/* Clear what we've used from `mb' */
					mbuf_remove(mb, nbytes);
					break;
				}
			}
			break;

		case MG_EV_ACCEPT:
			mg_sock_addr_to_str(ev_data, buf, sizeof(buf), MG_SOCK_STRINGIFY_IP);
			syslog(LOG_INFO, "Connection from socket %s", buf);
			break;

		case MG_EV_RECV:
			/*
			 * shove the bytes into our `mb' buffer, and return, after
			 * clearing our working buffer;
			 * we do the real work during POLL.
			 */

			mbuf_append(mb, io->buf, io->len);
			mbuf_remove(io, io->len);
			break;

		default:
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
	bind_opts.user_data = NULL;

	syslog(LOG_INFO, "Listening for GPRS on port %s", port_str);

	c = mg_bind_opt(&mgr, port_str, ev_handler, bind_opts);
	if (c == NULL) {
		syslog(LOG_ERR, "Error starting server: %s: %m", *bind_opts.error_string);
		exit(1);
	}

	struct mbuf mbi;
	udata.mb = (struct mbuf *)&mbi;
	mbuf_init(udata.mb, 48);

	mgr.user_data = &udata; // experiment

	while (1) {
		mg_mgr_poll(&mgr, 1000);
	}

	/* NOTREACHED */
	mg_mgr_free(&mgr);
	return (0);
}
