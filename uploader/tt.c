#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <curl/curl.h>
#include <assert.h>
#include <inttypes.h>
#include "beanstalk.h"

/*
 * 
 * tt.c (C)2018 by Jan-Piet Mens <jp@mens.de>
 *
 *    _____ _____ 
 *   |_   _|_   _|
 *     | |   | |  
 *     | |   | |  
 *     |_|   |_|  
 *                
 * To Traccar. Read beanstalk and copy the JSON therein to traccar by HTTP POST
 * This utility replaces to-traccar.py which we disabled on 2018-MAR-23 due
 * to it causing very many left-over connections, possibly due to an issue in
 * Python requests.
 */

#define URL "http://127.0.0.1:5144/"	/* OwnTracks protocol in Traccar */
#define TUBENAME "totraccar"

struct WriteThis {
	const char *readptr;
	size_t sizeleft;
};

static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
	struct WriteThis *wt = (struct WriteThis *)userp;
	size_t buffer_size = size * nmemb;

	if (wt->sizeleft) {
		/*
		 * copy as much as possible from the source to the
		 * destination
		 */
		size_t copy_this_much = wt->sizeleft;
		if (copy_this_much > buffer_size)
			copy_this_much = buffer_size;
		memcpy(dest, wt->readptr, copy_this_much);

		wt->readptr += copy_this_much;
		wt->sizeleft -= copy_this_much;
		return copy_this_much;	/* we copied this many bytes */
	}
	return 0;		/* no more data left to deliver */
}

/*
 * Return true if an upload was possible (with http status 200), false otherwise
 */

bool upload(CURL * cu, char *buf)
{
	CURLcode status;
	long code;

	curl_easy_setopt(cu, CURLOPT_POSTFIELDS, buf);
	curl_easy_setopt(cu, CURLOPT_POSTFIELDSIZE, -1L);

	status = curl_easy_perform(cu);
	if (status != 0) {
		fprintf(stderr, "tt: unable to POST data: curl_strerror: %s\n", curl_easy_strerror(status));
		assert(status == 0);
		/* unreached */
		return (false);
	}
	curl_easy_getinfo(cu, CURLINFO_RESPONSE_CODE, &code);
	if (code != 200) {
		fprintf(stderr, "tt: error: server responded to POST with code %ld: %s\n",
			code, buf);
		return (false);
	}

	return (true);
}

CURL *curlsetup(const char *url)
{
	CURL *cu;
	struct WriteThis wt;

	wt.readptr = malloc(5120);
	wt.sizeleft = 5120;


	cu = curl_easy_init();

	curl_easy_setopt(cu, CURLOPT_POST, 1L);
	curl_easy_setopt(cu, CURLOPT_URL, url);
	curl_easy_setopt(cu, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(cu, CURLOPT_VERBOSE, 0L);

	curl_easy_setopt(cu, CURLOPT_READFUNCTION, read_callback);
	curl_easy_setopt(cu, CURLOPT_READDATA, &wt);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charset: utf-8");

	curl_easy_setopt(cu, CURLOPT_HTTPHEADER, headers);

	return (cu);
}


int main(int argc, char **argv)
{
	BSJ *job;
	CURL *cu;
	bool bf;
	int handle = bs_connect("127.0.0.1", 11300);

	fprintf(stderr, "%s: starting. Sleeping 5s\n", *argv);
	sleep(5);

	assert(handle != BS_STATUS_FAIL);

	assert(bs_watch(handle, TUBENAME) == BS_STATUS_OK);
	assert(bs_ignore(handle, "default") == BS_STATUS_OK);

	if ((cu = curlsetup(URL)) == NULL) {
		puts("MEH");
		return -1;
	}

	while (bs_reserve(handle, &job) == BS_STATUS_OK) {
		assert(job);

		/*
		printf("reserve job id: %" PRId64 " size: %lu\n", job->id, job->size);
		write(fileno(stderr), job->data, job->size);
		write(fileno(stderr), "\r\n", 2);
		*/

		/*
		printf("%s\n", job->data);
		*/

		bf = upload(cu, job->data);

		if (bf == true) {
			// printf("delete job id: %" PRId64 "\n", job->id);
			assert(bs_delete(handle, job->id) == BS_STATUS_OK);
		} else {
			assert(bs_bury(handle, job->id, 0) == BS_STATUS_OK);
		}
		bs_free_job(job);
	}

	bs_disconnect(handle);
	exit(0);
}
