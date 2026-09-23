#ifndef DEFS_H
#define DEFS_H

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#define APP_NAME "πFish"

#define FALSE 0
#define TRUE 1
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define COMMAND_MAX 1024
#define MESSAGE_MAX 2048
#define FRAME_MIN 32
#define EVENT_BATCH 64
#define POLL_MS 100

#endif /* DEFS_H */
