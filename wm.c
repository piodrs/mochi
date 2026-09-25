#define _POSIX_C_SOURCE 200809L

#include "wm.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>

#include <poll.h>

#include <X11/Xlib.h>

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
#include "x11.h"

enum { WM_RUNNING = -1 };

Session mochi;

static volatile sig_atomic_t stopped;

static void wm_stop(int sig)
{
	stopped = sig;
}

bool wm_split(bool vertical)
{
	if (!frame_split(mochi.frame, vertical)) {
		input_message("Cannot split this frame");
		return false;
	}
	mochi.frame = mochi.frame->first;
	client_refresh();
	return true;
}

void wm_frame(int direction)
{
	mochi.frame = frame_next(mochi.tree, mochi.frame, direction);
	client_refresh();
	input_message(mochi.frame->win ? "Selected frame" : "Empty frame");
}

void wm_remove(void)
{
	mochi.frame = frame_remove(&mochi.tree, mochi.frame);
	client_refresh();
}

void wm_only(void)
{
	frame_only(&mochi.tree, mochi.frame);
	client_refresh();
}

void wm_quit(bool restart)
{
	mochi.status = restart ? WM_RESTART : 0;
}

int wm_run(void)
{
	XWindowAttributes attr;
	struct sigaction action = {.sa_handler = wm_stop};

	mochi.status = 1;
	if (!x11_open())
		return 1;
	mochi.tree = frame_create();
	if (!mochi.tree || !display_init()) {
		fprintf(stderr, APP_NAME ": initialization failed\n");
		goto close_display;
	}
	mochi.frame = mochi.tree;
	if (!XGetWindowAttributes(mochi.display, mochi.root, &attr))
		goto free_display;
	frame_layout(mochi.tree, 0, 0, attr.width, attr.height);
	sigemptyset(&action.sa_mask);
	bool signals_failed = sigaction(SIGTERM, &action, NULL) < 0 ||
		sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGHUP, &action, NULL) < 0;
	action.sa_handler = SIG_DFL;
	if (sigaction(SIGCHLD, &action, NULL) < 0 || signals_failed) {
		perror(APP_NAME ": signals");
		goto free_display;
	}
	mochi.status = WM_RUNNING;
	client_scan();
	if (mochi.status != WM_RUNNING)
		goto free_clients;
	client_refresh();
	bool keys_ready = keys_init();
	if (!config_load(true) && !keys_ready) {
		fprintf(stderr, APP_NAME ": cannot grab prefix key\n");
		mochi.status = 1;
	}
	struct pollfd fd = {.fd = ConnectionNumber(mochi.display), .events = POLLIN};
	while (mochi.status == WM_RUNNING && !stopped) {
		int count = 0;
		while (mochi.status == WM_RUNNING && !stopped && count++ < EVENT_BATCH &&
			XPending(mochi.display)) {
			XEvent event;
			XNextEvent(mochi.display, &event);
			event_dispatch(&event);
		}
		process_reap();
		input_tick();
		XFlush(mochi.display);
		if (mochi.status != WM_RUNNING || stopped)
			break;
		int status = poll(&fd, 1, XPending(mochi.display) ? 0 : POLL_MS);
		if (status < 0 && errno != EINTR) {
			mochi.status = 1;
			break;
		}
		if (status > 0 && (fd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
			mochi.status = 1;
			break;
		}
	}

free_clients:
	input_cancel();
	keys_free();
	client_free();
	process_free();
	XSetInputFocus(mochi.display, PointerRoot, RevertToPointerRoot, CurrentTime);

free_display:
	display_free();
close_display:
	frame_free(mochi.tree);
	XCloseDisplay(mochi.display);
	return mochi.status == WM_RUNNING ? 0 : mochi.status;
}
