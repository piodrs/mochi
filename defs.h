#ifndef DEFS_H
#define DEFS_H

#define APP_NAME "mochiWM"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
enum { COMMAND_MAX = 1024, MESSAGE_MAX = 2048, FRAME_MIN = 32, EVENT_BATCH = 64, POLL_MS = 100 };

#endif /* DEFS_H */
