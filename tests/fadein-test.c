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

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

char *server_parameters="--use-pixman --width=320 --height=240";

TEST(fadein)
{
	struct client *client;
	char *out_path;
	char *ref_path;
	int i;
	uint32_t head_number = 0;

	client = client_create(100, 100, 100, 100);
	assert(client);

	for (i = 0; i < 6; i++) {
		out_path = screenshot_output_filename(testfadein.name,
						      i, head_number);

		/* Use a thin 40xN vertical strip for comparison */
		wl_test_record_screenshot(client->test->wl_test,
					  out_path, head_number,
					  80, 40, 40, DISPLAY_HEIGHT-80);
		client_roundtrip(client);

		// We skip testing the 2nd step of the fade in because the timing
		// can result in significantly different colors from run to run
		if (i == 2) {
			// At t=0.5, the screen fade colors are changing rapidly,
			// so just verify the screen is different than at t=0.25.
			ref_path = screenshot_reference_filename(testfadein.name,
								 1, head_number);
			assert(! files_equal(out_path, ref_path));
		} else {
			ref_path = screenshot_reference_filename(testfadein.name,
								 i, head_number);
			assert(files_equal(out_path, ref_path));
		}

		free (out_path);
		free (ref_path);

		usleep(250000);
	}
}
