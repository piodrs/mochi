#include "x11.h"

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "defs.h"
#include "session.h"

static bool trapping;
static int failed;
static Atom x11_protocols;
static Atom wm_state;
static Atom change_state;
static Atom timestamp;

static int x11_error(Display *display, XErrorEvent *event)
{
	char text[256];

	if (trapping) {
		failed = event->error_code;
		return 0;
	}
	if (event->error_code == BadWindow || event->error_code == BadMatch ||
		event->error_code == BadDrawable)
		return 0;
	XGetErrorText(display, event->error_code, text, sizeof text);
	fprintf(stderr, APP_NAME ": X11: %s (request %u)\n", text, event->request_code);
	return 0;
}

void x11_trap(void)
{
	XSync(mochi.display, False);
	failed = 0;
	trapping = true;
}

int x11_untrap(void)
{
	XSync(mochi.display, False);
	trapping = false;
	return failed;
}

bool x11_open(void)
{
	mochi.display = XOpenDisplay(NULL);
	if (!mochi.display) {
		fprintf(stderr, APP_NAME ": cannot open display\n");
		return false;
	}
	mochi.root = DefaultRootWindow(mochi.display);
	XSetErrorHandler(x11_error);
	x11_trap();
	XSelectInput(mochi.display, mochi.root,
		SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask);
	if (x11_untrap()) {
		fprintf(stderr, APP_NAME ": another window manager owns this screen\n");
		XCloseDisplay(mochi.display);
		return false;
	}
	x11_protocols = XInternAtom(mochi.display, "WM_PROTOCOLS", False);
	wm_state = XInternAtom(mochi.display, "WM_STATE", False);
	change_state = XInternAtom(mochi.display, "WM_CHANGE_STATE", False);
	timestamp = XInternAtom(mochi.display, "_MOCHIWM_TIMESTAMP", False);
	return true;
}

Time x11_time(Window win)
{
	XEvent event;

	XChangeProperty(mochi.display, win, timestamp, XA_INTEGER, 8, PropModeReplace, NULL, 0);
	XWindowEvent(mochi.display, win, PropertyChangeMask, &event);
	return event.xproperty.time;
}

bool x11_iconic(const XClientMessageEvent *event)
{
	return event->message_type == change_state && event->format == 32 &&
		event->data.l[0] == IconicState;
}

bool x11_hidden(Window win)
{
	Atom type;
	int format;
	unsigned long count;
	unsigned long rest;

	unsigned char *data = NULL;
	bool hidden = false;
	if (XGetWindowProperty(mochi.display, win, wm_state, 0, 2, False, wm_state, &type, &format,
		    &count, &rest, &data) == Success &&
		type == wm_state && format == 32 && count == 2)
		hidden = ((long *)data)[0] == IconicState;
	if (data)
		XFree(data);
	return hidden;
}

void x11_state(Window win, long value)
{
	long data[2] = {value, None};
	XChangeProperty(mochi.display, win, wm_state, wm_state, 32, PropModeReplace,
		(unsigned char *)data, 2);
}

void x11_protocol(Window win, Atom atom)
{
	XEvent event = {
		.xclient = {
			.type = ClientMessage,
			.window = win,
			.message_type = x11_protocols,
			.format = 32,
			.data = {.l = {(long)atom, (long)mochi.time}}
		}
	};
	XSendEvent(mochi.display, win, False, NoEventMask, &event);
}
