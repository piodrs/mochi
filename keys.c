#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "input.h"
#include "keys.h"
#include "session.h"
#include "x11.h"

static Keymap active;
static unsigned int locks;

typedef struct {
	const char *key;
	const char *command;
} DefaultBinding;

static const DefaultBinding defaults[] = {
	{
		"2",
		"split-horizontal",
	},
	{
		"3",
		"split-vertical",
	},
	{
		"0",
		"delete-frame",
	},
	{
		"1",
		"only-frame",
	},
	{
		"o",
		"next-frame",
	},
	{
		"M-o",
		"previous-frame",
	},
	{
		"C-n",
		"next-window",
	},
	{
		"C-p",
		"previous-window",
	},
	{
		"b",
		"select",
	},
	{
		"C-b",
		"windows",
	},
	{
		"M-!",
		"exec",
	},
	{
		"k",
		"close",
	},
	{
		"M-x",
		"command",
	},
	{
		"?",
		"help",
	},
};

int keys_parse(const char *text, Key *key)
{
	unsigned int bit;
	char ch;

	key->mask = 0;
	while (text[0] && text[1] == '-') {
		switch (*text) {
		case 'C':
			bit = ControlMask;
			break;
		case 'M':
			bit = Mod1Mask;
			break;
		case 's':
			bit = Mod4Mask;
			break;
		default:
			return FALSE;
		}
		if (key->mask & bit)
			return FALSE;
		key->mask |= bit;
		text += 2;
	}
	if (!*text)
		return FALSE;
	if (!text[1]) {
		ch = *text;
		if (ch < 33 || ch > 126)
			return FALSE;
		key->sym = ch;
	} else {
		key->sym = XStringToKeysym(text);
	}
	return key->sym != NoSymbol && !IsModifierKey(key->sym) &&
	       key->sym != XK_Escape &&
	       !((key->sym == XK_g || key->sym == XK_G) &&
		 key->mask == ControlMask);
}

int keys_bind(Keymap *map, Key key, const char *command)
{
	size_t i;

	if (strlen(command) >= COMMAND_MAX)
		return FALSE;
	for (i = 0; i < map->count; ++i)
		if (map->bindings[i].key.sym == key.sym &&
		    map->bindings[i].key.mask == key.mask)
			break;
	if (i == BINDING_MAX)
		return FALSE;
	map->bindings[i].key = key;
	strcpy(map->bindings[i].command, command);
	if (i == map->count)
		++map->count;
	return TRUE;
}

void keys_unbind(Keymap *map, Key key)
{
	size_t i;

	for (i = 0; i < map->count; ++i) {
		if (map->bindings[i].key.sym != key.sym ||
		    map->bindings[i].key.mask != key.mask)
			continue;
		--map->count;
		memmove(map->bindings + i, map->bindings + i + 1,
			(map->count - i) * sizeof map->bindings[0]);
		return;
	}
}

void keys_defaults(Keymap *map)
{
	Key key;
	size_t i;

	memset(map, 0, sizeof *map);
	keys_parse("C-t", &map->prefix);
	strcpy(map->name, "C-t");
	for (i = 0; i < ARRAY_SIZE(defaults); ++i) {
		keys_parse(defaults[i].key, &key);
		keys_bind(map, key, defaults[i].command);
	}
}

static void update_locks(void)
{
	XModifierKeymap *map;
	KeyCode num;
	int i;
	int j;

	locks = LockMask;
	num = XKeysymToKeycode(fish.display, XK_Num_Lock);
	map = XGetModifierMapping(fish.display);
	if (!map)
		return;
	for (i = 0; i < 8; ++i)
		for (j = 0; j < map->max_keypermod; ++j)
			if (num &&
			    map->modifiermap[i * map->max_keypermod + j] == num)
				locks |= 1U << i;
	XFreeModifiermap(map);
}

static int grab(Key key)
{
	KeyCode code;
	unsigned int shift;
	unsigned int mask;

	code = XKeysymToKeycode(fish.display, key.sym);
	if (!code)
		return FALSE;
	shift = 0;
	if (XkbKeycodeToKeysym(fish.display, code, 0, 0) != key.sym) {
		if (XkbKeycodeToKeysym(fish.display, code, 0, 1) != key.sym)
			return FALSE;
		shift = ShiftMask;
	}
	x11_trap();
	for (mask = 0; mask < 256; ++mask)
		if (!(mask & ~locks))
			XGrabKey(fish.display, code, key.mask | shift | mask,
				 fish.root, False, GrabModeAsync,
				 GrabModeAsync);
	return x11_untrap() == 0;
}

int keys_install(const Keymap *map)
{
	int ok;

	XGrabServer(fish.display);
	XUngrabKey(fish.display, AnyKey, AnyModifier, fish.root);
	ok = grab(map->prefix);
	if (ok) {
		active = *map;
	} else {
		XUngrabKey(fish.display, AnyKey, AnyModifier, fish.root);
		if (active.prefix.sym && !grab(active.prefix)) {
			XUngrabKey(fish.display, AnyKey, AnyModifier,
				   fish.root);
			fish.status = 1;
		}
	}
	XUngrabServer(fish.display);
	XFlush(fish.display);
	return ok;
}

int keys_init(void)
{
	update_locks();
	keys_defaults(&active);
	if (grab(active.prefix))
		return TRUE;
	keys_free();
	return FALSE;
}

void keys_free(void)
{
	XUngrabKey(fish.display, AnyKey, AnyModifier, fish.root);
}

void keys_refresh(void)
{
	update_locks();
	if (!keys_install(&active))
		input_message(
			"Cannot grab prefix after keyboard mapping change");
}

int keys_prefix(KeySym sym, unsigned int mask)
{
	return sym == active.prefix.sym && mask == active.prefix.mask;
}

unsigned int keys_locks(void)
{
	return locks;
}

const char *keys_lookup(KeySym sym, unsigned int mask)
{
	size_t i;

	for (i = 0; i < active.count; ++i)
		if (active.bindings[i].key.sym == sym &&
		    active.bindings[i].key.mask == mask)
			return active.bindings[i].command;
	return NULL;
}

const char *keys_name(void)
{
	return active.name;
}

int keys_help(void)
{
	char text[MESSAGE_MAX];
	char label[KEY_NAME_MAX];
	char symbol[2];
	const char *name;
	Key key;
	size_t i;
	size_t pos;
	size_t len;

	sprintf(text, "Prefix %s\n%-12s  Cancel\n", active.name,
		"C-g / Escape");
	pos = strlen(text);
	for (i = 0; i < active.count; ++i) {
		key = active.bindings[i].key;
		name = XKeysymToString(key.sym);
		if (key.sym >= 33 && key.sym <= 126) {
			symbol[0] = key.sym;
			symbol[1] = '\0';
			name = symbol;
		}
		sprintf(label, "%s%s%s%.*s", key.mask & ControlMask ? "C-" : "",
			key.mask & Mod1Mask ? "M-" : "",
			key.mask & Mod4Mask ? "s-" : "", KEY_NAME_MAX - 7,
			name ? name : "?");
		len = strlen(label) + strlen(active.bindings[i].command) + 3;
		if (len >= sizeof text - pos)
			break;
		sprintf(text + pos, "%s%s  %s", i ? "\n" : "", label,
			active.bindings[i].command);
		pos = strlen(text);
	}
	input_message(text);
	return TRUE;
}
