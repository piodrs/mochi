#ifndef SESSION_H
#define SESSION_H

#include <X11/Xlib.h>

struct Frame;

typedef struct {
	Display *display;
	Window root;
	struct Frame *tree;
	struct Frame *frame;
	Time time;
	int status;
} Session;

extern Session mochi;

#endif /* SESSION_H */
