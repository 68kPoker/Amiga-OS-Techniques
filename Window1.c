
#include <stdio.h>
#include <string.h>

#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <intuition/gadgetclass.h>

#include <clib/intuition_protos.h>
#include <clib/diskfont_protos.h>
#include <clib/graphics_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/exec_protos.h>

#define RGB(c) ((c)|((c)<<8)|((c)<<16)|((c)<<24))

STRPTR label = "Hello, world!";

WORD zoom[] =
{
    0, 0, 640, 256,
};

static struct TextAttr ta = {
    "Personal.font", 8,
    FS_NORMAL,
    FPF_ROMFONT | FPF_DESIGNED
};

struct TextFont *tf;

void drawBar(struct BitMap *bm, struct Window *w, UBYTE back, UBYTE text, STRPTR label)
{
    struct RastPort rport, *rp = &rport;
    WORD height = 16;
    UBYTE pen = back;

    InitRastPort(rp);
    rp->BitMap = bm;
    SetFont(rp, tf);

    SetAPen(rp, 2);
    Move(rp, 0, height);
    Draw(rp, 0, 0);
    Draw(rp, w->WScreen->Width - 1, 0);

    SetAPen(rp, pen);
    RectFill(rp, 1, 1, w->WScreen->Width - 2, height - 1);

    SetABPenDrMd(rp, text, 0, JAM1);
    Move(rp, 4, 4 + rp->Font->tf_Baseline);
    Text(rp, label, strlen(label));

    SetAPen(rp, 1);
    Move(rp, 0, height);
    Draw(rp, w->WScreen->Width - 1, height);
    Draw(rp, w->WScreen->Width - 1, 0);
}

struct Window *openWindow(struct Screen *s)
{
    struct Window *w;

    if (w = OpenWindowTags(NULL,
        WA_CustomScreen,    s,
        WA_Left,            64,
        WA_Top,             32,
        WA_Width,           s->Width - 128,
        WA_Height,          s->Height - 64,
        WA_Borderless,      FALSE,
        WA_GimmeZeroZero,   FALSE,
        WA_SimpleRefresh,   TRUE,
        WA_RMBTrap,         TRUE,
        WA_BackFill,        LAYERS_BACKFILL,
        WA_Activate,        TRUE,
        WA_Title,           "Amiga Development",
        WA_DragBar,         TRUE,
        WA_CloseGadget,     TRUE,
        WA_DepthGadget,     TRUE,
        WA_IDCMP,           IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|PALETTEIDCMP,
        WA_Zoom,            zoom,
        TAG_DONE))
    {
        return(w);
    }
    return(NULL);
}

struct BitMap *allocBitMap(struct Window *w)
{
    struct BitMap *bm;

    if (bm = AllocBitMap(w->WScreen->Width, w->WScreen->Height, w->RPort->BitMap->Depth, BMF_CLEAR, w->RPort->BitMap))
    {
        return(bm);
    }
    return(NULL);
}

struct Gadget *addGadgets(struct Window *w, struct VisualInfo *vi)
{
    struct NewGadget ng;
    struct Gadget *prev, *glist;

    prev = CreateContext(&glist);

    ng.ng_TextAttr = &ta;
    ng.ng_VisualInfo = vi;
    ng.ng_LeftEdge = w->BorderLeft + 64;
    ng.ng_TopEdge = w->BorderTop + 32;
    ng.ng_Width = 160;
    ng.ng_Height = 32;
    ng.ng_Flags = PLACETEXT_LEFT;
    ng.ng_GadgetText = "Color:";
    ng.ng_GadgetID = 1;
    ng.ng_UserData = NULL;

    prev = CreateGadget(PALETTE_KIND, prev, &ng,
        GTPA_Depth, w->RPort->BitMap->Depth,
        GTPA_Color, 2,
        GTPA_IndicatorWidth,    32,
        TAG_DONE);

    ng.ng_TopEdge += ng.ng_Height + 4;
    ng.ng_Height = 16;
    ng.ng_GadgetID = 2;
    ng.ng_GadgetText = "Label:";

    prev = CreateGadget(STRING_KIND, prev, &ng,
        GTST_String, label,
        TAG_DONE);

    ng.ng_TopEdge += ng.ng_Height + 4;
    ng.ng_GadgetID = 3;
    ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetText = "Quit";

    prev = CreateGadget(BUTTON_KIND, prev, &ng,
        TAG_DONE);

