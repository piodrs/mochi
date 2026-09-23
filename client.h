#ifndef CLIENT_H
#define CLIENT_H

#include <X11/Xlib.h>

void client_refresh(void);
void client_next(int direction);
int client_close(void);
int client_select(const char *arg);
int client_list(void);
void client_map(Window win);
void client_unmap(XUnmapEvent *event);
void client_destroy(Window win);
void client_configure(XConfigureRequestEvent *event);
void client_message(XClientMessageEvent *event);
void client_scan(void);
void client_property(XPropertyEvent *event);
void client_free(void);

#endif /* CLIENT_H */
