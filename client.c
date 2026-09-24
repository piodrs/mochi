#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "client.h"
#include "defs.h"
#include "display.h"
#include "frame.h"
#include "input.h"
#include "session.h"
#include "x11.h"

typedef struct Client Client;

struct Client {
	Client *next;
	Window win;
	Window transient;
	unsigned long id;
	int mapped;
	int input;
	int take_focus;
	int delete_window;
	unsigned int pending;
	int border;
	int width;
	int height;
};

Client *clients;
unsigned long next_id;
Atom client_protocols;
Atom take_focus;
Atom delete_window;

Client *client_find(Window win)
{
	Client *cp;

	for (cp = clients; cp; cp = cp->next)
		if (cp->win == win)
			return cp;
	return NULL;
}

void properties(Client *cp)
{
	XWMHints *hints;
	Atom *list;
	int count;
	int i;

	hints = XGetWMHints(mochi.display, cp->win);
	cp->input = !hints || !(hints->flags & InputHint) || hints->input;
	if (hints)
		XFree(hints);
	cp->take_focus = FALSE;
	cp->delete_window = FALSE;
	if (XGetWMProtocols(mochi.display, cp->win, &list, &count)) {
		for (i = 0; i < count; ++i) {
			if (list[i] == take_focus)
				cp->take_focus = TRUE;
			if (list[i] == delete_window)
				cp->delete_window = TRUE;
		}
		XFree(list);
	}
}

Frame *client_frame(Client *cp)
{
	Frame *fp;

	while (cp) {
		fp = frame_find(mochi.tree, cp->win);
		if (fp)
			return fp;
		cp = client_find(cp->transient);
	}
	return NULL;
}

Client *focused(void)
{
	Client *cp;
	Client *selected;

	selected = client_find(mochi.frame->win);
	for (cp = clients; cp; cp = cp->next)
		if (cp->transient && client_frame(cp) == mochi.frame)
			selected = cp;
	return selected;
}

void configure(Client *cp, Frame *fp)
{
	XEvent event;
	int x;
	int y;
	int w;
	int h;

	w = fp->width > 0 ? fp->width : 1;
	h = fp->height > 0 ? fp->height : 1;
	if (cp->transient) {
		if (cp->width < w)
			w = cp->width;
		if (cp->height < h)
			h = cp->height;
	}
	x = fp->x + (fp->width - w) / 2;
	y = fp->y + (fp->height - h) / 2;
	XMoveResizeWindow(mochi.display, cp->win, x, y, w, h);
	memset(&event, 0, sizeof event);
	event.xconfigure.type = ConfigureNotify;
	event.xconfigure.display = mochi.display;
	event.xconfigure.event = cp->win;
	event.xconfigure.window = cp->win;
	event.xconfigure.x = x;
	event.xconfigure.y = y;
	event.xconfigure.width = w;
	event.xconfigure.height = h;
	event.xconfigure.above = None;
	XSendEvent(mochi.display, cp->win, False, StructureNotifyMask, &event);
}

void client_refresh(void)
{
	Client *cp;
	Client *selected;
	Frame *fp;

	frame_layout(mochi.tree, 0, 0, mochi.tree->width, mochi.tree->height);
	for (cp = clients; cp; cp = cp->next) {
		fp = client_frame(cp);
		if (fp) {
			configure(cp, fp);
			if (!cp->mapped) {
				XMapWindow(mochi.display, cp->win);
				cp->mapped = TRUE;
				x11_state(cp->win, NormalState);
			}
		} else if (cp->mapped) {
			++cp->pending;
			XUnmapWindow(mochi.display, cp->win);
			cp->mapped = FALSE;
			x11_state(cp->win, IconicState);
		}
	}
	selected = focused();
	mochi.time = x11_time(display_window());
	XSetInputFocus(mochi.display,
		       selected && selected->input ? selected->win : mochi.root,
		       RevertToPointerRoot, mochi.time);
	if (selected) {
		XRaiseWindow(mochi.display, selected->win);
		if (selected->take_focus)
			x11_protocol(selected->win, take_focus);
	}
	input_draw();
	XRaiseWindow(mochi.display, display_window());
	XFlush(mochi.display);
}

