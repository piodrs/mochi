#include "input.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "command.h"
#include "defs.h"
#include "display.h"
#include "keys.h"
#include "session.h"

enum InputMode { INPUT_IDLE, INPUT_PREFIX, INPUT_PROMPT };
enum { MESSAGE_SECONDS = 5 };

static enum InputMode mode;
static char line[COMMAND_MAX];
static char message[MESSAGE_MAX];
static size_t pos;
static time_t expires;

void input_draw(void)
{
	char text[MESSAGE_MAX];

	if (mode == INPUT_PROMPT)
		snprintf(text, sizeof text, "M-x %s_", line);
	else if (mode == INPUT_PREFIX)
		snprintf(text, sizeof text, "%s  (C-g cancel)", keys_name());
	else {
		display_draw(message, false);
		return;
	}
	display_draw(text, true);
}

void input_message(const char *text)
{
	snprintf(message, sizeof message, "%s", text);
	expires = time(NULL) + MESSAGE_SECONDS;
	display_show();
	input_draw();
}

bool input_prompt(const char *text)
{
	if (XGrabKeyboard(mochi.display, mochi.root, False, GrabModeAsync, GrabModeAsync,
		    CurrentTime) != GrabSuccess) {
		input_message("Cannot grab keyboard");
		return false;
	}
	mode = INPUT_PROMPT;
	snprintf(line, sizeof line, "%s", text);
	pos = strlen(line);
	display_show();
	input_draw();
	return true;
}

void input_tick(void)
{
	if (mode == INPUT_IDLE && expires && time(NULL) >= expires) {
		display_hide();
		expires = 0;
	}
}

void input_cancel(void)
{
	mode = INPUT_IDLE;
	message[0] = '\0';
	expires = 0;
	XUngrabKeyboard(mochi.display, CurrentTime);
	display_hide();
}

void input_key(const XKeyEvent *event)
{
	KeySym key;
	char bytes[32];

	XKeyEvent clean = *event;
	clean.state &= ~keys_locks();
	int n = XLookupString(&clean, bytes, sizeof bytes, &key, NULL);
	unsigned int mask = event->state & (ControlMask | Mod1Mask | Mod4Mask);
	if (mode == INPUT_IDLE) {
		if (!keys_prefix(key, mask))
			return;
		if (XGrabKeyboard(mochi.display, mochi.root, False, GrabModeAsync, GrabModeAsync,
			    event->time) != GrabSuccess)
			return;
		mode = INPUT_PREFIX;
		display_show();
		input_draw();
		return;
	}
	if (IsModifierKey(key))
		return;
	if (key == XK_Escape || ((key == XK_g || key == XK_G) && mask == ControlMask)) {
		input_cancel();
		return;
	}
	if (mode == INPUT_PREFIX) {
		input_cancel();
		const char *binding = keys_lookup(key, mask);
		if (binding) {
			if (!command_run(binding))
				XBell(mochi.display, 0);
			return;
		}
		input_message("Undefined key");
		return;
	}
	if (key == XK_Return || key == XK_KP_Enter) {
		input_cancel();
		if (!command_run(line))
			XBell(mochi.display, 0);
		return;
	}
	if (key == XK_BackSpace || (key == XK_h && mask == ControlMask)) {
		if (pos)
			line[--pos] = '\0';
	} else if (key == XK_u && mask == ControlMask) {
		pos = 0;
		line[0] = '\0';
	} else if (!mask) {
		if (n > 0 && pos == sizeof line - 1)
			XBell(mochi.display, 0);
		for (int j = 0; j < n && pos < sizeof line - 1; ++j)
			if (bytes[j] >= 32 && bytes[j] <= 126)
				line[pos++] = bytes[j];
		line[pos] = '\0';
	}
	input_draw();
}
