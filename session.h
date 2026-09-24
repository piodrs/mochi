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
	int status;
} Session;

extern Session mochi;

#endif /* SESSION_H */
