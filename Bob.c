
/* Blitter Objects */

/* Graphics.library BOBs */

#include <assert.h>
#include <stdio.h>

#include "Bob.h"

#include <exec/memory.h>
#include <hardware/custom.h>
#include <hardware/blit.h>

#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>

#define AB_OR_NAC 0xca

#ifndef AMIGA
#define __far
#endif

__far extern struct Custom custom;

struct GelsInfo *initGels(struct RastPort *rp)
{
    struct GelsInfo *gi;
    struct VSprite *head, *tail;

    if (gi = AllocMem(sizeof(*gi), MEMF_PUBLIC | MEMF_CLEAR))
    {
        if (head = AllocMem(sizeof(*head), MEMF_PUBLIC | MEMF_CLEAR))
        {
            if (tail = AllocMem(sizeof(*tail), MEMF_PUBLIC | MEMF_CLEAR))
            {
                InitGels(head, tail, gi);
                rp->GelsInfo = gi;
                return(gi);
            }
            FreeMem(head, sizeof(*head));
        }
        FreeMem(gi, sizeof(*gi));
    }
    return(NULL);
}

void cleanGels(struct GelsInfo *gi)
{
    FreeMem(gi->gelTail, sizeof(*gi->gelTail));
    FreeMem(gi->gelHead, sizeof(*gi->gelHead));
    FreeMem(gi, sizeof(*gi));
}

struct VSprite *newVSprite(WORD x, WORD y, WORD width, WORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff, WORD *imageData, WORD flags)
{
    struct VSprite *vs;

    if (vs = AllocMem(sizeof(*vs), MEMF_PUBLIC | MEMF_CLEAR))
    {
        vs->X = x;
        vs->Y = y;
        vs->Width = WordWidth(width);
        vs->Height = height;
        vs->Depth = depth;
        vs->ImageData = imageData;
        vs->PlanePick = planePick;
        vs->PlaneOnOff = planeOnOff;
        vs->Flags = flags;
        return(vs);
    }
    return(NULL);
}

void disposeVSprite(struct VSprite *vs)
{
    FreeMem(vs, sizeof(*vs));
}

struct Bob *newBob(WORD x, WORD y, WORD width, WORD height, UBYTE depth, UBYTE planePick, UBYTE planeOnOff, WORD *imageData, WORD bobFlags, WORD vsFlags, UBYTE rasDepth)
{
    struct VSprite *vs;
    struct Bob *bob;

    if (vs = newVSprite(x, y, width, height, depth, planePick, planeOnOff, imageData, vsFlags))
    {
        if (bob = AllocMem(sizeof(*bob), MEMF_PUBLIC | MEMF_CLEAR))
        {
            bob->BobVSprite = vs;
            vs->VSBob = bob;
            bob->Flags = bobFlags;
            if (bobFlags & SAVEBACK)
            {
                bob->SaveBuffer = AllocVec((vs->Width + 1) * sizeof(UWORD) * vs->Height * rasDepth, MEMF_CHIP);
            }
            if (bobFlags & OVERLAY)
            {
                bob->ImageShadow = AllocVec(vs->Width * sizeof(UWORD) * vs->Height, MEMF_CHIP);
            }
            return(bob);                        
        }
        disposeVSprite(vs);
    }
    return(NULL);
}

void disposeBob(struct Bob *bob)
{
    struct VSprite *vs = bob->BobVSprite;

    if (bob->ImageShadow)
    {
        FreeVec(bob->ImageShadow);
    }
    if (bob->SaveBuffer)
    {
        FreeVec(bob->SaveBuffer);
    }
    FreeMem(bob, sizeof(*bob));
    disposeVSprite(vs);
}

void drawGList(struct RastPort *rp)
{
    struct GelsInfo *gi = rp->GelsInfo;
    assert(gi);
    struct VSprite *vs;

    for (vs = gi->gelHead->NextVSprite; vs != gi->gelTail; vs = vs->NextVSprite)
    {
        struct Bob *bob = vs->VSBob;
        assert(bob);

        if ((vs->Flags & (SAVEBACK | BACKSAVED)) == (SAVEBACK | BACKSAVED))
        {
            restoreBack(rp, vs, bob);
        }
    }

    for (vs = gi->gelHead->NextVSprite; vs != gi->gelTail; vs = vs->NextVSprite)
    {
        struct Bob *bob = vs->VSBob;
        assert(bob);

        if (vs->Flags & SAVEBACK)
        {
            storeBack(rp, vs, bob);
            vs->Flags |= BACKSAVED;
        }
    }

    for (vs = gi->gelHead->NextVSprite; vs != gi->gelTail; vs = vs->NextVSprite)
    {
        struct Bob *bob = vs->VSBob;
        assert(bob);
        
        drawBob(rp, vs, bob);
    }
}

void restoreBack(struct RastPort *rp, struct VSprite *vs, struct Bob *bob)
{
    struct Custom *c = &custom;
    UBYTE p, depth = rp->BitMap->Depth;
    struct BitMap *bm = rp->BitMap;
    WORD bpr = bm->BytesPerRow;
    UWORD *buffer = bob->SaveBuffer;
    LONG size = (vs->Width + 1) * vs->Height;

    OwnBlitter();

    for (p = 0; p < depth; p++)
    {
        WaitBlit();
        c->bltcon0 = A_TO_D | SRCA | DEST;
        c->bltcon1 = 0;
        c->bltapt = buffer;
        c->bltdpt = bm->Planes[p] + (vs->OldY * bpr) + ((vs->OldX >> 4) << 1);
        c->bltamod = 0;
        c->bltdmod = bpr - ((vs->Width + 1) << 1);
        c->bltafwm = 0xffff;
        c->bltalwm = 0xffff;
        c->bltsize = (vs->Height << HSIZEBITS) | (vs->Width + 1);

        buffer += size;
    }
    DisownBlitter();
}

