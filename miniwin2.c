/*
 * Compiz Fusion Miniwin 2 plugin
 *
 * Copyright (C) 2007  Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Kristian Lyngstøl <kristian@bohemians.org>
 *
 */

#include <compiz-core.h>
#include "miniwin2_options.h"

typedef struct { 
    Window id;
    float scale;
    int x;
    int y;
} miniWindow;

typedef struct {
    PaintWindowProc paintWindow;
    Window scaledWin;
    int windowPrivateIndex;
} miniScreen;

static int displayPrivateIndex;
static int screenPrivateIndex;

#define MINI_SCREEN(s) miniScreen *ms = s->base.privates[screenPrivateIndex].ptr;
#define MINI_WINDOW(w) miniWindow *mw = w->base.privates[ms->windowPrivateIndex].ptr;

/* functions */
static void
miniwinScale (CompWindow *w)
{
    printf ("Miniwin2 triggered\n");
}

static void
miniwinUnscale (CompWindow *w)
{
    printf ("Miniwin2 triggered 2\n");
}


/* 
 * Initially triggered keybinding.
 * Fetch the window, fetch the resize, constrain it.
 *
 */
static Bool
miniwin2Trigger(CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    CompWindow *w = findWindowAtDisplay(d, d->activeWindow);
    if (!w)
	return TRUE;
    MINI_SCREEN (w->screen);
    MINI_WINDOW (w);
    if (mw->scale == 1.0f)
    {
	miniwinScale (w);
	mw->scale = 0.5f;
	mw->x = 0;
	mw->y = 0;
    }
    else
    {
	miniwinUnscale (w);
	mw->scale = 1.0f;
    }
    damageScreen (w->screen);
    return TRUE;
}

static Bool
mwPaintWindow (CompWindow *w,
	       const WindowPaintAttrib *attrib,
	       const CompTransform *transform,
	       Region region,
	       unsigned int mask)
{
    Bool status;
    CompScreen *s = w->screen;
    MINI_SCREEN (s);
    MINI_WINDOW (w);

    if (mw->scale != 1.0f)
    {
	CompTransform mTransform = *transform;
	int xOrigin, yOrigin;
	xOrigin = w->attrib.x + w->input.left;
	yOrigin = w->attrib.y + w->input.top;

	matrixTranslate (&mTransform, xOrigin, yOrigin, 0);
	matrixScale (&mTransform, mw->scale, mw->scale, 0);
	matrixTranslate (&mTransform,
			 (mw->x - w->attrib.x) / 
			 mw->scale - xOrigin,
			 (mw->y - w->attrib.y) /
			 mw->scale - yOrigin,
			 0);

	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	UNWRAP (ms, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, &mTransform, region, mask);
	WRAP (ms, s, paintWindow, mwPaintWindow);
    } else
    {
	UNWRAP (ms, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ms, s, paintWindow, mwPaintWindow);
    }

    return status;
}

/* Configuration, initialization, boring stuff. --------------------- */
static Bool
miniwin2InitScreen (CompPlugin *p,
		    CompScreen *s)
{
    miniScreen *ms;
    ms = malloc (sizeof (miniScreen));
    if (!ms)
	return FALSE; // fixme: error message.
    ms->windowPrivateIndex = allocateWindowPrivateIndex (s);
    s->base.privates[screenPrivateIndex].ptr = ms;
    WRAP (ms, s, paintWindow, mwPaintWindow); 
    return TRUE; 
}

static Bool
miniwin2InitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;
    screenPrivateIndex = allocateScreenPrivateIndex (d); 
    if (screenPrivateIndex < 0)
	return FALSE;
    miniwin2SetTriggerKeyInitiate (d, miniwin2Trigger);
    return TRUE;
}

static Bool
miniwin2Init (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;
    return TRUE;
}

static Bool
miniwin2InitWindow (CompPlugin *p,
		    CompWindow *w)
{
    MINI_SCREEN(w->screen);

    miniWindow *mw;
    mw = malloc (sizeof (miniWindow));
    if (!mw)
	return FALSE;

    mw->id = w->id;
    mw->scale = 1.0f;
    mw->x = 0;
    mw->y = 0;
    w->base.privates[ms->windowPrivateIndex].ptr = mw;
    return TRUE;
}

static CompBool
miniwin2InitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) miniwin2Init, /* InitCore */
	(InitPluginObjectProc) miniwin2InitDisplay,
	(InitPluginObjectProc) miniwin2InitScreen, 
	(InitPluginObjectProc) miniwin2InitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
miniwin2FiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	0, 
	0, 
	0 
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable miniwin2VTable = {
    "miniwin2",
    0,
    0,
    0,
    miniwin2InitObject,
    miniwin2FiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &miniwin2VTable;
}
