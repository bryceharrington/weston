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
#include <string.h> // memcpy
#include <cairo.h>

#include "weston-test-client-helper.h"

char *server_parameters="--use-pixman --width=320 --height=240";
int buffer_copy_done;

// TODO: Isn't this already available from a header?
static void *
xmalloc(size_t size)
{
        void *p;

        p = malloc(size);
	if (p == NULL) {
                fprintf(stderr, "out of memory\n");
                exit(EXIT_FAILURE);
        }

        return p;
}

/*
 * Does the test infrastructure take care of hooking this up for us?

static void
screenshot_done(void *data, struct wl_test *test)
{
	buffer_copy_done = 1;
}

static const struct wl_test_listener wl_test_listener = {
        screenshot_done
};
*/

TEST(fadein)
{
	struct client *client;
	struct surface *screenshot = NULL;
	struct surface *reference = NULL;
	struct rectangle clip;
	bool match = false;
	bool dump_all_images = true;

	/* Create the client */
	client = client_create(100, 100, 100, 100);
	assert(client);
	printf("Client created\n");

	/* Create a surface to hold the screenshot */
	screenshot = xzalloc(sizeof *screenshot);
	screenshot->wl_surface = wl_compositor_create_surface(client->wl_compositor);
	assert(screenshot->wl_surface);
	printf("Screenshot surface created\n");

	/* Create and attach buffer to our surface */
	screenshot->wl_buffer = create_shm_buffer(client,
						  client->output->width,
						  client->output->height,
						  &screenshot->data);
	assert(screenshot->wl_buffer);
	printf("Screenshot buffer created and attached to surface\n");

	usleep(750000);

	/* Take a snapshot.  Result will be in screenshot->wl_buffer. */
	wl_test_capture_screenshot(client->test->wl_test,
				   client->output->wl_output,
				   screenshot->wl_buffer);
	printf("Capture request sent\n");
	client_roundtrip(client);
	printf("Roundtrip done\n");

	// TODO: Wait for the done event instead of just sleeping...
	usleep(1000000);

	clip.x = 0;
	clip.y = 0;
	clip.width = client->output->width;
	clip.height = client->output->height;

	printf("Clip: %d,%d %d x %d\n", clip.x, clip.y, clip.width, clip.height);

	match = check_match(screenshot, &clip, reference);
	if (!match || dump_all_images) {
		/* Write image to .png file */
		int output_stride, buffer_stride, i;
		cairo_surface_t *surface;
		void *data, *d, *s;

		printf("Writing image to png file\n");

		buffer_stride = client->output->width * 4;

		data = xmalloc(buffer_stride * client->output->height);
		assert(data);

		output_stride = client->output->width * 4;
		s = screenshot->data;
		d = data;

		for (i = 0; i < client->output->height; i++) {
			memcpy(d, s, output_stride);
			d += buffer_stride;
			s += output_stride;
		}

		surface = cairo_image_surface_create_for_data(data,
							      CAIRO_FORMAT_ARGB32,
							      client->output->width,
							      client->output->height,
							      buffer_stride);
		cairo_surface_write_to_png(surface, "clientside-screenshot.png");
		cairo_surface_destroy(surface);
		free(data);
	}
	//assert(match);
	printf("Test complete\n");
}
