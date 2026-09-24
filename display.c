#include <stddef.h>
#include <string.h>

#include <X11/Xlib.h>

#include "defs.h"
#include "display.h"
#include "frame.h"
#include "session.h"
#include "x11.h"

#define BAR_HEIGHT 24
#define LINE_HEIGHT 16
#define TEXT_PAD 4

Window bar;
GC gc;
XFontStruct *font;

int display_init(void)
{
	XSetWindowAttributes attr;
	int screen;

	screen = DefaultScreen(fish.display);
	font = XLoadQueryFont(fish.display, "fixed");
	if (!font)
		return FALSE;
	attr.override_redirect = True;
	attr.background_pixel = BlackPixel(fish.display, screen);
	attr.event_mask = ExposureMask | PropertyChangeMask;
	x11_trap();
	bar = XCreateWindow(fish.display, fish.root, 0, 0, 1, BAR_HEIGHT, 0,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWOverrideRedirect | CWBackPixel | CWEventMask,
			    &attr);
	if (x11_untrap()) {
		bar = None;
		goto fail;
	}
	gc = XCreateGC(fish.display, bar, 0, NULL);
	if (!gc)
		goto fail;
	XSetForeground(fish.display, gc, WhitePixel(fish.display, screen));
	XSetFont(fish.display, gc, font->fid);
	return TRUE;

fail:
	display_free();
	return FALSE;
}

void display_free(void)
{
	if (gc)
		XFreeGC(fish.display, gc);
	if (font)
		XFreeFont(fish.display, font);
	if (bar)
		XDestroyWindow(fish.display, bar);
	gc = NULL;
	font = NULL;
	bar = None;
}

Window display_window(void)
{
	return bar;
}

int row_length(const char *text, int n, int cols)
{
	int count;

	count = 0;
	while (count < n && count < cols && text[count] != '\n')
		++count;
	return count;
}

void display_draw(const char *text, int tail)
{
	const char *start;
	const char *scan;
	int n;
	int available;
	int cols;
	int rows;
	int row;
	int count;
	int limit;
	int w;
	int x;
	int y;
	int remaining;
	size_t len;

	start = text;
	len = strlen(start);
	if (len > MESSAGE_MAX)
		len = MESSAGE_MAX;
	n = len;
	w = fish.frame->width > 0 ? fish.frame->width : 1;
	x = fish.frame->x;
	y = fish.frame->y;
	if (!tail && strchr(text, '\n')) {
		w = fish.tree->width > 0 ? fish.tree->width : 1;
		x = 0;
		y = 0;
	}
	available = w - TEXT_PAD * 2;
	if (tail)
		while (n > 0 && XTextWidth(font, start, n) > available) {
			++start;
			--n;
		}
	cols = available /
	       (font->max_bounds.width > 0 ? font->max_bounds.width : 1);
	if (cols < 1)
		cols = 1;
	limit = (fish.tree->height - y - TEXT_PAD * 2) / LINE_HEIGHT;
	if (limit < 1)
		limit = 1;
	scan = start;
	remaining = n;
	rows = 0;
	do {
		count = row_length(scan, remaining, cols);
		if (count < remaining && scan[count] == '\n')
			++count;
		scan += count;
		remaining -= count;
		++rows;
	} while (!tail && remaining > 0 && rows < limit);
	XMoveResizeWindow(fish.display, bar, x, y, w,
			  rows * LINE_HEIGHT + TEXT_PAD * 2);
	XClearWindow(fish.display, bar);
	for (row = 0; row < rows; ++row) {
		count = row_length(start, n, cols);
		XDrawString(fish.display, bar, gc, TEXT_PAD,
			    row * LINE_HEIGHT + LINE_HEIGHT, start, count);
		if (count < n && start[count] == '\n')
			++count;
		start += count;
		n -= count;
	}
}

void display_show(void)
{
	XMapRaised(fish.display, bar);
}

void display_hide(void)
{
	XUnmapWindow(fish.display, bar);
}
