#ifndef FRAME_H
#define FRAME_H

#include <stdbool.h>

typedef struct Frame Frame;

struct Frame {
	Frame *parent;
	Frame *first;
	Frame *second;
	bool vertical;
	int x;
	int y;
	int width;
	int height;
	unsigned long win;
};

Frame *frame_create(void);
void frame_free(Frame *frame);
void frame_layout(Frame *frame, int x, int y, int width, int height);
bool frame_split(Frame *frame, bool vertical);
Frame *frame_remove(Frame **root, Frame *frame);
Frame *frame_next(Frame *root, Frame *frame, int direction);
Frame *frame_find(Frame *frame, unsigned long win);
void frame_only(Frame **root, Frame *frame);

#endif /* FRAME_H */
