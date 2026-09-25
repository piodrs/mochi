#include "client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

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
	bool mapped;
	bool input;
	bool take_focus;
	bool delete_window;
	unsigned int pending;
	unsigned int border;
	int width;
	int height;
};

static Client *clients;
static unsigned long next_id;
static Atom client_protocols;
static Atom take_focus;
static Atom delete_window;

static Client *client_find(Window win)
{
	for (Client *client = clients; client; client = client->next)
		if (client->win == win)
			return client;
	return NULL;
}

static void client_properties(Client *client)
{
	Atom *list;
	int count;

	XWMHints *hints = XGetWMHints(mochi.display, client->win);
	client->input = !hints || !(hints->flags & InputHint) || hints->input;
	if (hints)
		XFree(hints);
	client->take_focus = false;
	client->delete_window = false;
	if (XGetWMProtocols(mochi.display, client->win, &list, &count)) {
		for (int i = 0; i < count; ++i) {
			if (list[i] == take_focus)
				client->take_focus = true;
			if (list[i] == delete_window)
				client->delete_window = true;
		}
		XFree(list);
	}
}

static Frame *client_frame(Client *client)
{
	while (client) {
		Frame *frame = frame_find(mochi.tree, client->win);
		if (frame)
			return frame;
		client = client_find(client->transient);
	}
	return NULL;
}

static Client *client_focused(void)
{
	Client *selected = client_find(mochi.frame->win);
	for (Client *client = clients; client; client = client->next)
		if (client->transient && client_frame(client) == mochi.frame)
			selected = client;
	return selected;
}

static void client_resize(Client *client, Frame *frame)
{
	int w = frame->width > 0 ? frame->width : 1;
	int h = frame->height > 0 ? frame->height : 1;
	if (client->transient) {
		if (client->width < w)
			w = client->width;
		if (client->height < h)
			h = client->height;
	}
	int x = frame->x + (frame->width - w) / 2;
	int y = frame->y + (frame->height - h) / 2;
	XMoveResizeWindow(mochi.display, client->win, x, y, (unsigned int)w, (unsigned int)h);
	XEvent event = {
		.xconfigure = {
			.type = ConfigureNotify,
			.display = mochi.display,
			.event = client->win,
			.window = client->win,
			.x = x,
			.y = y,
			.width = w,
			.height = h,
			.above = None
		}
	};
	XSendEvent(mochi.display, client->win, False, StructureNotifyMask, &event);
}