void show(Client *cp)
{
	Frame *fp;
	Client *parent;

	while ((parent = client_find(cp->transient)) != NULL)
		cp = parent;
	fp = frame_find(mochi.tree, cp->win);
	if (fp)
		mochi.frame = fp;
	else
		mochi.frame->win = cp->win;
}

void manage(Window win, int select)
{
	XWindowAttributes attr;
	Client *cp;
	Client **tail;

	if (win == display_window() || client_find(win))
		return;
	if (!XGetWindowAttributes(mochi.display, win, &attr) ||
	    attr.override_redirect || attr.class == InputOnly)
		return;
	cp = calloc(1, sizeof *cp);
	if (!cp) {
		fprintf(stderr, APP_NAME ": out of memory\n");
		mochi.status = 1;
		return;
	}
	cp->win = win;
	cp->id = ++next_id;
	cp->mapped = attr.map_state != IsUnmapped;
	cp->border = attr.border_width;
	cp->width = attr.width;
	cp->height = attr.height;
	XGetTransientForHint(mochi.display, win, &cp->transient);
	if (!client_find(cp->transient))
		cp->transient = None;
	properties(cp);
	for (tail = &clients; *tail; tail = &(*tail)->next)
		;
	*tail = cp;
	XAddToSaveSet(mochi.display, win);
	XSelectInput(mochi.display, win, PropertyChangeMask);
	XSetWindowBorderWidth(mochi.display, win, 0);
	x11_state(cp->win, cp->mapped ? NormalState : IconicState);
	if (!cp->transient && (select || !mochi.frame->win))
		mochi.frame->win = win;
	else if (cp->transient && select)
		show(cp);
}

void forget(Client *cp, int destroyed)
{
	Client **link;
	Client *other;
	Frame *fp;

	fp = frame_find(mochi.tree, cp->win);
	if (fp)
		fp->win = None;
	for (link = &clients; *link != cp; link = &(*link)->next)
		;
	*link = cp->next;
	for (other = clients; other; other = other->next)
		if (other->transient == cp->win)
			other->transient = None;
	if (!destroyed) {
		XSetWindowBorderWidth(mochi.display, cp->win, cp->border);
		XRemoveFromSaveSet(mochi.display, cp->win);
		XSelectInput(mochi.display, cp->win, NoEventMask);
		x11_state(cp->win, WithdrawnState);
	}
	free(cp);
	if (fp)
		for (other = clients; other; other = other->next)
			if (!client_frame(other) && !other->transient) {
				fp->win = other->win;
				break;
			}
	client_refresh();
}

void client_next(int direction)
{
	Client *cp;
	Client *previous;
	Client *selected;

	if (!clients)
		return;
	selected = client_find(mochi.frame->win);
	previous = NULL;
	for (cp = clients; cp; cp = cp->next) {
		if (cp == selected)
			break;
		if (!cp->transient)
			previous = cp;
	}
	if (direction > 0) {
		cp = cp ? cp->next : clients;
		while (cp && cp->transient)
			cp = cp->next;
		if (!cp)
			cp = clients;
	} else {
		if (!previous)
			for (cp = clients; cp; cp = cp->next)
				if (!cp->transient)
					previous = cp;
		cp = previous;
	}
	show(cp);
	client_refresh();
}

int client_close(void)
{
	Client *cp;

	cp = focused();
	if (!cp)
		return FALSE;
	if (cp->delete_window)
		x11_protocol(cp->win, delete_window);
	else
		XKillClient(mochi.display, cp->win);
	return TRUE;
}

int client_select(const char *arg)
{
	Client *cp;
	char *end;
	unsigned long id;

	errno = 0;
	id = strtoul(arg, &end, 10);
	if (errno || !*arg || *end || *arg == '-') {
		input_message("select requires a window number (windows)");
		return FALSE;
	}
	for (cp = clients; cp; cp = cp->next)
		if (cp->id == id) {
			show(cp);
			client_refresh();
			return TRUE;
		}
	input_message("No such window");
	return FALSE;
}

