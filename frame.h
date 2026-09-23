#ifndef FRAME_H
#define FRAME_H

typedef struct Frame Frame;

struct Frame {
	Frame *parent;
	Frame *first;
	Frame *second;
	int vertical;
	int x;
	int y;
	int width;
	int height;
	unsigned long win;
};

Frame *frame_create(void);
void frame_free(Frame *fp);
void frame_layout(Frame *fp, int x, int y, int width, int height);
int frame_split(Frame *fp, int vertical);
Frame *frame_remove(Frame **root, Frame *fp);
Frame *frame_next(Frame *root, Frame *fp, int direction);
Frame *frame_find(Frame *fp, unsigned long win);
void frame_only(Frame **root, Frame *fp);

#endif /* FRAME_H */
