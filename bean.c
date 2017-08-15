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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "conf.h"
#include "util.h"
#include "json.h"
#include "bean.h"

void bean_put(struct udata *ud, JsonNode *jfull)
{
	const int priority = 2000;	/* 0 == Urgent */
	const int delay = 0;
	const int ttr = 30;		/* <ttr> -- time to run -- is an
					 * integer number of seconds to allow a
					 * worker to run this job. This time is
					 * counted from the moment a worker
					 * reserves this job. If the worker
					 * does not delete, release, or bury
					 * the job within <ttr> seconds, the
					 * job will time out and the server
					 * will release the job.  The minimum
					 * ttr is 1. If the client sends 0, the
					 * server will silently increase the
					 * ttr to 1.
					 */
	int id;
	char *js;

	if ((js = json_encode(jfull)) != NULL) {
		id = bs_put(ud->bean_socket, priority, delay, ttr, js, strlen(js) + 0);

		xlog(ud, "bean put: job id: %d\n", id);
		assert(id != -1);
		free(js);
	} else {
		xlog(ud, "Cannot encode JSON for bean\n");
	}
}
