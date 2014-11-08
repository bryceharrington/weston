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

char *server_parameters="--use-pixman --width=320 --height=240";

static char*
output_filename(const char* basename, int head) {
	static const char *path = "./";
	char *filename;

        if (asprintf(&filename, "%s%s-%d.png", path, basename, head) < 0)
		filename = NULL;

	return filename;
}

static char*
reference_filename(const char* basename, int head) {
        static const char *path = "./tests/reference/";
        char *filename;

        if (asprintf(&filename, "%s%s-%d.png", path, basename, head) < 0)
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

	for (i = 0; i < 6; i++) {
		snprintf(basename, sizeof basename, "fadein-%02d", i);
		// FIXME: Iterate over all heads
		out_path = output_filename(basename, 0);
		ref_path = reference_filename(basename, 0);

		// FIXME: Would be preferred to pass in out_path rather than basename here...
		wl_test_record_screenshot(client->test->wl_test, basename);
		client_roundtrip(client);
		if (i == 0) {
			if (files_equal(out_path, ref_path))
				printf("%s is correct\n", out_path);
			else
				printf("%s doesn't match reference %s\n", out_path, ref_path);
		}
		free (out_path);
		free (ref_path);

		usleep(250000);
	}

}
