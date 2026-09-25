#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include <X11/Xlib.h>

void input_key(const XKeyEvent *event);
void input_draw(void);
bool input_prompt(const char *text);
void input_message(const char *text);
void input_tick(void);
void input_cancel(void);

#endif /* INPUT_H */
