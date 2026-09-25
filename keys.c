#include "keys.h"

#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "defs.h"
#include "input.h"
#include "session.h"
#include "x11.h"

static Keymap active;
static unsigned int locks;

typedef struct {
	const char *key;
	const char *command;
} DefaultBinding;

static const DefaultBinding defaults[] = {
	{"2", "split-horizontal"},
	{"3", "split-vertical"},
	{"0", "delete-frame"},
	{"1", "only-frame"},
	{"o", "next-frame"},
	{"M-o", "previous-frame"},
	{"C-n", "next-window"},
	{"C-p", "previous-window"},
	{"b", "select"},
	{"C-b", "windows"},
	{"M-!", "exec"},
	{"k", "close"},
	{"M-x", "command"},
	{"?", "help"},
};

bool keys_parse(const char *text, Key *key)
{
	key->mask = 0;
	while (text[0] && text[1] == '-') {
		unsigned int bit;
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
			return false;
		}
		if (key->mask & bit)
			return false;
		key->mask |= bit;
		text += 2;
	}
	if (!*text)
		return false;
	if (!text[1]) {
		char ch = *text;
		if (ch < 33 || ch > 126)
			return false;
		key->sym = (KeySym)ch;
	} else {
		key->sym = XStringToKeysym(text);
	}
	return key->sym != NoSymbol && !IsModifierKey(key->sym) && key->sym != XK_Escape &&
		!((key->sym == XK_g || key->sym == XK_G) && key->mask == ControlMask);
}

bool keys_bind(Keymap *map, Key key, const char *command)
{
	size_t i;

	if (strlen(command) >= COMMAND_MAX)
		return false;
	for (i = 0; i < map->count; ++i)
		if (map->bindings[i].key.sym == key.sym && map->bindings[i].key.mask == key.mask)
			break;
	if (i == BINDING_MAX)
		return false;
	map->bindings[i].key = key;
	strcpy(map->bindings[i].command, command);
	if (i == map->count)
		++map->count;
	return true;
}

void keys_unbind(Keymap *map, Key key)
{
	for (size_t i = 0; i < map->count; ++i) {
		if (map->bindings[i].key.sym != key.sym || map->bindings[i].key.mask != key.mask)
			continue;
		--map->count;
		memmove(map->bindings + i, map->bindings + i + 1,
			(map->count - i) * sizeof map->bindings[0]);
		return;
	}
}

void keys_defaults(Keymap *map)
{
	*map = (Keymap){0};
	keys_parse("C-t", &map->prefix);
	strcpy(map->name, "C-t");
	for (size_t i = 0; i < ARRAY_SIZE(defaults); ++i) {
		Key key;
		keys_parse(defaults[i].key, &key);
		keys_bind(map, key, defaults[i].command);
	}
}

static void keys_update_locks(void)
{
	locks = LockMask;
	KeyCode num = XKeysymToKeycode(mochi.display, XK_Num_Lock);
	XModifierKeymap *map = XGetModifierMapping(mochi.display);
	if (!map)
		return;
	for (int i = 0; i < 8; ++i)
		for (int j = 0; j < map->max_keypermod; ++j)
			if (num && map->modifiermap[i * map->max_keypermod + j] == num)
				locks |= 1U << i;
	XFreeModifiermap(map);
}

static bool keys_grab(Key key)
{
	KeyCode code = XKeysymToKeycode(mochi.display, key.sym);
	if (!code)
		return false;
	unsigned int shift = 0;
	if (XkbKeycodeToKeysym(mochi.display, code, 0, 0) != key.sym) {
		if (XkbKeycodeToKeysym(mochi.display, code, 0, 1) != key.sym)
			return false;
		shift = ShiftMask;
	}
	x11_trap();
	for (unsigned int mask = 0; mask < 256; ++mask)
		if (!(mask & ~locks))
			XGrabKey(mochi.display, code, key.mask | shift | mask, mochi.root, False,
				GrabModeAsync, GrabModeAsync);
	return x11_untrap() == 0;
}

bool keys_install(const Keymap *map)
{
	XGrabServer(mochi.display);
	XUngrabKey(mochi.display, AnyKey, AnyModifier, mochi.root);
	bool ok = keys_grab(map->prefix);
	if (ok) {
		active = *map;
	} else {
		XUngrabKey(mochi.display, AnyKey, AnyModifier, mochi.root);
		if (active.prefix.sym && !keys_grab(active.prefix)) {
			XUngrabKey(mochi.display, AnyKey, AnyModifier, mochi.root);
			mochi.status = 1;
		}
	}
	XUngrabServer(mochi.display);
	XFlush(mochi.display);
	return ok;
}

bool keys_init(void)
{
	keys_update_locks();
	keys_defaults(&active);
	if (keys_grab(active.prefix))
		return true;
	keys_free();
	return false;
}

void keys_free(void)
{
	XUngrabKey(mochi.display, AnyKey, AnyModifier, mochi.root);
}

void keys_refresh(void)
{
	keys_update_locks();
	if (!keys_install(&active))
		input_message("Cannot grab prefix after keyboard mapping change");
}

bool keys_prefix(KeySym sym, unsigned int mask)
{
	return sym == active.prefix.sym && mask == active.prefix.mask;
}

unsigned int keys_locks(void)
{
	return locks;
}

const char *keys_lookup(KeySym sym, unsigned int mask)
{
	for (size_t i = 0; i < active.count; ++i)
		if (active.bindings[i].key.sym == sym && active.bindings[i].key.mask == mask)
			return active.bindings[i].command;
	return NULL;
}

const char *keys_name(void)
{
	return active.name;
}

void keys_help(void)
{
	char text[MESSAGE_MAX];

	snprintf(text, sizeof text, "Prefix %s\n%-12s  Cancel\n", active.name, "C-g / Escape");
	size_t pos = strlen(text);
	for (size_t i = 0; i < active.count; ++i) {
		char label[KEY_NAME_MAX];
		char symbol[2];
		Key key = active.bindings[i].key;
		const char *name = XKeysymToString(key.sym);
		if (key.sym >= 33 && key.sym <= 126) {
			symbol[0] = (char)key.sym;
			symbol[1] = '\0';
			name = symbol;
		}
		snprintf(label, sizeof label, "%s%s%s%.*s", key.mask & ControlMask ? "C-" : "",
			key.mask & Mod1Mask ? "M-" : "", key.mask & Mod4Mask ? "s-" : "",
			KEY_NAME_MAX - 7, name ? name : "?");
		size_t label_length = strlen(label);
		size_t command_length = strlen(active.bindings[i].command);
		if (label_length + command_length + 3 >= sizeof text - pos)
			break;
		if (i)
			text[pos++] = '\n';
		memcpy(text + pos, label, label_length);
		pos += label_length;
		text[pos++] = ' ';
		text[pos++] = ' ';
		memcpy(text + pos, active.bindings[i].command, command_length + 1);
		pos += command_length;
	}
	input_message(text);
}
