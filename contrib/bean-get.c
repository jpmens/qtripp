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

#include <stdio.h>
#include <assert.h>
#include "beanstalk.h"

int main(int argc, char **argv)
{
	int s = bs_connect("127.0.0.1", 11300);
	BSJ *job;
	long id;

	bs_use(s, "qtripp");
	bs_watch(s, "qtripp");
	bs_ignore(s, "default");

	while (bs_reserve(s, &job) == BS_STATUS_OK) {
		printf("reserve job id: %lld size: %lu\n", job->id, job->size);

		/*
		 * \r\n\0
		 * | | |
		 * | | + ---  job->size + 2
		 * | +------  job->size + 1
		 * +--------  job->size + 0
		 */

		job->data[job->size] = 0;

		printf("%s\n", job->data);

		bs_free_job(job);
		bs_delete(s, job->id);
	}

	bs_disconnect(s);
	return (0);
}
