#include <stddef.h>
#include <string.h>

#include <X11/Xlib.h>

#include "defs.h"
#include "display.h"
#include "frame.h"
#include "session.h"
#include "x11.h"

#define LINE_HEIGHT 16
#define TEXT_PAD 4

Window bar;
GC gc;
XFontStruct *font;

int display_init(void)
{
	XSetWindowAttributes attr;
	int screen;

	screen = DefaultScreen(mochi.display);
	font = XLoadQueryFont(mochi.display, "fixed");
	if (!font)
		return FALSE;
	attr.override_redirect = True;
	attr.background_pixel = BlackPixel(mochi.display, screen);
	attr.event_mask = ExposureMask | PropertyChangeMask;
	x11_trap();
	bar = XCreateWindow(mochi.display, mochi.root, 0, 0, 1,
			    LINE_HEIGHT + TEXT_PAD * 2, 0,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWOverrideRedirect | CWBackPixel | CWEventMask,
			    &attr);
	if (x11_untrap()) {
		bar = None;
		goto fail;
	}
	gc = XCreateGC(mochi.display, bar, 0, NULL);
	if (!gc)
		goto fail;
	XSetForeground(mochi.display, gc, WhitePixel(mochi.display, screen));
	XSetFont(mochi.display, gc, font->fid);
	return TRUE;

fail:
	display_free();
	return FALSE;
}

void display_free(void)
{
	if (gc)
		XFreeGC(mochi.display, gc);
	if (font)
		XFreeFont(mochi.display, font);
	if (bar)
		XDestroyWindow(mochi.display, bar);
	gc = NULL;
	font = NULL;
	bar = None;
}

Window display_window(void)
{
	return bar;
}

int display_row_length(const char *text, int n, int cols)
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

	start = text;
	n = strlen(start);
	w = mochi.frame->width > 0 ? mochi.frame->width : 1;
	x = mochi.frame->x;
	y = mochi.frame->y;
	if (!tail && strchr(text, '\n')) {
		w = mochi.tree->width > 0 ? mochi.tree->width : 1;
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
	limit = (mochi.tree->height - y - TEXT_PAD * 2) / LINE_HEIGHT;
	if (limit < 1)
		limit = 1;
	scan = start;
	remaining = n;
	rows = 0;
	do {
		count = display_row_length(scan, remaining, cols);
		if (count < remaining && scan[count] == '\n')
			++count;
		scan += count;
		remaining -= count;
		++rows;
	} while (!tail && remaining > 0 && rows < limit);
	XMoveResizeWindow(mochi.display, bar, x, y, w,
			  rows * LINE_HEIGHT + TEXT_PAD * 2);
	XClearWindow(mochi.display, bar);
	for (row = 0; row < rows; ++row) {
		count = display_row_length(start, n, cols);
		XDrawString(mochi.display, bar, gc, TEXT_PAD,
			    row * LINE_HEIGHT + LINE_HEIGHT, start, count);
		if (count < n && start[count] == '\n')
			++count;
		start += count;
		n -= count;
	}
}

void display_show(void)
{
	XMapRaised(mochi.display, bar);
}

void display_hide(void)
{
	XUnmapWindow(mochi.display, bar);
}
