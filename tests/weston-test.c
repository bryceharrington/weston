/*
 * Copyright © 2012 Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include "../src/compositor.h"
#include "wayland-test-server-protocol.h"

#ifdef ENABLE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif /* ENABLE_EGL */

#include <cairo.h>

struct weston_test {
	struct weston_compositor *compositor;
	struct weston_layer layer;
	struct weston_process process;
};

struct weston_test_surface {
	struct weston_surface *surface;
	struct weston_view *view;
	int32_t x, y;
	struct weston_test *test;
};

static void
test_client_sigchld(struct weston_process *process, int status)
{
	struct weston_test *test =
		container_of(process, struct weston_test, process);

	/* Chain up from weston-test-runner's exit code so that automake
	 * knows the exit status and can report e.g. skipped tests. */
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		exit(WEXITSTATUS(status));

	/* In case the child aborted or segfaulted... */
	assert(status == 0);

	wl_display_terminate(test->compositor->wl_display);
}

static struct weston_seat *
get_seat(struct weston_test *test)
{
	struct wl_list *seat_list;
	struct weston_seat *seat;

	seat_list = &test->compositor->seat_list;
	assert(wl_list_length(seat_list) == 1);
	seat = container_of(seat_list->next, struct weston_seat, link);

	return seat;
}

static void
notify_pointer_position(struct weston_test *test, struct wl_resource *resource)
{
	struct weston_seat *seat = get_seat(test);
	struct weston_pointer *pointer = seat->pointer;

	wl_test_send_pointer_position(resource, pointer->x, pointer->y);
}

static void
test_surface_configure(struct weston_surface *surface, int32_t sx, int32_t sy)
{
	struct weston_test_surface *test_surface = surface->configure_private;
	struct weston_test *test = test_surface->test;

	if (wl_list_empty(&test_surface->view->layer_link.link))
		weston_layer_entry_insert(&test->layer.view_list,
					  &test_surface->view->layer_link);

	weston_view_set_position(test_surface->view,
				 test_surface->x, test_surface->y);

	weston_view_update_transform(test_surface->view);
}

static void
move_surface(struct wl_client *client, struct wl_resource *resource,
	     struct wl_resource *surface_resource,
	     int32_t x, int32_t y)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_test_surface *test_surface;

	test_surface = surface->configure_private;
	if (!test_surface) {
		test_surface = malloc(sizeof *test_surface);
		if (!test_surface) {
			wl_resource_post_no_memory(resource);
			return;
		}

		test_surface->view = weston_view_create(surface);
		if (!test_surface->view) {
			wl_resource_post_no_memory(resource);
			free(test_surface);
			return;
		}

		surface->configure_private = test_surface;
		surface->configure = test_surface_configure;
	}

	test_surface->surface = surface;
	test_surface->test = wl_resource_get_user_data(resource);
	test_surface->x = x;
	test_surface->y = y;
}

static void
move_pointer(struct wl_client *client, struct wl_resource *resource,
	     int32_t x, int32_t y)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);
	struct weston_pointer *pointer = seat->pointer;

	notify_motion(seat, 100,
		      wl_fixed_from_int(x) - pointer->x,
		      wl_fixed_from_int(y) - pointer->y);

	notify_pointer_position(test, resource);
}

static void
send_button(struct wl_client *client, struct wl_resource *resource,
	    int32_t button, uint32_t state)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);

	notify_button(seat, 100, button, state);
}

static void
activate_surface(struct wl_client *client, struct wl_resource *resource,
		 struct wl_resource *surface_resource)
{
	struct weston_surface *surface = surface_resource ?
		wl_resource_get_user_data(surface_resource) : NULL;
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat;

	seat = get_seat(test);

	if (surface) {
		weston_surface_activate(surface, seat);
		notify_keyboard_focus_in(seat, &seat->keyboard->keys,
					 STATE_UPDATE_AUTOMATIC);
	}
	else {
		notify_keyboard_focus_out(seat);
		weston_surface_activate(surface, seat);
	}
}

static void
send_key(struct wl_client *client, struct wl_resource *resource,
	 uint32_t key, enum wl_keyboard_key_state state)
{
	struct weston_test *test = wl_resource_get_user_data(resource);
	struct weston_seat *seat = get_seat(test);

	notify_key(seat, 100, key, state, STATE_UPDATE_AUTOMATIC);
}

#ifdef ENABLE_EGL
static int
is_egl_buffer(struct wl_resource *resource)
{
	PFNEGLQUERYWAYLANDBUFFERWL query_buffer =
		(void *) eglGetProcAddress("eglQueryWaylandBufferWL");
	EGLint format;

	if (query_buffer(eglGetCurrentDisplay(),
			 resource,
			 EGL_TEXTURE_FORMAT,
			 &format))
		return 1;

	return 0;
}
#endif /* ENABLE_EGL */