int client_list(void)
{
	Client *cp;
	Client *selected;
	char text[MESSAGE_MAX];
	char *name;
	char number[3 * sizeof(unsigned long) + 1];
	size_t pos;
	int available;

	selected = focused();
	pos = 0;
	text[0] = '\0';
	for (cp = clients; cp; cp = cp->next) {
		name = NULL;
		XFetchName(mochi.display, cp->win, &name);
		sprintf(number, "%lu", cp->id);
		if (strlen(number) + 5 > sizeof text - pos) {
			if (name)
				XFree(name);
			break;
		}
		available = sizeof text - pos - strlen(number) - 5;
		sprintf(text + pos, "%s:%s%.*s  ", number,
			cp == selected ? "*" : "", available,
			name ? name : "untitled");
		if (name)
			XFree(name);
		pos = strlen(text);
	}
	input_message(*text ? text : "No windows");
	return TRUE;
}

void client_configure(XConfigureRequestEvent *event)
{
	Client *cp;
	Frame *fp;
	XWindowChanges changes;

	cp = client_find(event->window);
	if (cp) {
		if (cp->transient) {
			if (event->value_mask & CWWidth)
				cp->width = event->width;
			if (event->value_mask & CWHeight)
				cp->height = event->height;
		}
		fp = client_frame(cp);
		if (fp)
			configure(cp, fp);
		return;
	}
	changes.x = event->x;
	changes.y = event->y;
	changes.width = event->width;
	changes.height = event->height;
	changes.border_width = event->border_width;
	changes.sibling = event->above;
	changes.stack_mode = event->detail;
	XConfigureWindow(mochi.display, event->window, event->value_mask,
			 &changes);
}

void client_map(Window win)
{
	Client *cp;

	cp = client_find(win);
	if (cp)
		show(cp);
	else
		manage(win, TRUE);
	client_refresh();
}

void client_unmap(XUnmapEvent *event)
{
	Client *cp;

	cp = client_find(event->window);
	if (!cp)
		return;
	if (cp->pending && !event->send_event)
		--cp->pending;
	else
		forget(cp, FALSE);
}

void client_destroy(Window win)
{
	Client *cp;

	cp = client_find(win);
	if (cp)
		forget(cp, TRUE);
}

void client_message(XClientMessageEvent *event)
{
	Frame *fp;

	if (!x11_iconic(event))
		return;
	fp = frame_find(mochi.tree, event->window);
	if (fp) {
		fp->win = None;
		client_refresh();
	}
}

void client_scan(void)
{
	Window parent;
	Window rw;
	Window *children;
	unsigned int count;
	unsigned int i;
	XWindowAttributes attr;

	client_protocols = XInternAtom(mochi.display, "WM_PROTOCOLS", False);
	take_focus = XInternAtom(mochi.display, "WM_TAKE_FOCUS", False);
	delete_window = XInternAtom(mochi.display, "WM_DELETE_WINDOW", False);
	if (!XQueryTree(mochi.display, mochi.root, &rw, &parent, &children,
			&count))
		return;
	for (i = 0; i < count; ++i)
		if (XGetWindowAttributes(mochi.display, children[i], &attr) &&
		    (attr.map_state == IsViewable || x11_hidden(children[i])))
			manage(children[i], FALSE);
	XFree(children);
}

void client_free(void)
{
	Client *cp;

	while (clients) {
		cp = clients;
		clients = cp->next;
		XSetWindowBorderWidth(mochi.display, cp->win, cp->border);
		XMapWindow(mochi.display, cp->win);
		x11_state(cp->win, NormalState);
		XRemoveFromSaveSet(mochi.display, cp->win);
		free(cp);
	}
}

void client_property(XPropertyEvent *event)
{
	Client *cp;

	if (event->atom != XA_WM_HINTS && event->atom != client_protocols)
		return;
	cp = client_find(event->window);
	if (cp)
		properties(cp);
}
