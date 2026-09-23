#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "defs.h"
#include "input.h"
#include "process.h"
#include "session.h"

typedef struct Process Process;

struct Process {
	Process *next;
	pid_t pid;
	char command[COMMAND_MAX];
};

static Process *head;

static void failure(const char *command, int status)
{
	char message[MESSAGE_MAX];

	if (WIFSIGNALED(status))
		snprintf(message, sizeof message,
			 "Program killed by signal %d: %s", WTERMSIG(status),
			 command);
	else
		snprintf(message, sizeof message,
			 "Program exited with status %d: %s",
			 WEXITSTATUS(status), command);
	fprintf(stderr, APP_NAME ": %s\n", message);
	input_message(message);
}

int process_spawn(const char *command)
{
	Process *pp;
	pid_t pid;
	struct sigaction action;
	sigset_t mask;

	if (!*command || strlen(command) >= COMMAND_MAX)
		return FALSE;
	pp = malloc(sizeof *pp);
	if (!pp) {
		input_message("Cannot allocate program entry");
		return FALSE;
	}
	pid = fork();
	if (pid < 0) {
		free(pp);
		input_message("Cannot start program");
		return FALSE;
	}
	if (!pid) {
		close(ConnectionNumber(fish.display));
		memset(&action, 0, sizeof action);
		sigemptyset(&action.sa_mask);
		action.sa_handler = SIG_DFL;
		sigemptyset(&mask);
		if (sigaction(SIGCHLD, &action, NULL) < 0 ||
		    sigaction(SIGINT, &action, NULL) < 0 ||
		    sigaction(SIGTERM, &action, NULL) < 0 ||
		    sigaction(SIGHUP, &action, NULL) < 0 ||
		    sigaction(SIGPIPE, &action, NULL) < 0 ||
		    sigprocmask(SIG_SETMASK, &mask, NULL) < 0 || setsid() < 0)
			_exit(126);
		execl("/bin/sh", "sh", "-c", command, (char *)NULL);
		_exit(127);
	}

	pp->pid = pid;
	strcpy(pp->command, command);
	pp->next = head;
	head = pp;
	return TRUE;
}

void process_reap(void)
{
	Process **link;
	Process *pp;
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		for (link = &head; *link && (*link)->pid != pid;
		     link = &(*link)->next)
			;
		pp = *link;
		if (!pp)
			continue;
		*link = pp->next;
		if (WIFSIGNALED(status) ||
		    (WIFEXITED(status) && WEXITSTATUS(status)))
			failure(pp->command, status);
		free(pp);
	}
}

void process_free(void)
{
	Process *pp;

	while (head) {
		pp = head;
		head = pp->next;
		free(pp);
	}
}
