#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>

bool command_run(const char *line);
bool command_valid(const char *line);

#endif /* COMMAND_H */
