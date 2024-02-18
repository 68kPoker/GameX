
#ifndef SCREEN_H
#define SCREEN_H

#include <exec/types.h>

#define COPLINE 200 /* Copper interrupt line */

struct windowUser
{
    struct Window *w;
    struct Layer_Info *li;
    struct BitMap *gfx;
};

struct rpUser
{
    struct Layer *l;
    void (*draw)(struct rpUser *rpu);
};

struct Window *openScreen(struct windowUser *wu, UBYTE depth);
void closeScreen(struct windowUser *wu);
WORD instCopper(struct windowUser *wu);
void remCopper(void);
struct Layer *createLayer(struct windowUser *wu, struct rpUser *rpu, WORD x0, WORD y0, WORD x1, WORD y1);
struct Layer *moveLayer(struct windowUser *wu, struct rpUser *rpu, WORD x0, WORD y0, WORD x1, WORD y1);

#endif /* SCREEN_H */
