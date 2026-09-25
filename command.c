#include "command.h"

#include <stddef.h>
#include <string.h>

#include "client.h"
#include "config.h"
#include "defs.h"
#include "input.h"
#include "keys.h"
#include "process.h"
#include "wm.h"

typedef enum {
	COMMAND_SPLIT_HORIZONTAL,
	COMMAND_SPLIT_VERTICAL,
	COMMAND_NEXT_FRAME,
	COMMAND_PREVIOUS_FRAME,
	COMMAND_NEXT_WINDOW,
	COMMAND_PREVIOUS_WINDOW,
	COMMAND_DELETE_FRAME,
	COMMAND_ONLY_FRAME,
	COMMAND_CLOSE,
	COMMAND_WINDOWS,
	COMMAND_QUIT,
	COMMAND_RESTART,
	COMMAND_PROMPT,
	COMMAND_HELP,
	COMMAND_RELOAD,
} CommandAction;

typedef struct {
	const char *name;
	CommandAction action;
} Command;

static const Command commands[] = {
	{"split-horizontal", COMMAND_SPLIT_HORIZONTAL},
	{"split-vertical", COMMAND_SPLIT_VERTICAL},
	{"next-frame", COMMAND_NEXT_FRAME},
	{"previous-frame", COMMAND_PREVIOUS_FRAME},
	{"next-window", COMMAND_NEXT_WINDOW},
	{"previous-window", COMMAND_PREVIOUS_WINDOW},
	{"delete-frame", COMMAND_DELETE_FRAME},
	{"only-frame", COMMAND_ONLY_FRAME},
	{"close", COMMAND_CLOSE},
	{"windows", COMMAND_WINDOWS},
	{"quit", COMMAND_QUIT},
	{"restart", COMMAND_RESTART},
	{"command", COMMAND_PROMPT},
	{"help", COMMAND_HELP},
	{"reload-config", COMMAND_RELOAD},
};

static char *command_split(char *line)
{
	char *arg = line;
	while (*arg && *arg != ' ' && *arg != '\t')
		++arg;
	if (*arg)
		*arg++ = '\0';
	while (*arg == ' ' || *arg == '\t')
		++arg;
	char *end = arg + strlen(arg);
	while (end > arg && (end[-1] == ' ' || end[-1] == '\t'))
		*--end = '\0';
	return arg;
}

static const Command *command_find(const char *name)
{
	for (size_t i = 0; i < ARRAY_SIZE(commands); ++i)
		if (!strcmp(name, commands[i].name))
			return &commands[i];
	return NULL;
}

bool command_valid(const char *text)
{
	char line[COMMAND_MAX];

	while (*text == ' ' || *text == '\t')
		++text;
	if (strlen(text) >= sizeof line)
		return false;
	strcpy(line, text);
	char *arg = command_split(line);
	if (!strcmp(line, "exec") || !strcmp(line, "select"))
		return true;
	return !*arg && command_find(line) != NULL;
}

bool command_run(const char *text)
{
	char line[COMMAND_MAX];

	while (*text == ' ' || *text == '\t')
		++text;
	if (!*text)
		return true;
	if (strlen(text) >= sizeof line) {
		input_message("Command too long");
		return false;
	}
	strcpy(line, text);
	char *arg = command_split(line);
	if (!strcmp(line, "exec"))
		return *arg ? process_spawn(arg) : input_prompt("exec ");
	if (!strcmp(line, "select"))
		return *arg ? client_select(arg) : input_prompt("select ");
	const Command *command = command_find(line);
	if (!command || *arg) {
		input_message("Unknown command or invalid arguments");
		return false;
	}
	switch (command->action) {
	case COMMAND_SPLIT_HORIZONTAL:
		return wm_split(false);
	case COMMAND_SPLIT_VERTICAL:
		return wm_split(true);
	case COMMAND_NEXT_FRAME:
		wm_frame(1);
		break;
	case COMMAND_PREVIOUS_FRAME:
		wm_frame(-1);
		break;
	case COMMAND_NEXT_WINDOW:
		client_next(1);
		break;
	case COMMAND_PREVIOUS_WINDOW:
		client_next(-1);
		break;
	case COMMAND_DELETE_FRAME:
		wm_remove();
		break;
	case COMMAND_ONLY_FRAME:
		wm_only();
		break;
	case COMMAND_CLOSE:
		return client_close();
	case COMMAND_WINDOWS:
		client_list();
		break;
	case COMMAND_QUIT:
		wm_quit(false);
		break;
	case COMMAND_RESTART:
		wm_quit(true);
		break;
	case COMMAND_PROMPT:
		return input_prompt("");
	case COMMAND_HELP:
		keys_help();
		break;
	case COMMAND_RELOAD:
		return config_load(false);
	}
	return true;
}
