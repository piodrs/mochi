#ifndef X11_H
#define X11_H

#include <stdbool.h>

#include <X11/Xlib.h>

bool x11_open(void);
void x11_trap(void);
int x11_untrap(void);
bool x11_hidden(Window win);
void x11_state(Window win, long value);
void x11_protocol(Window win, Atom atom);
bool x11_iconic(const XClientMessageEvent *event);
Time x11_time(Window win);

#endif /* X11_H */
