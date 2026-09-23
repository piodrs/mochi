#ifndef SESSION_H
#define SESSION_H

#include <X11/Xlib.h>
#include "frame.h"

typedef struct {
	Display *display;
	Window root;
	Frame *tree;
	Frame *frame;
	Time time;
	int running;
	int restart;
} Session;

extern Session fish;

#endif /* SESSION_H */
