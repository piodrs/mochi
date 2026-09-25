#include "display.h"

#include <stddef.h>
#include <string.h>

#include <X11/Xlib.h>

#include "frame.h"
#include "session.h"
#include "x11.h"

enum { LINE_HEIGHT = 16, TEXT_PAD = 4 };

static Window bar;
static GC gc;
static XFontStruct *font;

bool display_init(void)
{
	int screen = DefaultScreen(mochi.display);
	font = XLoadQueryFont(mochi.display, "fixed");
	if (!font)
		return false;
	XSetWindowAttributes attr = {
		.override_redirect = True,
		.background_pixel = BlackPixel(mochi.display, screen),
		.event_mask = ExposureMask | PropertyChangeMask
	};
	x11_trap();
	bar = XCreateWindow(mochi.display, mochi.root, 0, 0, 1, LINE_HEIGHT + TEXT_PAD * 2, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWOverrideRedirect | CWBackPixel | CWEventMask, &attr);
	if (x11_untrap()) {
		bar = None;
		goto fail;
	}
	gc = XCreateGC(mochi.display, bar, 0, NULL);
	if (!gc)
		goto fail;
	XSetForeground(mochi.display, gc, WhitePixel(mochi.display, screen));
	XSetFont(mochi.display, gc, font->fid);
	return true;

fail:
	display_free();
	return false;
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

static int display_row_length(const char *text, int n, int cols)
{
	int count = 0;
	while (count < n && count < cols && text[count] != '\n')
		++count;
	return count;
}

void display_draw(const char *text, bool tail)
{
	int count;

	const char *start = text;
	int n = (int)strlen(start);
	int w = mochi.frame->width > 0 ? mochi.frame->width : 1;
	int x = mochi.frame->x;
	int y = mochi.frame->y;
	if (!tail && strchr(text, '\n')) {
		w = mochi.tree->width > 0 ? mochi.tree->width : 1;
		x = 0;
		y = 0;
	}
	int available = w - TEXT_PAD * 2;
	if (tail)
		while (n > 0 && XTextWidth(font, start, n) > available) {
			++start;
			--n;
		}
	int cols = available / (font->max_bounds.width > 0 ? font->max_bounds.width : 1);
	if (cols < 1)
		cols = 1;
	int limit = (mochi.tree->height - y - TEXT_PAD * 2) / LINE_HEIGHT;
	if (limit < 1)
		limit = 1;
	const char *scan = start;
	int remaining = n;
	int rows = 0;
	do {
		count = display_row_length(scan, remaining, cols);
		if (count < remaining && scan[count] == '\n')
			++count;
		scan += count;
		remaining -= count;
		++rows;
	} while (!tail && remaining > 0 && rows < limit);
	XMoveResizeWindow(mochi.display, bar, x, y, (unsigned int)w,
		(unsigned int)(rows * LINE_HEIGHT + TEXT_PAD * 2));
	XClearWindow(mochi.display, bar);
	for (int row = 0; row < rows; ++row) {
		count = display_row_length(start, n, cols);
		XDrawString(mochi.display, bar, gc, TEXT_PAD, row * LINE_HEIGHT + LINE_HEIGHT,
			start, count);
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
