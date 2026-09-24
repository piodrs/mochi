#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "session.h"
#include "x11.h"

int trapping;
int failed;
Atom x11_protocols;
Atom wm_state;
Atom change_state;
Atom timestamp;

int xerror(Display *display, XErrorEvent *event)
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
	fprintf(stderr, APP_NAME ": X11: %s (request %u)\n", text,
		event->request_code);
	return 0;
}

void x11_trap(void)
{
	XSync(fish.display, False);
	failed = 0;
	trapping = TRUE;
}

int x11_untrap(void)
{
	XSync(fish.display, False);
	trapping = FALSE;
	return failed;
}

int x11_open(void)
{
	fish.display = XOpenDisplay(NULL);
	if (!fish.display) {
		fprintf(stderr, APP_NAME ": cannot open display\n");
		return FALSE;
	}
	fish.root = DefaultRootWindow(fish.display);
	XSetErrorHandler(xerror);
	x11_trap();
	XSelectInput(fish.display, fish.root,
		     SubstructureRedirectMask | SubstructureNotifyMask |
			     StructureNotifyMask);
	if (x11_untrap()) {
		fprintf(stderr,
			APP_NAME ": another window manager owns this screen\n");
		XCloseDisplay(fish.display);
		return FALSE;
	}
	x11_protocols = XInternAtom(fish.display, "WM_PROTOCOLS", False);
	wm_state = XInternAtom(fish.display, "WM_STATE", False);
	change_state = XInternAtom(fish.display, "WM_CHANGE_STATE", False);
	timestamp = XInternAtom(fish.display, "_FISH_TIMESTAMP", False);
	return TRUE;
}

Time x11_time(Window win)
{
	XEvent event;

	XChangeProperty(fish.display, win, timestamp, XA_INTEGER, 8,
			PropModeReplace, NULL, 0);
	XWindowEvent(fish.display, win, PropertyChangeMask, &event);
	return event.xproperty.time;
}

int x11_iconic(XClientMessageEvent *event)
{
	return event->message_type == change_state && event->format == 32 &&
	       event->data.l[0] == IconicState;
}

int x11_hidden(Window win)
{
	Atom type;
	int format;
	unsigned long count;
	unsigned long rest;
	unsigned char *data;
	int hidden;

	data = NULL;
	hidden = FALSE;
	if (XGetWindowProperty(fish.display, win, wm_state, 0, 2, False,
			       wm_state, &type, &format, &count, &rest,
			       &data) == Success &&
	    type == wm_state && format == 32 && count == 2)
		hidden = ((long *)data)[0] == IconicState;
	if (data)
		XFree(data);
	return hidden;
}

void x11_state(Window win, long value)
{
	long data[2];

	data[0] = value;
	data[1] = None;
	XChangeProperty(fish.display, win, wm_state, wm_state, 32,
			PropModeReplace, (unsigned char *)data, 2);
}

void x11_protocol(Window win, Atom atom)
{
	XEvent event;

	memset(&event, 0, sizeof event);
	event.xclient.type = ClientMessage;
	event.xclient.window = win;
	event.xclient.message_type = x11_protocols;
	event.xclient.format = 32;
	event.xclient.data.l[0] = atom;
	event.xclient.data.l[1] = fish.time;
	XSendEvent(fish.display, win, False, NoEventMask, &event);
}
