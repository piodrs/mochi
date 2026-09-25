#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

void config_path(const char *path);
bool config_load(bool startup);

#endif /* CONFIG_H */
