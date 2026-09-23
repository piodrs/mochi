#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "config.h"
#include "defs.h"
#include "input.h"
#include "keys.h"
#include "process.h"

#define STARTUP_MAX 32
#define PATH_MAXIMUM 4096

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

static char *skip(char *text)
{
	while (*text == ' ' || *text == '\t' || *text == '\r')
		++text;
	return text;
}

static char *word(char **rest)
{
	char *start;
	char *end;

	start = skip(*rest);
	end = start;
	while (*end && *end != ' ' && *end != '\t' && *end != '\r')
		++end;
	if (*end)
		*end++ = '\0';
	*rest = skip(end);
	return start;
}

static const char *parse(Config *cfg, char *line)
{
	char *op;
	char *arg;
	Key key;
	size_t len;

	len = strlen(line);
	if (len && line[len - 1] == '\r')
		line[len - 1] = '\0';
	line = skip(line);
	if (!*line || *line == '#')
		return NULL;
	op = word(&line);
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
	arg = word(&line);
	if (!keys_parse(arg, &key))
		return "invalid key (C-g and Escape are reserved for cancel)";
	if (!strcmp(op, "bind")) {
		if (!*line || !command_valid(line))
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

static const char *read_config(FILE *file, Config *cfg, unsigned long *number)
{
	char line[COMMAND_MAX];
	const char *error;
	size_t len;
	int ch;

	len = 0;
	*number = 1;
	while ((ch = fgetc(file)) != EOF) {
		if (ch != '\n') {
			if (!ch || len == sizeof line - 1)
				return "line too long or contains a NUL byte";
			line[len++] = ch;
			continue;
		}
		line[len] = '\0';
		error = parse(cfg, line);
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
	return parse(cfg, line);
}

static int report(const char *path, unsigned long line, const char *error)
{
	char text[MESSAGE_MAX];

	sprintf(text, "%.1023s:%lu: %.900s", path, line, error);
	fprintf(stderr, APP_NAME ": %s\n", text);
	input_message(text);
	return FALSE;
}

int config_load(int startup)
{
	Config *cfg;
	FILE *file;
	const char *path;
	const char *home;
	const char *error;
	char name[PATH_MAXIMUM];
	char text[KEY_NAME_MAX + 64];
	unsigned long number;
	size_t i;
	int ok;

	path = filename;
	if (!path) {
		home = getenv("HOME");
		if (!home || !*home)
			return report("~/.fishrc", 0, "HOME is not set");
		if (strlen(home) > sizeof name - sizeof "/.fishrc")
			return report("~/.fishrc", 0, "path too long");
		sprintf(name, "%s/.fishrc", home);
		path = name;
	}
	file = fopen(path, "r");
	if (!file && (errno != ENOENT || filename))
		return report(path, 0, strerror(errno));
	cfg = calloc(1, sizeof *cfg);
	if (!cfg) {
		if (file)
			fclose(file);
		return report(path, 0, "out of memory");
	}
	keys_defaults(&cfg->keys);
	number = 0;
	error = NULL;
	if (file) {
		error = read_config(file, cfg, &number);
		if (fclose(file) && !error)
			error = "cannot close configuration";
	}
	ok = FALSE;
	if (error)
		report(path, number, error);
	else if (!keys_install(&cfg->keys))
		report(path, 0,
		       "prefix unavailable; previous bindings retained");
	else {
		ok = TRUE;
		sprintf(text, "Prefix %s", keys_name());
		input_message(startup ? text
				      : "Configuration reloaded (startup "
					"programs skipped)");
		if (startup)
			for (i = 0; i < cfg->count; ++i)
				process_spawn(cfg->startup[i]);
	}
	free(cfg);
	return ok;
}