static void
get_n_buffers(struct wl_client *client, struct wl_resource *resource)
{
	int n_buffers = 0;

#ifdef ENABLE_EGL
	struct wl_resource *buffer_resource;
	int i;

	for (i = 0; i < 1000; i++) {
		buffer_resource = wl_client_get_object(client, i);

		if (buffer_resource == NULL)
			continue;

		if (is_egl_buffer(buffer_resource))
			n_buffers++;
	}
#endif /* ENABLE_EGL */

	wl_test_send_n_egl_buffers(resource, n_buffers);
}

static void
dump_image(const char *filename, int x, int y, uint32_t *image,
	   unsigned int clip_x, unsigned int clip_y,
	   unsigned int clip_width, unsigned int clip_height)
{
	cairo_surface_t *surface, *flipped;
	cairo_t *cr;

	surface = cairo_image_surface_create_for_data((unsigned char *)image,
						      CAIRO_FORMAT_ARGB32,
						      x, y, x * 4);
	flipped = cairo_surface_create_similar_image(surface, CAIRO_FORMAT_ARGB32, x, y);

	cr = cairo_create(flipped);

	if (clip_width > 0 && clip_height > 0) {
		/* Clip out a rectangular area, e.g. to ignore desktop bars, etc. */
		cairo_move_to(cr, clip_x, clip_y);
		cairo_rel_line_to(cr, clip_width, 0);
		cairo_rel_line_to(cr, 0, clip_height);
		cairo_rel_line_to(cr, -clip_width, 0);
		cairo_rel_line_to(cr, 0, -clip_height);
		cairo_close_path(cr);

		cairo_clip(cr);
	}

	cairo_translate(cr, 0.0, y);
	cairo_scale(cr, 1.0, -1.0);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(surface);

	cairo_surface_write_to_png(flipped, filename);
	cairo_surface_destroy(flipped);
}

static void
record_screenshot(struct wl_client *client, struct wl_resource *resource,
		  const char *filename, unsigned int output_number,
		  unsigned int clip_x, unsigned int clip_y,
		  unsigned int clip_width, unsigned int clip_height)
{
	struct weston_output *o;
	struct weston_test *test = wl_resource_get_user_data(resource);
	uint32_t *buffer;
	int w, h = 0;
	unsigned int i = 0;

	assert(output_number < (unsigned int)wl_list_length(&test->compositor->output_list));
	wl_list_for_each(o, &test->compositor->output_list, link) {
		if (i == output_number)
			break;
		i++;
	}
	assert(i == output_number);

	switch (o->transform) {
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		w = o->height;
		h = o->width;
		break;
	default:
		w = o->width;
		h = o->height;
		break;
	}
	buffer = malloc(w * h * 4);
	if (!buffer)
		return;

	test->compositor->renderer->read_pixels(o,
						o->compositor->read_format,
						buffer, 0, 0, w, h);

	dump_image(filename, w, h, buffer,
		   clip_x, clip_y, clip_width, clip_height);
	free(buffer);
}

static const struct wl_test_interface test_implementation = {
	move_surface,
	move_pointer,
	send_button,
	activate_surface,
	send_key,
	get_n_buffers,
	record_screenshot
};

static void
bind_test(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct weston_test *test = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_test_interface, 1, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource,
				       &test_implementation, test, NULL);

	notify_pointer_position(test, resource);
}

static void
idle_launch_client(void *data)
{
	struct weston_test *test = data;
	pid_t pid;
	sigset_t allsigs;
	char *path;

	path = getenv("WESTON_TEST_CLIENT_PATH");
	if (path == NULL)
		return;
	pid = fork();
	if (pid == -1)
		exit(EXIT_FAILURE);
	if (pid == 0) {
		sigfillset(&allsigs);
		sigprocmask(SIG_UNBLOCK, &allsigs, NULL);
		execl(path, path, NULL);
		weston_log("compositor: executing '%s' failed: %m\n", path);
		exit(EXIT_FAILURE);
	}

	test->process.pid = pid;
	test->process.cleanup = test_client_sigchld;
	weston_watch_process(&test->process);
}

WL_EXPORT int
module_init(struct weston_compositor *ec,
	    int *argc, char *argv[])
{
	struct weston_test *test;
	struct wl_event_loop *loop;

	test = zalloc(sizeof *test);
	if (test == NULL)
		return -1;

	test->compositor = ec;
	weston_layer_init(&test->layer, &ec->cursor_layer.link);

	if (wl_global_create(ec->wl_display, &wl_test_interface, 1,
			     test, bind_test) == NULL)
		return -1;

	loop = wl_display_get_event_loop(ec->wl_display);
	wl_event_loop_add_idle(loop, idle_launch_client, test);

	return 0;
}
