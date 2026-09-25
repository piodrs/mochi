#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>

bool process_spawn(const char *command);
void process_reap(void);
void process_free(void);

#endif /* PROCESS_H */