    ng.ng_TopEdge += ng.ng_Height + 4;
    ng.ng_GadgetID = 4;
    ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetText = "To Back";

    prev = CreateGadget(BUTTON_KIND, prev, &ng,
        TAG_DONE);

    ng.ng_TopEdge += ng.ng_Height + 4;
    ng.ng_GadgetID = 5;
    ng.ng_Flags = PLACETEXT_IN;
    ng.ng_GadgetText = "To Front";

    prev = CreateGadget(BUTTON_KIND, prev, &ng,
        TAG_DONE);

    AddGList(w, glist, 0, -1, NULL);
    RefreshGList(glist, w, NULL, -1);

    GT_RefreshWindow(w, NULL);

    return(glist);
}

int main(void)
{
    struct Screen *s;

    tf = OpenDiskFont(&ta);

    if (s = LockPubScreen(NULL))
    {
        struct Window *w;

        if (w = openWindow(s))
        {
            struct BitMap *bm;

            if (bm = allocBitMap(w))
            {
                struct VisualInfo *vi;

                drawBar(bm, w, 2, 1, label);

                BltBitMapRastPort(bm, 0, 0, w->RPort, w->BorderLeft, w->BorderTop, w->GZZWidth, w->GZZHeight, 0xc0);
                WaitBlit();

                if (vi = GetVisualInfoA(s, NULL))
                {
                    struct Gadget *glist;

                    if (glist = addGadgets(w, vi))
                    {
                        struct MsgPort *up = w->UserPort;
                        BOOL done = FALSE;
                        WORD color = 2, text = 1;

                        while (!done)
                        {
                            struct IntuiMessage *msg;

                            WaitPort(up);

                            while (msg = GT_GetIMsg(up))
                            {
                                ULONG class = msg->Class;
                                WORD code = msg->Code;
                                APTR iaddr = msg->IAddress;

                                GT_ReplyIMsg(msg);

                                if (class == IDCMP_REFRESHWINDOW)
                                {
                                    GT_BeginRefresh(w);
                                    BltBitMapRastPort(bm, 0, 0, w->RPort, w->BorderLeft, w->BorderTop, w->GZZWidth, 17, 0xc0);
                                    GT_EndRefresh(w, TRUE);
                                }
                                else if (class == IDCMP_CLOSEWINDOW)
                                {
                                    done = TRUE;
                                }
                                else if (class == IDCMP_GADGETUP)
                                {
                                    struct Gadget *gad = iaddr;

                                    if (gad->GadgetID == 1)
                                    {
                                        ULONG rgb[3];
                                        ULONG grey;

                                        GetRGB32(s->ViewPort.ColorMap, code, 1, rgb);

                                        rgb[0] >>= 24;
                                        rgb[1] >>= 24;
                                        rgb[2] >>= 24;

                                        grey = (rgb[0] + rgb[1] + rgb[2]) / 3;

                                        grey = RGB(grey);

                                        if (grey > 0x7fffffff)
                                        {
                                            drawBar(bm, w, color = code, text = 1, label);
                                        }
                                        else
                                        {
                                            drawBar(bm, w, color = code, text = 2, label);
                                        }
                                        BltBitMapRastPort(bm, 0, 0, w->RPort, w->BorderLeft, w->BorderTop, w->GZZWidth, 17, 0xc0);
                                    }
                                    else if (gad->GadgetID == 2)
                                    {
                                        label = ((struct StringInfo *)gad->SpecialInfo)->Buffer;
                                        drawBar(bm, w, color, text, label);
                                        BltBitMapRastPort(bm, 0, 0, w->RPort, w->BorderLeft, w->BorderTop, w->GZZWidth, 17, 0xc0);
                                    }
                                    else if (gad->GadgetID == 3)
                                    {
                                        done = TRUE;
                                    }
                                    else if (gad->GadgetID == 4)
                                    {
                                        WindowToBack(w);
                                    }
                                    else if (gad->GadgetID == 5)
                                    {
                                        WindowToFront(w);
                                    }
                                }
                            }
                        }
                        RemoveGList(w, glist, -1);
                        FreeGadgets(glist);
                    }
                    FreeVisualInfo(vi);
                }
                FreeBitMap(bm);
            }
            CloseWindow(w);
        }
        UnlockPubScreen(NULL, s);
    }
    if (tf)
    {
        CloseFont(tf);
    }
    return(0);
}
