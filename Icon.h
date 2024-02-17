
#include <exec/types.h>

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

void rectFill(struct RastPort *rp, WORD x0, WORD y0, WORD x1, WORD y1);
void bltTileRastPort(struct BitMap *sbm, WORD xsrc, WORD ysrc, struct RastPort *rp, WORD xdest, WORD ydest, WORD width, WORD height);
