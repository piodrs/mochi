#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "command.h"
#include "defs.h"
#include "display.h"
#include "input.h"
#include "keys.h"
#include "session.h"

#define INPUT_IDLE 0
#define INPUT_PREFIX 1
#define INPUT_PROMPT 2
#define MESSAGE_SECONDS 5

int mode;
char line[COMMAND_MAX];
char message[MESSAGE_MAX];
size_t pos;
time_t expires;

void input_draw(void)
{
	char text[MESSAGE_MAX];

	if (mode == INPUT_PROMPT)
		sprintf(text, "M-x %s_", line);
	else if (mode == INPUT_PREFIX)
		sprintf(text, "%s  (C-g cancel)", keys_name());
	else {
		display_draw(message, FALSE);
		return;
	}
	display_draw(text, TRUE);
}

void input_message(const char *text)
{
	sprintf(message, "%.*s", MESSAGE_MAX - 1, text);
	expires = time(NULL) + MESSAGE_SECONDS;
	display_show();
	input_draw();
}

int input_prompt(const char *text)
{
	if (XGrabKeyboard(fish.display, fish.root, False, GrabModeAsync,
			  GrabModeAsync, CurrentTime) != GrabSuccess) {
		input_message("Cannot grab keyboard");
		return FALSE;
	}
	mode = INPUT_PROMPT;
	sprintf(line, "%.*s", COMMAND_MAX - 1, text);
	pos = strlen(line);
	display_show();
	input_draw();
	return TRUE;
}

void input_tick(void)
{
	if (!mode && expires && time(NULL) >= expires) {
		display_hide();
		expires = 0;
	}
}

void input_cancel(void)
{
	mode = INPUT_IDLE;
	message[0] = '\0';
	expires = 0;
	XUngrabKeyboard(fish.display, CurrentTime);
	display_hide();
}

void input_key(XKeyEvent *event)
{
	KeySym key;
	char bytes[32];
	unsigned int mask;
	const char *binding;
	int n;
	int j;
	XKeyEvent clean;

	clean = *event;
	clean.state &= ~keys_locks();
	n = XLookupString(&clean, bytes, sizeof bytes, &key, NULL);
	mask = event->state & (ControlMask | Mod1Mask | Mod4Mask);
	if (!mode) {
		if (!keys_prefix(key, mask))
			return;
		if (XGrabKeyboard(fish.display, fish.root, False, GrabModeAsync,
				  GrabModeAsync, event->time) != GrabSuccess)
			return;
		mode = INPUT_PREFIX;
		display_show();
		input_draw();
		return;
	}
	if (IsModifierKey(key))
		return;
	if (key == XK_Escape ||
	    ((key == XK_g || key == XK_G) && mask == ControlMask)) {
		input_cancel();
		return;
	}
	if (mode == INPUT_PREFIX) {
		input_cancel();
		binding = keys_lookup(key, mask);
		if (binding) {
			if (!command_run(binding))
				XBell(fish.display, 0);
			return;
		}
		input_message("Undefined key");
		return;
	}
	if (key == XK_Return || key == XK_KP_Enter) {
		input_cancel();
		if (!command_run(line))
			XBell(fish.display, 0);
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
			XBell(fish.display, 0);
		for (j = 0; j < n && pos < sizeof line - 1; ++j)
			if (bytes[j] >= 32 && bytes[j] <= 126)
				line[pos++] = bytes[j];
		line[pos] = '\0';
	}
	input_draw();
}
