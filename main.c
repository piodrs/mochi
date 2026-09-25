#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "config.h"
#include "defs.h"
#include "wm.h"

int main(int argc, char **argv)
{
	if (argc == 3 && !strcmp(argv[1], "-c"))
		config_path(argv[2]);
	else if (argc != 1) {
		fprintf(stderr, "usage: %s [-c file]\n", argv[0]);
		return 1;
	}
	int status = wm_run();
	if (status == WM_RESTART) {
		execvp(argv[0], argv);
		perror(APP_NAME ": restart");
		return 1;
	}
	return status;
}
