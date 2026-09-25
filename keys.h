#ifndef KEYS_H
#define KEYS_H

#include <stdbool.h>
#include <stddef.h>

#include <X11/Xlib.h>

#include "defs.h"

enum { KEY_NAME_MAX = 64, BINDING_MAX = 128 };

typedef struct {
	KeySym sym;
	unsigned int mask;
} Key;

typedef struct {
	Key key;
	char command[COMMAND_MAX];
} Binding;

typedef struct {
	Key prefix;
	char name[KEY_NAME_MAX];
	Binding bindings[BINDING_MAX];
	size_t count;
} Keymap;

bool keys_parse(const char *text, Key *key);
void keys_defaults(Keymap *map);
bool keys_bind(Keymap *map, Key key, const char *command);
void keys_unbind(Keymap *map, Key key);
bool keys_install(const Keymap *map);
bool keys_init(void);
void keys_free(void);
void keys_refresh(void);
bool keys_prefix(KeySym sym, unsigned int mask);
unsigned int keys_locks(void);
const char *keys_lookup(KeySym sym, unsigned int mask);
const char *keys_name(void);
void keys_help(void);

#endif /* KEYS_H */
