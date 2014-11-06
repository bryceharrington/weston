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
#include <stdbool.h>

#include "weston-test-client-helper.h"

char *server_parameters="--use-pixman";

static char*
output_filename(const char* basename) {
	static const char *path = "./";
	static const char *ext = "ppm";
	char *filename;

        if (asprintf(&filename, "%s%s.%s", path, basename, ext) < 0)
		filename = NULL;

	return filename;
}

static char*
reference_filename(const char* basename) {
	static const char *path = "./tests/reference/";
	static const char *ext = "ppm";
	char *filename;

        if (asprintf(&filename, "%s%s.%s", path, basename, ext) < 0)
		filename = NULL;

	return filename;
}

static bool
files_equal(const char *test_filename, const char* ref_filename)
{
	FILE *test, *ref;
	int t, p;

	if (test_filename == NULL || ref_filename == NULL)
		return false;

	test = fopen (test_filename, "rb");
	if (test == NULL)
		return false;

	ref = fopen (ref_filename, "rb");
	if (ref == NULL) {
		fclose (test);
		return false;
	}

	do {
		t = getc (test);
		p = getc (ref);
		if (t != p)
			break;
	} while (t != EOF && p != EOF);

	fclose (test);
	fclose (ref);

	return t == p;  /* both EOF */
}

TEST(headless)
{
	struct client *client;
	char basename[32];
	char *out_path;
	char *ref_path;
	int i;

	client = client_create(100, 100, 100, 100);
	assert(client);

	for (i=0; i<=12; i++) {
		snprintf(basename, sizeof basename, "fadein-%02d", i);
		out_path = output_filename(basename);
		ref_path = reference_filename(basename);

		wl_test_record_screenshot(client->test->wl_test, out_path);
		client_roundtrip(client);
		if (i == 0) {
			/* FIXME:  The clock will of course differ from
			 * when the reference images were generated, which
			 * will count as a failure since the files won't
			 * be identical.  So for now, only look at the
			 * very first frame, which will be black.
			 *
			 * Ultimately we should mask out the clock or set
			 * it to a fixed time, so the comparison works.
			 */
			assert(files_equal(out_path, ref_path));
		}

		sleep(.25);
	}

	free (out_path);
	free (ref_path);
}
