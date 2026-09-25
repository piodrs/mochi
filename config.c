#include "config.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "defs.h"
#include "input.h"
#include "keys.h"
#include "process.h"

enum { STARTUP_MAX = 32, CONFIG_PATH_MAX = 4096 };

typedef struct {
	Keymap keys;
	char startup[STARTUP_MAX][COMMAND_MAX];
	size_t count;
} Config;

static const char *filename;

void config_path(const char *path)
{
	filename = path;
}

static char *config_skip(char *text)
{
	while (*text == ' ' || *text == '\t' || *text == '\r')
		++text;
	return text;
}

static char *config_word(char **rest)
{
	char *start = *rest;
	char *end = start;
	while (*end && *end != ' ' && *end != '\t' && *end != '\r')
		++end;
	if (*end)
		*end++ = '\0';
	*rest = config_skip(end);
	return start;
}

static const char *config_parse(Config *cfg, char *line)
{
	Key key;

	size_t len = strlen(line);
	if (len && line[len - 1] == '\r')
		line[len - 1] = '\0';
	line = config_skip(line);
	if (!*line || *line == '#')
		return NULL;
	char *op = config_word(&line);
	if (!strcmp(op, "exec")) {
		if (!*line)
			return "exec requires a program";
		if (cfg->count == STARTUP_MAX)
			return "too many startup programs";
		strcpy(cfg->startup[cfg->count++], line);
		return NULL;
	}
	if (strcmp(op, "prefix") && strcmp(op, "bind") && strcmp(op, "unbind"))
		return "expected prefix, bind, unbind or exec";
	char *arg = config_word(&line);
	if (!keys_parse(arg, &key))
		return "invalid key (C-g and Escape are reserved for cancel)";
	if (!strcmp(op, "bind")) {
		if (!command_valid(line))
			return "invalid command in binding";
		if (!keys_bind(&cfg->keys, key, line))
			return "too many bindings";
		return NULL;
	}
	if (*line)
		return "unexpected argument after key";
	if (!strcmp(op, "unbind")) {
		keys_unbind(&cfg->keys, key);
		return NULL;
	}
	if (!key.mask || strlen(arg) >= KEY_NAME_MAX)
		return "prefix requires C-, M- or s- and a short key name";
	cfg->keys.prefix = key;
	strcpy(cfg->keys.name, arg);
	return NULL;
}

static const char *config_read(FILE *file, Config *cfg, unsigned long *number)
{
	char line[COMMAND_MAX];
	int ch;

	size_t len = 0;
	*number = 1;
	while ((ch = fgetc(file)) != EOF) {
		if (ch != '\n') {
			if (!ch || len == sizeof line - 1)
				return "line too long or contains a NUL byte";
			line[len++] = (char)ch;
			continue;
		}
		line[len] = '\0';
		const char *error = config_parse(cfg, line);
		if (error)
			return error;
		len = 0;
		if (*number == ULONG_MAX)
			return "too many configuration lines";
		++*number;
	}
	if (ferror(file))
		return "cannot read configuration";
	line[len] = '\0';
	return config_parse(cfg, line);
}

static bool config_report(const char *path, unsigned long line, const char *error)
{
	char text[MESSAGE_MAX];

	snprintf(text, sizeof text, "%.1023s:%lu: %.900s", path, line, error);
	fprintf(stderr, APP_NAME ": %s\n", text);
	input_message(text);
	return false;
}

bool config_load(bool startup)
{
	char name[CONFIG_PATH_MAX];

	const char *path = filename;
	if (!path) {
		const char *home = getenv("HOME");
		if (!home || !*home)
			return config_report("~/.mochirc", 0, "HOME is not set");
		if (strlen(home) > sizeof name - sizeof "/.mochirc")
			return config_report("~/.mochirc", 0, "path too long");
		snprintf(name, sizeof name, "%s/.mochirc", home);
		path = name;
	}
	FILE *file = fopen(path, "r");
	if (!file && (errno != ENOENT || filename))
		return config_report(path, 0, strerror(errno));
	Config *cfg = calloc(1, sizeof *cfg);
	if (!cfg) {
		if (file)
			fclose(file);
		return config_report(path, 0, "out of memory");
	}
	keys_defaults(&cfg->keys);
	unsigned long number = 0;
	const char *error = NULL;
	if (file) {
		error = config_read(file, cfg, &number);
		if (fclose(file) && !error)
			error = "cannot close configuration";
	}
	bool ok = false;
	if (error)
		config_report(path, number, error);
	else if (!keys_install(&cfg->keys))
		config_report(path, 0, "prefix unavailable; previous bindings retained");
	else {
		char text[KEY_NAME_MAX + 64];

		ok = true;
		snprintf(text, sizeof text, "Prefix %s", keys_name());
		input_message(startup ? text
				      : "Configuration reloaded (startup "
					"programs skipped)");
		if (startup)
			for (size_t i = 0; i < cfg->count; ++i)
				process_spawn(cfg->startup[i]);
	}
	free(cfg);
	return ok;
}
