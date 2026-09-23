#ifndef WM_H
#define WM_H

#define WM_RESTART 2

int wm_run(void);
int wm_split(int vertical);
void wm_frame(int direction);
int wm_remove(void);
int wm_only(void);
void wm_quit(int restart);

#endif /* WM_H */
