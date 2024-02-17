
#include <stdio.h>
#include <assert.h>

#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <graphics/gfxmacros.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <hardware/blit.h>
#include <graphics/rpattr.h>

#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/layers_protos.h>
#include <clib/graphics_protos.h>

#include "Screen.h"

#ifndef AMIGA
#define __far
#define __saveds
#define __asm
#define __regargs
#define __a1
#endif

__far extern struct Custom custom;

static struct Interrupt cis;

struct copperdata
{
    struct ViewPort *vp;
    WORD sigBit;
    struct Task *task;
};

static struct copperdata cdata;

__saveds __asm __regargs static LONG myCopper(register __a1 struct copperdata *cd)
{
    if (!(cd->vp->Modes & VP_HIDE))
    {
        Signal(cd->task, 1L << cd->sigBit);
    }
    return(0);
}

struct Window *openScreen(struct windowUser *wu)
{
    struct Screen *s;

    if (!(s = OpenScreenTags(NULL,
        SA_LikeWorkbench, TRUE,
        SA_Title, "GameX (c) 2024 Robert Szacki",
        SA_ShowTitle, FALSE,
        SA_Quiet, TRUE,
        SA_Exclusive, TRUE,
        SA_BackFill, LAYERS_NOBACKFILL,
        SA_Depth, 3,
        TAG_DONE)))
        printf("Couldn't open screen.\n");
    else
    {
        struct Window *w;

        if (!(w = OpenWindowTags(NULL,
            WA_CustomScreen, s,
            WA_Left, 0,
            WA_Top, 0,
            WA_Width, s->Width,
            WA_Height, s->Height,
            WA_Backdrop, TRUE,
            WA_Borderless, TRUE,
            WA_Activate, TRUE,
            WA_RMBTrap, TRUE,
            WA_SimpleRefresh, TRUE,
            WA_NoCareRefresh, TRUE,
            WA_BackFill, LAYERS_NOBACKFILL,
            WA_IDCMP, IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS,
            WA_ReportMouse, TRUE,
            WA_MouseQueue, 2,
            TAG_DONE)))
            printf("Couldn't open window.\n");
        else
        {
            w->UserData = (APTR)wu;
            wu->w = w;

            if (!(wu->li = NewLayerInfo()))
                printf("Couldn't create layer info.\n");
            else
            {
                InstallLayerInfoHook(wu->li, LAYERS_NOBACKFILL);
                return(w);
            }
            CloseWindow(w);
        }
        CloseScreen(s);
    }
    return(NULL);
}

void closeScreen(struct windowUser *wu)
{
    struct Layer *top;
    struct Screen *s = wu->w->WScreen;

    while (top = wu->li->top_layer)
    {
        DeleteLayer(0, top);
    }

    DisposeLayerInfo(wu->li);
    CloseWindow(wu->w);
    CloseScreen(s);
}

WORD instCopper(struct windowUser *wu)
{
    cis.is_Code = (void(*)())myCopper;
    cis.is_Data = (APTR)&cdata;
    cis.is_Node.ln_Pri = 0;

    cdata.vp = &wu->w->WScreen->ViewPort;
    cdata.task = FindTask(NULL);

    if (!((cdata.sigBit = AllocSignal(-1)) != -1))
        printf("Couldn't alloc signal.\n");
    else
    {
        struct UCopList *ucl;

        if (!(ucl = AllocMem(sizeof(*ucl), MEMF_PUBLIC | MEMF_CLEAR)))
            printf("Couldn't alloc mem.\n");
        else
        {
            CINIT(ucl, 3);
            CWAIT(ucl, 200, 0);
            CMOVE(ucl, custom.intreq, INTF_SETCLR | INTF_COPER);
            CEND(ucl);

            Forbid();
            cdata.vp->UCopIns = ucl;
            Permit();

            RethinkDisplay();

            AddIntServer(INTB_COPER, &cis);

            SetTaskPri(cdata.task, 2);
            return(cdata.sigBit);
        }
        FreeSignal(cdata.sigBit);
    }
    return(-1);
}

void remCopper(void)
{
    SetTaskPri(cdata.task, 0);
    RemIntServer(INTB_COPER, &cis);
    FreeSignal(cdata.sigBit);
}

struct Layer *createLayer(struct windowUser *wu, struct rpUser *rpu, WORD x0, WORD y0, WORD x1, WORD y1)
{
    struct Layer *l;

    if (!(rpu->l = l = CreateUpfrontHookLayer(wu->li, wu->w->RPort->BitMap, x0, y0, x1, y1, LAYERSIMPLE, LAYERS_NOBACKFILL, NULL)))
        printf("Couldn't create layer.\n");
    else
    {
        l->rp->RP_User = (APTR)rpu;
        return(l);
    }
    return(NULL);
}

struct Layer *moveLayer(struct windowUser *wu, struct rpUser *rpu, WORD x0, WORD y0, WORD x1, WORD y1)
{
    struct Layer *l = rpu->l;

    if (createLayer(wu, rpu, x0, y0, x1, y1))
    {
        /* Draw before deleting */

        DeleteLayer(0, l);
        return(rpu->l);
    }
    return(NULL);
}