void storeBack(struct RastPort *rp, struct VSprite *vs, struct Bob *bob)
{
    struct Custom *c = &custom;
    UBYTE p, depth = rp->BitMap->Depth;
    struct BitMap *bm = rp->BitMap;
    WORD bpr = bm->BytesPerRow;
    UWORD *buffer = bob->SaveBuffer;
    LONG size = (vs->Width + 1) * vs->Height;

    OwnBlitter();

    for (p = 0; p < depth; p++)
    {
        WaitBlit();
        c->bltcon0 = A_TO_D | SRCA | DEST;
        c->bltcon1 = 0;
        c->bltapt = bm->Planes[p] + (vs->Y * bpr) + ((vs->X >> 4) << 1);
        c->bltdpt = buffer;
        c->bltamod = bpr - ((vs->Width + 1) << 1);
        c->bltdmod = 0;
        c->bltafwm = 0xffff;
        c->bltalwm = 0xffff;
        c->bltsize = (vs->Height << HSIZEBITS) | (vs->Width + 1);

        buffer += size;
    }

    DisownBlitter();

    vs->OldX = vs->X;
    vs->OldY = vs->Y;
}

void drawBob(struct RastPort *rp, struct VSprite *vs, struct Bob *bob)
{
    struct Custom *c = &custom;
    UBYTE p;
    UWORD *imageData = vs->ImageData;
    UBYTE planePick = vs->PlanePick;
    UBYTE planeOnOff = vs->PlaneOnOff;
    LONG size = vs->Width * vs->Height;
    UBYTE depth = rp->BitMap->Depth;
    UBYTE shift = vs->X & 0xf;
    struct BitMap *bm = rp->BitMap;
    WORD bpr = bm->BytesPerRow;
    PLANEPTR dest;
    UWORD con0 = AB_OR_NAC | SRCC | DEST | (shift << ASHIFTSHIFT);
    UWORD con1 = 0;

    if (vs->Flags & OVERLAY)
    {
        con0 |= SRCA;
    }    

    OwnBlitter();

    for (p = 0; p < depth; p++)
    {
        if (planePick & 1)
        {
            /* Draw image data */
            WaitBlit();
            c->bltcon0 = con0 | SRCB;
            c->bltcon1 = con1 | (shift << BSHIFTSHIFT);
            c->bltapt = bob->ImageShadow;
            c->bltadat = 0xffff;
            c->bltbpt = imageData;
            c->bltcpt = dest = bm->Planes[p] + (vs->Y * bpr) + ((vs->X >> 4) << 1);
            c->bltdpt = dest;
            c->bltamod = -2;
            c->bltbmod = -2;
            c->bltcmod = bpr - ((vs->Width + 1) << 1);
            c->bltdmod = bpr - ((vs->Width + 1) << 1);
            c->bltafwm = 0xffff;
            c->bltalwm = 0;
            c->bltsize = (vs->Height << HSIZEBITS) | (vs->Width + 1);

            imageData += size;
        }
        else
        {
            /* Draw 0s or 1s */

            UWORD data = -(planeOnOff & 1);

            WaitBlit();
            c->bltcon0 = con0;
            c->bltcon1 = con1;
            c->bltapt = bob->ImageShadow;
            c->bltadat = 0xffff;
            c->bltbdat = data;
            c->bltcpt = dest = bm->Planes[p] + (vs->Y * bpr) + ((vs->X >> 4) << 1);
            c->bltdpt = dest;
            c->bltamod = -2;
            c->bltcmod = bpr - ((vs->Width + 1) << 1);
            c->bltdmod = bpr - ((vs->Width + 1) << 1);
            c->bltafwm = 0xffff;
            c->bltalwm = 0;
            c->bltsize = (vs->Height << HSIZEBITS) | (vs->Width + 1);
        }
        planePick >>= 1;
        planeOnOff >>= 1;
    }

    DisownBlitter();
}

UWORD *createImageData(struct BitMap *bm, WORD width, WORD height, WORD count)
{
    UWORD *imageData, *curData;
    UBYTE depth = bm->Depth, p;
    WORD i;
    LONG imagePlaneSize = width * height, imageSize = imagePlaneSize * depth;
    struct BitMap aux;
    WORD x, y, imagesPerRow = GetBitMapAttr(bm, BMA_WIDTH) / (width << 4);

    if (imageData = AllocVec(imageSize * sizeof(UWORD) * count, MEMF_CHIP))
    {
        InitBitMap(&aux, depth, width << 4, height);
        curData = imageData;
        for (i = 0; i < count; i++)
        {
            for (p = 0; p < depth; p++)
            {
                aux.Planes[p] = curData;
                curData += imagePlaneSize;
            }
            x = (i % imagesPerRow) * (width << 4);
            y = (i / imagesPerRow) * height;
            BltBitMap(bm, x, y, &aux, 0, 0, width << 4, height, 0xc0, 0xff, NULL);
        }
        return(imageData);
    }
    return(NULL);
}
