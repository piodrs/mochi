#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

int command_run(const char *line);
int command_valid(const char *line);
void command_describe(const char *text, char *buffer, size_t size);

#endif /* COMMAND_H */
