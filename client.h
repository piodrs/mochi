#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

#include <X11/Xlib.h>

void client_refresh(void);
void client_next(int direction);
bool client_close(void);
bool client_select(const char *arg);
void client_list(void);
void client_map(Window win);
void client_unmap(const XUnmapEvent *event);
void client_destroy(Window win);
void client_configure(const XConfigureRequestEvent *event);
void client_message(const XClientMessageEvent *event);
void client_scan(void);
void client_property(const XPropertyEvent *event);
void client_free(void);

#endif /* CLIENT_H */
