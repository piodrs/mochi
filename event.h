#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

void event_dispatch(XEvent *event);

#endif /* EVENT_H */
