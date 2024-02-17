
/* Graphics blocks */

/* Fast aligned blocks drawing */

#include <graphics/rpattr.h>
#include <clib/graphics_protos.h>

#include "Icon.h"

#ifndef AMIGA
#define __far
#endif

__far extern struct Custom custom;

void rectFillCR(struct RastPort *rp, WORD x0, WORD y0, WORD x1, WORD y1)
{
    LONG bpen, mask;
    struct Custom *c = &custom;
    struct BitMap *dbm = rp->BitMap;
    WORD i;
    WORD width = (x1 - x0 + 1) >> 4, height = y1 - y0 + 1;
    LONG offset = (y0 * dbm->BytesPerRow) + ((x0 >> 4) << 1);
    assert(width > 0);

    GetRPAttrs(rp,
        RPTAG_BPen, &bpen,
        RPTAG_WriteMask, &mask,
        TAG_DONE);

    OwnBlitter();

    for (i = 0; i < dbm->Depth; i++)
    {
        if (mask & 1)
        {
            WaitBlit();
            c->bltcon0 = A_TO_D | DEST;
            c->bltcon1 = 0;
            c->bltadat = -(bpen & 1);
            c->bltdpt = dbm->Planes[i] + offset;
            c->bltdmod = dbm->BytesPerRow - (width << 1);
            c->bltafwm = 0xffff;
            c->bltalwm = 0xffff;
            c->bltsize = (height << HSIZEBITS) | width;
        }
        mask >>= 1;
        bpen >>= 1;
    }

    DisownBlitter();
}

void bltTileCR(struct BitMap *sbm, WORD xsrc, WORD ysrc, struct RastPort *rp, WORD xdest, WORD ydest, WORD width, WORD height)
{
    LONG mask;
    struct Custom *c = &custom;
    struct BitMap *dbm = rp->BitMap;
    WORD i;
    LONG soffset = (ysrc * sbm->BytesPerRow) + ((xsrc >> 4) << 1);
    LONG doffset = (ydest * dbm->BytesPerRow) + ((xdest >> 4) << 1);

    GetRPAttrs(rp,
        RPTAG_WriteMask, &mask,
        TAG_DONE);

    width >>= 4;
    assert(width > 0);

    OwnBlitter();

    for (i = 0; i < dbm->Depth; i++)
    {
        if (mask & 1)
        {
            WaitBlit();
            c->bltcon0 = A_TO_D | SRCA | DEST;
            c->bltcon1 = 0;
            c->bltapt = sbm->Planes[i] + soffset;
            c->bltdpt = dbm->Planes[i] + doffset;
            c->bltamod = sbm->BytesPerRow - (width << 1);
            c->bltdmod = dbm->BytesPerRow - (width << 1);
            c->bltafwm = 0xffff;
            c->bltalwm = 0xffff;
            c->bltsize = (height << HSIZEBITS) | width;
        }
        mask >>= 1;
    }

    DisownBlitter();
}

void rectFill(struct RastPort *rp, WORD x0, WORD y0, WORD x1, WORD y1)
{
    struct ClipRect *cr;
    struct Rectangle *lb = &rp->Layer->bounds;

    x0 += lb->MinX;
    x1 += lb->MinX;
    y0 += lb->MinY;
    y1 += lb->MinY;

    for (cr = rp->Layer->ClipRect; cr != NULL; cr = cr->Next)
    {
        if (!cr->lobs)
        {
            struct Rectangle *crb = &cr->bounds;

            WORD xs = MAX(x0, crb->MinX);
            WORD xe = MIN(x1, crb->MaxX);
            WORD ys = MAX(y0, crb->MinY);
            WORD ye = MIN(y1, crb->MaxY);

            if (xs < xe && ys < ye)
            {
                rectFillCR(rp, xs, ys, xe, ye);
            }
        }
    }
}

void bltTileRastPort(struct BitMap *sbm, WORD xsrc, WORD ysrc, struct RastPort *rp, WORD xdest, WORD ydest, WORD width, WORD height)
{
    struct ClipRect *cr;
    struct Rectangle *lb = &rp->Layer->bounds;

    xdest += lb->MinX;
    ydest += lb->MinY;

    for (cr = rp->Layer->ClipRect; cr != NULL; cr = cr->Next)
    {
        if (!cr->lobs)
        {
            struct Rectangle *crb = &cr->bounds;

            WORD xs = MAX(xdest, crb->MinX);
            WORD xe = MIN(xdest + width - 1, crb->MaxX);
            WORD ys = MAX(ydest, crb->MinY);
            WORD ye = MIN(ydest + height - 1, crb->MaxY);

            WORD offx = xs - xdest;
            WORD offy = ys - ydest;

            if (xs < xe && ys < ye)
            {
                bltTileCR(sbm, xsrc + offx, ysrc + offy, rp, xs, ys, xe - xs + 1, ye - ys + 1);
            }
        }
    }
}
