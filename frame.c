#include "frame.h"

#include <stdlib.h>

#include "defs.h"

Frame *frame_create(void)
{
	return calloc(1, sizeof(Frame));
}

void frame_free(Frame *frame)
{
	if (!frame)
		return;
	frame_free(frame->first);
	frame_free(frame->second);
	free(frame);
}

void frame_layout(Frame *frame, int x, int y, int width, int height)
{
	frame->x = x;
	frame->y = y;
	frame->width = width;
	frame->height = height;
	if (!frame->first)
		return;
	int first_size = frame->vertical ? width / 2 : height / 2;
	if (frame->vertical) {
		frame_layout(frame->first, x, y, first_size, height);
		frame_layout(frame->second, x + first_size, y, width - first_size, height);
	} else {
		frame_layout(frame->first, x, y, width, first_size);
		frame_layout(frame->second, x, y + first_size, width, height - first_size);
	}
}

bool frame_split(Frame *frame, bool vertical)
{
	if (frame->first || (vertical ? frame->width : frame->height) < FRAME_MIN * 2)
		return false;
	Frame *first = frame_create();
	Frame *second = frame_create();
	if (!first || !second) {
		free(first);
		free(second);
		return false;
	}
	first->parent = frame;
	second->parent = frame;
	first->win = frame->win;
	frame->win = 0;
	frame->first = first;
	frame->second = second;
	frame->vertical = vertical;
	return true;
}

Frame *frame_next(Frame *root, Frame *frame, int direction)
{
	Frame *parent = frame->parent;
	while (parent && frame == (direction > 0 ? parent->second : parent->first)) {
		frame = parent;
		parent = parent->parent;
	}
	frame = parent ? (direction > 0 ? parent->second : parent->first) : root;
	while (frame->first)
		frame = direction > 0 ? frame->first : frame->second;
	return frame;
}

Frame *frame_remove(Frame **root, Frame *frame)
{
	Frame *parent = frame->parent;
	if (!parent)
		return frame;
	Frame *sibling = parent->first == frame ? parent->second : parent->first;
	Frame *grandparent = parent->parent;
	sibling->parent = grandparent;
	sibling->x = parent->x;
	sibling->y = parent->y;
	sibling->width = parent->width;
	sibling->height = parent->height;
	if (!grandparent)
		*root = sibling;
	else if (grandparent->first == parent)
		grandparent->first = sibling;
	else
		grandparent->second = sibling;
	free(frame);
	free(parent);
	while (sibling->first)
		sibling = sibling->first;
	return sibling;
}

Frame *frame_find(Frame *frame, unsigned long win)
{
	if (!win)
		return NULL;
	if (!frame->first)
		return frame->win == win ? frame : NULL;
	Frame *found = frame_find(frame->first, win);
	return found ? found : frame_find(frame->second, win);
}

void frame_only(Frame **root, Frame *frame)
{
	Frame *parent = frame->parent;
	if (!parent)
		return;
	if (parent->first == frame)
		parent->first = NULL;
	else
		parent->second = NULL;
	frame->x = (*root)->x;
	frame->y = (*root)->y;
	frame->width = (*root)->width;
	frame->height = (*root)->height;
	frame_free(*root);
	frame->parent = NULL;
	*root = frame;
}
