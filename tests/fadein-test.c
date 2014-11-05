/*
 * Copyright © 2014 Samsung
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <unistd.h>
#include <stdio.h>

#include "weston-test-client-helper.h"

char *server_parameters="--use-pixman --width=320 --height=240";

static char*
output_filename(const char* basename) {
	static const char *path = "./";
	char *filename;

        if (asprintf(&filename, "%s%s", path, basename) < 0)
		filename = NULL;

	return filename;
}

TEST(fadein)
{
	struct client *client;
	char basename[32];
	char *out_path;
	int i;

	client = client_create(100, 100, 100, 100);
	assert(client);

	for (i = 0; i < 6; i++) {
		snprintf(basename, sizeof basename, "fadein-%02d", i);
		out_path = output_filename(basename);

		wl_test_record_screenshot(client->test->wl_test, out_path);
		client_roundtrip(client);
		free (out_path);

		usleep(250000);
	}

}
