#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xlib.h>

int display_init(void);
void display_free(void);
Window display_window(void);
void display_draw(const char *text, int tail);
void display_show(void);
void display_hide(void);

#endif /* DISPLAY_H */
