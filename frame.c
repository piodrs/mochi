#include <stdlib.h>
#include "defs.h"
#include "frame.h"

Frame *frame_create(void)
{
	return calloc(1, sizeof(Frame));
}

void frame_free(Frame *fp)
{
	if (!fp)
		return;
	frame_free(fp->first);
	frame_free(fp->second);
	free(fp);
}

void frame_layout(Frame *fp, int x, int y, int width, int height)
{
	int n;

	fp->x = x;
	fp->y = y;
	fp->width = width;
	fp->height = height;
	if (!fp->first)
		return;
	n = fp->vertical ? width / 2 : height / 2;
	if (fp->vertical) {
		frame_layout(fp->first, x, y, n, height);
		frame_layout(fp->second, x + n, y, width - n, height);
	} else {
		frame_layout(fp->first, x, y, width, n);
		frame_layout(fp->second, x, y + n, width, height - n);
	}
}

int frame_split(Frame *fp, int vertical)
{
	Frame *a;
	Frame *b;

	if (fp->first || (vertical ? fp->width : fp->height) < FRAME_MIN * 2)
		return FALSE;
	a = frame_create();
	b = frame_create();
	if (!a || !b) {
		free(a);
		free(b);
		return FALSE;
	}
	a->parent = fp;
	b->parent = fp;
	a->win = fp->win;
	fp->win = 0;
	fp->first = a;
	fp->second = b;
	fp->vertical = vertical;
	return TRUE;
}

static Frame *frame_first(Frame *fp)
{
	while (fp->first)
		fp = fp->first;
	return fp;
}

Frame *frame_next(Frame *root, Frame *fp, int direction)
{
	Frame *pp;

	pp = fp->parent;
	while (pp && fp == (direction > 0 ? pp->second : pp->first)) {
		fp = pp;
		pp = pp->parent;
	}
	fp = pp ? (direction > 0 ? pp->second : pp->first) : root;
	while (fp->first)
		fp = direction > 0 ? fp->first : fp->second;
	return fp;
}

Frame *frame_remove(Frame **root, Frame *fp)
{
	Frame *pp;
	Frame *sp;
	Frame *gp;

	pp = fp->parent;
	if (!pp)
		return fp;
	sp = pp->first == fp ? pp->second : pp->first;
	gp = pp->parent;
	sp->parent = gp;
	sp->x = pp->x;
	sp->y = pp->y;
	sp->width = pp->width;
	sp->height = pp->height;
	if (!gp)
		*root = sp;
	else if (gp->first == pp)
		gp->first = sp;
	else
		gp->second = sp;
	free(fp);
	free(pp);
	return frame_first(sp);
}

Frame *frame_find(Frame *fp, unsigned long win)
{
	Frame *found;

	if (!win)
		return NULL;
	if (!fp->first)
		return fp->win == win ? fp : NULL;
	found = frame_find(fp->first, win);
	return found ? found : frame_find(fp->second, win);
}

void frame_only(Frame **root, Frame *fp)
{
	Frame *pp;

	pp = fp->parent;
	if (!pp)
		return;
	if (pp->first == fp)
		pp->first = NULL;
	else
		pp->second = NULL;
	fp->x = (*root)->x;
	fp->y = (*root)->y;
	fp->width = (*root)->width;
	fp->height = (*root)->height;
	frame_free(*root);
	fp->parent = NULL;
	*root = fp;
}
