#define _POSIX_C_SOURCE 200809L

#include "process.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "defs.h"
#include "input.h"
#include "session.h"

typedef struct Process Process;

struct Process {
	Process *next;
	pid_t pid;
	char command[COMMAND_MAX];
};

static Process *processes;

static void process_failure(const char *command, int status)
{
	char message[MESSAGE_MAX];

	if (WIFSIGNALED(status))
		snprintf(message, sizeof message, "Program killed by signal %d: %s",
			WTERMSIG(status), command);
	else
		snprintf(message, sizeof message, "Program exited with status %d: %s",
			WEXITSTATUS(status), command);
	fprintf(stderr, APP_NAME ": %s\n", message);
	input_message(message);
}

bool process_spawn(const char *command)
{
	if (!*command || strlen(command) >= COMMAND_MAX)
		return false;
	Process *process = malloc(sizeof *process);
	if (!process) {
		input_message("Cannot allocate program entry");
		return false;
	}
	pid_t pid = fork();
	if (pid < 0) {
		free(process);
		input_message("Cannot start program");
		return false;
	}
	if (!pid) {
		struct sigaction action = {.sa_handler = SIG_DFL};
		sigset_t mask;

		close(ConnectionNumber(mochi.display));
		sigemptyset(&action.sa_mask);
		sigemptyset(&mask);
		if (sigaction(SIGCHLD, &action, NULL) < 0 || sigaction(SIGINT, &action, NULL) < 0 ||
			sigaction(SIGTERM, &action, NULL) < 0 ||
			sigaction(SIGHUP, &action, NULL) < 0 ||
			sigaction(SIGPIPE, &action, NULL) < 0 ||
			sigprocmask(SIG_SETMASK, &mask, NULL) < 0 || setsid() < 0)
			_exit(126);
		execl("/bin/sh", "sh", "-c", command, (char *)NULL);
		_exit(127);
	}

	process->pid = pid;
	strcpy(process->command, command);
	process->next = processes;
	processes = process;
	return true;
}

void process_reap(void)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		Process **link;
		for (link = &processes; *link && (*link)->pid != pid; link = &(*link)->next)
			;
		Process *process = *link;
		if (!process)
			continue;
		*link = process->next;
		if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status)))
			process_failure(process->command, status);
		free(process);
	}
}

void process_free(void)
{
	while (processes) {
		Process *process = processes;
		processes = process->next;
		free(process);
	}
}
