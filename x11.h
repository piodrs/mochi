#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>

int x11_open(void);
void x11_trap(void);
int x11_untrap(void);
void x11_state(Window win, long value);
void x11_protocol(Window win, Atom atom);
int x11_iconic(XClientMessageEvent *event);
Time x11_time(Window win);

#endif /* X11_H */
