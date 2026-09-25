#ifndef WM_H
#define WM_H

#include <stdbool.h>

enum { WM_RESTART = 2 };

int wm_run(void);
bool wm_split(bool vertical);
void wm_frame(int direction);
void wm_remove(void);
void wm_only(void);
void wm_quit(bool restart);

#endif /* WM_H */
