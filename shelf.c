/*
 * Compiz Fusion Shelf plugin
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
 * TODO: 
 *  - Mem leaks (no malloc ()'s are ever freed)
 *  - Unwrap on unload
 *  - Input mask
 *  - Animation
 *  - Floating resize (on scroll wheel for instance)
 *  - Misc
 */

#include <compiz-core.h>
#include "shelf_options.h"

typedef struct { 
    Window id;
    float scale;
} miniWindow;

typedef struct {
    PaintWindowProc paintWindow;
    DamageWindowRectProc damageWindowRect;
    int windowPrivateIndex;
} miniScreen;

static int displayPrivateIndex;
static int screenPrivateIndex;

#define MINI_SCREEN(s) \
    miniScreen *ms = s->base.privates[screenPrivateIndex].ptr;
#define MINI_WINDOW(w) \
    miniWindow *mw = w->base.privates[ms->windowPrivateIndex].ptr;

/* Initially triggered keybinding.
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
	mw->scale = 0.5f;
    else if (mw->scale == 0.5f)
	mw->scale = 0.25f;
    else 
	mw->scale = 1.00f;
    damageScreen (w->screen);
    return TRUE;
}

/* Fixme:
 * This should not cause total screen damage, but only adjust the rect
 * correctly.
 */
static Bool
miniDamageWindowRect (CompWindow *w,
                       Bool       initial,
                       BoxPtr     rect)
{
    Bool status = FALSE;

    MINI_SCREEN (w->screen);
    MINI_WINDOW (w);

    if (mw->scale != 1.0f)
    {
	damageScreen (w->screen);
	status = TRUE;
    }

    UNWRAP (ms, w->screen, damageWindowRect);
    status |= (*w->screen->damageWindowRect) (w, initial, rect);
    WRAP (ms, w->screen, damageWindowRect, miniDamageWindowRect);

    return status;
}

/* Scale the window if it is supposed to be scaled. 
 */
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
	xOrigin = w->attrib.x - w->input.left;
	yOrigin = w->attrib.y - w->input.top;
	matrixTranslate (&mTransform, xOrigin, yOrigin,  0);
	matrixScale (&mTransform, mw->scale, mw->scale, 0);
	matrixTranslate (&mTransform, -xOrigin, -yOrigin,  0);
	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	UNWRAP (ms, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, &mTransform, region, mask);
	WRAP (ms, s, paintWindow, mwPaintWindow);
    }
    else
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
    WRAP (ms, s, damageWindowRect, miniDamageWindowRect);
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
    shelfSetTriggerKeyInitiate (d, miniwin2Trigger);
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
    "shelf",
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
