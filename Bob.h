
/* Blitter objects */

#include <graphics/gels.h>

#define WordWidth(w) (((w)+15)>>4)

struct GelsInfo *initGels(struct RastPort *rp);
void cleanGels(struct GelsInfo *gi);

struct VSprite *newVSprite(WORD x, WORD y, WORD width, WORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff, WORD *imageData, WORD flags);
struct Bob *newBob(WORD x, WORD y, WORD width, WORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff, WORD *imageData, WORD bobFlags, WORD vsFlags, UBYTE rasDepth);

void disposeVSprite(struct VSprite *vs);
void disposeBob(struct Bob *bob);

void drawGList(struct RastPort *rp); /* Custom DrawGList() */

void storeBack(struct RastPort *rp, struct VSprite *vs, struct Bob *bob);
void restoreBack(struct RastPort *rp, struct VSprite *vs, struct Bob *bob);
void drawBob(struct RastPort *rp, struct VSprite *vs, struct Bob *bob);
