#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "client.h"
#include "config.h"
#include "defs.h"
#include "display.h"
#include "event.h"
#include "frame.h"
#include "input.h"
#include "keys.h"
#include "process.h"
#include "session.h"
#include "wm.h"
#include "x11.h"

static volatile sig_atomic_t stopped;

static void stop(int sig) { stopped = sig; }

int wm_split(int vertical)
{
	if (!frame_split(fish.frame, vertical)) {
		input_message("Cannot split this frame");
		return FALSE;
	}
	fish.frame = fish.frame->first;
	client_refresh();
	return TRUE;
}

void wm_frame(int direction)
{
	fish.frame = frame_next(fish.tree, fish.frame, direction);
	client_refresh();
	input_message(fish.frame->win ? "Selected frame" : "Empty frame");
}

int wm_remove(void)
{
	int w;
	int h;

	w = fish.tree->width;
	h = fish.tree->height;
	fish.frame = frame_remove(&fish.tree, fish.frame);
	frame_layout(fish.tree, 0, 0, w, h);
	client_refresh();
	return TRUE;
}

int wm_only(void)
{
	int w;
	int h;

	w = fish.tree->width;
	h = fish.tree->height;
	frame_only(&fish.tree, fish.frame);
	frame_layout(fish.tree, 0, 0, w, h);
	client_refresh();
	return TRUE;
}

void wm_quit(int restart)
{
	fish.restart = restart;
	fish.running = FALSE;
}

int wm_run(void)
{
	XWindowAttributes attr;
	XEvent event;
	struct pollfd fd;
	struct sigaction action;
	int status;
	int count;
	int result;

	result = 1;
	if (!x11_open())
		return 1;
	fish.tree = frame_create();
	if (!fish.tree || !display_init()) {
		fprintf(stderr, APP_NAME ": initialization failed\n");
		goto close_display;
	}
	fish.frame = fish.tree;
	if (!XGetWindowAttributes(fish.display, fish.root, &attr))
		goto free_display;
	frame_layout(fish.tree, 0, 0, attr.width, attr.height);
	memset(&action, 0, sizeof action);
	sigemptyset(&action.sa_mask);
	action.sa_handler = stop;
	status = sigaction(SIGTERM, &action, NULL) < 0 ||
		 sigaction(SIGINT, &action, NULL) < 0 ||
		 sigaction(SIGHUP, &action, NULL) < 0;
	action.sa_handler = SIG_DFL;
	if (sigaction(SIGCHLD, &action, NULL) < 0 || status) {
		perror(APP_NAME ": signals");
		goto free_display;
	}
	fish.running = TRUE;
	result = 0;
	client_scan();
	client_refresh();
	status = keys_init();
	if (!config_load(TRUE) && !status) {
		fprintf(stderr, APP_NAME ": cannot grab prefix key\n");
		fish.running = FALSE;
		result = 1;
	}
	fd.fd = ConnectionNumber(fish.display);
	fd.events = POLLIN;
	while (fish.running && !stopped) {
		count = 0;
		while (fish.running && !stopped && count++ < EVENT_BATCH &&
		       XPending(fish.display)) {
			XNextEvent(fish.display, &event);
			event_dispatch(&event);
		}
		process_reap();
		input_tick();
		XFlush(fish.display);
		if (!fish.running || stopped)
			break;
		status = poll(&fd, 1, XPending(fish.display) ? 0 : POLL_MS);
		if (status < 0 && errno != EINTR) {
			result = 1;
			break;
		}
		if (status > 0 &&
		    (fd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
			result = 1;
			break;
		}
	}
	input_cancel();
	keys_free();
	client_free();
	process_free();
	XSetInputFocus(fish.display, PointerRoot, RevertToPointerRoot,
		       CurrentTime);

free_display:
	display_free();
close_display:
	frame_free(fish.tree);
	XCloseDisplay(fish.display);
	return fish.restart ? WM_RESTART : result;
}