void client_refresh(void)
{
	frame_layout(mochi.tree, 0, 0, mochi.tree->width, mochi.tree->height);
	for (Client *client = clients; client; client = client->next) {
		Frame *frame = client_frame(client);
		if (frame) {
			client_resize(client, frame);
			if (!client->mapped) {
				XMapWindow(mochi.display, client->win);
				client->mapped = true;
				x11_state(client->win, NormalState);
			}
		} else if (client->mapped) {
			++client->pending;
			XUnmapWindow(mochi.display, client->win);
			client->mapped = false;
			x11_state(client->win, IconicState);
		}
	}
	Client *selected = client_focused();
	mochi.time = x11_time(display_window());
	XSetInputFocus(mochi.display, selected && selected->input ? selected->win : mochi.root,
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

static void client_show(Client *client)
{
	Client *parent;

	while ((parent = client_find(client->transient)) != NULL)
		client = parent;
	Frame *frame = frame_find(mochi.tree, client->win);
	if (frame)
		mochi.frame = frame;
	else
		mochi.frame->win = client->win;
}

static void client_manage(Window win, bool select)
{
	XWindowAttributes attr;
	Client **tail;

	if (win == display_window())
		return;
	if (!XGetWindowAttributes(mochi.display, win, &attr) || attr.override_redirect ||
		attr.class == InputOnly)
		return;
	Client *client = calloc(1, sizeof *client);
	if (!client) {
		fprintf(stderr, APP_NAME ": out of memory\n");
		mochi.status = 1;
		return;
	}
	client->win = win;
	client->id = ++next_id;
	client->mapped = attr.map_state != IsUnmapped;
	client->border = (unsigned int)attr.border_width;
	client->width = attr.width;
	client->height = attr.height;
	XGetTransientForHint(mochi.display, win, &client->transient);
	if (!client_find(client->transient))
		client->transient = None;
	client_properties(client);
	for (tail = &clients; *tail; tail = &(*tail)->next)
		;
	*tail = client;
	XAddToSaveSet(mochi.display, win);
	XSelectInput(mochi.display, win, PropertyChangeMask);
	XSetWindowBorderWidth(mochi.display, win, 0);
	x11_state(client->win, client->mapped ? NormalState : IconicState);
	if (!client->transient && (select || !mochi.frame->win))
		mochi.frame->win = win;
	else if (client->transient && select)
		client_show(client);
}

static void client_forget(Client *client, bool destroyed)
{
	Client **link;

	Frame *frame = frame_find(mochi.tree, client->win);
	if (frame)
		frame->win = None;
	for (link = &clients; *link != client; link = &(*link)->next)
		;
	*link = client->next;
	for (Client *other = clients; other; other = other->next)
		if (other->transient == client->win)
			other->transient = None;
	if (!destroyed) {
		XSetWindowBorderWidth(mochi.display, client->win, client->border);
		XRemoveFromSaveSet(mochi.display, client->win);
		XSelectInput(mochi.display, client->win, NoEventMask);
		x11_state(client->win, WithdrawnState);
	}
	free(client);
	if (frame)
		for (Client *other = clients; other; other = other->next)
			if (!other->transient && !frame_find(mochi.tree, other->win)) {
				frame->win = other->win;
				break;
			}
	client_refresh();
}

void client_next(int direction)
{
	Client *client;

	if (!clients)
		return;
	Client *selected = client_find(mochi.frame->win);
	Client *previous = NULL;
	for (client = clients; client && client != selected; client = client->next)
		if (!client->transient)
			previous = client;
	if (direction > 0) {
		client = client ? client->next : clients;
		while (client && client->transient)
			client = client->next;
		if (!client)
			client = clients;
	} else {
		if (!previous)
			for (client = clients; client; client = client->next)
				if (!client->transient)
					previous = client;
		client = previous;
	}
	client_show(client);
	client_refresh();
}

bool client_close(void)
{
	Client *client = client_focused();
	if (!client)
		return false;
	if (client->delete_window)
		x11_protocol(client->win, delete_window);
	else
		XKillClient(mochi.display, client->win);
	return true;
}

bool client_select(const char *arg)
{
	char *end;

	errno = 0;
	unsigned long id = strtoul(arg, &end, 10);
	if (errno || !*arg || *end || *arg == '-') {
		input_message("select requires a window number (windows)");
		return false;
	}
	for (Client *client = clients; client; client = client->next)
		if (client->id == id) {
			client_show(client);
			client_refresh();
			return true;
		}
	input_message("No such window");
	return false;
}

void client_list(void)
{
	char text[MESSAGE_MAX];

	Client *selected = client_focused();
	size_t pos = 0;
	text[0] = '\0';
	for (Client *client = clients; client; client = client->next) {
		char *name = NULL;
		char number[3 * sizeof(unsigned long) + 1];
		XFetchName(mochi.display, client->win, &name);
		snprintf(number, sizeof number, "%lu", client->id);
		if (strlen(number) + 5 > sizeof text - pos) {
			if (name)
				XFree(name);
			break;
		}
		int available = (int)(sizeof text - pos - strlen(number) - 5);
		snprintf(text + pos, sizeof text - pos, "%s:%s%.*s  ", number,
			client == selected ? "*" : "", available, name ? name : "untitled");
		if (name)
			XFree(name);
		pos = strlen(text);
	}
	input_message(*text ? text : "No windows");
}

void client_configure(const XConfigureRequestEvent *event)
{
	Client *client = client_find(event->window);
	if (client) {
		if (client->transient) {
			if (event->value_mask & CWWidth)
				client->width = event->width;
			if (event->value_mask & CWHeight)
				client->height = event->height;
		}
		Frame *frame = client_frame(client);
		if (frame)
			client_resize(client, frame);
		return;
	}
	XWindowChanges changes = {
		.x = event->x,
		.y = event->y,
		.width = event->width,
		.height = event->height,
		.border_width = event->border_width,
		.sibling = event->above,
		.stack_mode = event->detail
	};
	XConfigureWindow(mochi.display, event->window, (unsigned int)event->value_mask, &changes);
}

void client_map(Window win)
{
	Client *client = client_find(win);
	if (client)
		client_show(client);
	else
		client_manage(win, true);
	client_refresh();
}

void client_unmap(const XUnmapEvent *event)
{
	Client *client = client_find(event->window);
	if (!client)
		return;
	if (client->pending && !event->send_event)
		--client->pending;
	else
		client_forget(client, false);
}

void client_destroy(Window win)
{
	Client *client = client_find(win);
	if (client)
		client_forget(client, true);
}

void client_message(const XClientMessageEvent *event)
{
	if (!x11_iconic(event))
		return;
	Frame *frame = frame_find(mochi.tree, event->window);
	if (frame) {
		frame->win = None;
		client_refresh();
	}
}

void client_scan(void)
{
	Window parent;
	Window rw;
	Window *children;
	unsigned int count;
	XWindowAttributes attr;

	client_protocols = XInternAtom(mochi.display, "WM_PROTOCOLS", False);
	take_focus = XInternAtom(mochi.display, "WM_TAKE_FOCUS", False);
	delete_window = XInternAtom(mochi.display, "WM_DELETE_WINDOW", False);
	if (!XQueryTree(mochi.display, mochi.root, &rw, &parent, &children, &count))
		return;
	for (unsigned int i = 0; i < count; ++i)
		if (XGetWindowAttributes(mochi.display, children[i], &attr) &&
			(attr.map_state == IsViewable || x11_hidden(children[i])))
			client_manage(children[i], false);
	XFree(children);
}

void client_free(void)
{
	while (clients) {
		Client *client = clients;
		clients = client->next;
		XSetWindowBorderWidth(mochi.display, client->win, client->border);
		XMapWindow(mochi.display, client->win);
		x11_state(client->win, NormalState);
		XRemoveFromSaveSet(mochi.display, client->win);
		free(client);
	}
}

void client_property(const XPropertyEvent *event)
{
	if (event->atom != XA_WM_HINTS && event->atom != client_protocols)
		return;
	Client *client = client_find(event->window);
	if (client)
		client_properties(client);
}
