#include <stddef.h>
#include <string.h>

#include "client.h"
#include "command.h"
#include "config.h"
#include "defs.h"
#include "input.h"
#include "keys.h"
#include "process.h"
#include "wm.h"

typedef struct {
	const char *name;
	int (*run)(void);
} Command;

int split_horizontal(void)
{
	return wm_split(FALSE);
}

int split_vertical(void)
{
	return wm_split(TRUE);
}

int next_frame(void)
{
	wm_frame(1);
	return TRUE;
}

int previous_frame(void)
{
	wm_frame(-1);
	return TRUE;
}

int next_window(void)
{
	client_next(1);
	return TRUE;
}

int previous_window(void)
{
	client_next(-1);
	return TRUE;
}

int quit(void)
{
	wm_quit(FALSE);
	return TRUE;
}

int restart(void)
{
	wm_quit(TRUE);
	return TRUE;
}

int prompt(void)
{
	return input_prompt("");
}

int reload(void)
{
	return config_load(FALSE);
}

const Command commands[] = {
	{
		"split-horizontal",
		split_horizontal,
	},
	{
		"split-vertical",
		split_vertical,
	},
	{
		"next-frame",
		next_frame,
	},
	{
		"previous-frame",
		previous_frame,
	},
	{
		"next-window",
		next_window,
	},
	{
		"previous-window",
		previous_window,
	},
	{
		"delete-frame",
		wm_remove,
	},
	{
		"only-frame",
		wm_only,
	},
	{
		"close",
		client_close,
	},
	{
		"windows",
		client_list,
	},
	{
		"quit",
		quit,
	},
	{
		"restart",
		restart,
	},
	{
		"command",
		prompt,
	},
	{
		"help",
		keys_help,
	},
	{
		"reload-config",
		reload,
	},
};

char *split(char *line)
{
	char *arg;
	char *end;

	arg = line;
	while (*arg && *arg != ' ' && *arg != '\t')
		++arg;
	if (*arg)
		*arg++ = '\0';
	while (*arg == ' ' || *arg == '\t')
		++arg;
	end = arg + strlen(arg);
	while (end > arg && (end[-1] == ' ' || end[-1] == '\t'))
		*--end = '\0';
	return arg;
}

const Command *command_find(const char *name)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(commands); ++i)
		if (!strcmp(name, commands[i].name))
			return &commands[i];
	return NULL;
}

int command_valid(const char *text)
{
	char line[COMMAND_MAX];
	char *arg;

	while (*text == ' ' || *text == '\t')
		++text;
	if (strlen(text) >= sizeof line)
		return FALSE;
	strcpy(line, text);
	arg = split(line);
	if (!strcmp(line, "exec") || !strcmp(line, "select"))
		return TRUE;
	return !*arg && command_find(line) != NULL;
}

int command_run(const char *text)
{
	char line[COMMAND_MAX];
	char *arg;
	const Command *cp;

	while (*text == ' ' || *text == '\t')
		++text;
	if (!*text)
		return TRUE;
	if (strlen(text) >= sizeof line) {
		input_message("Command too long");
		return FALSE;
	}
	strcpy(line, text);
	arg = split(line);
	if (!strcmp(line, "exec"))
		return *arg ? process_spawn(arg) : input_prompt("exec ");
	if (!strcmp(line, "select"))
		return *arg ? client_select(arg) : input_prompt("select ");
	cp = command_find(line);
	if (!cp || *arg) {
		input_message("Unknown command or invalid arguments");
		return FALSE;
	}
	return cp->run();
}
