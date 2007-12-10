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
 * Author(s): 
 * Kristian Lyngstøl <kristian@bohemians.org>
 *
 * Description:
 *
 * This plugin visually resizes a window to allow otherwise obtrusive
 * windows to be visible in a monitor-fashion. Use case: Anything with
 * progress bars, notification programs, etc.
 *
 * Todo: 
 *  - Input mask
 *  - Animation
 *  - Floating resize (on scroll wheel for instance)
 *  - Correct damage handeling
 */

#include <compiz-core.h>
#include "shelf_options.h"

typedef struct { 
    Window id;
    float scale;
} shelfWindow;

typedef struct {
    PaintWindowProc paintWindow;
    DamageWindowRectProc damageWindowRect;
    int windowPrivateIndex;
} shelfScreen;

static int displayPrivateIndex;
static int screenPrivateIndex;

#define SHELF_SCREEN(s) \
    shelfScreen *ss = s->base.privates[screenPrivateIndex].ptr;
#define SHELF_WINDOW(w) \
    shelfWindow *sw = w->base.privates[ss->windowPrivateIndex].ptr;

/* Initially triggered keybinding.
 */
static Bool
shelfTrigger(CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    CompWindow *w = findWindowAtDisplay(d, d->activeWindow);
    if (!w)
	return TRUE;
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    if (sw->scale == 1.0f)
	sw->scale = 0.5f;
    else if (sw->scale == 0.5f)
	sw->scale = 0.25f;
    else 
	sw->scale = 1.00f;
    damageScreen (w->screen);
    return TRUE;
}

/* Fixme:
 * This should not cause total screen damage, but only adjust the rect
 * correctly.
 */
static Bool
shelfDamageWindowRect (CompWindow *w,
                       Bool       initial,
                       BoxPtr     rect)
{
    Bool status = FALSE;

    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);

    if (sw->scale != 1.0f)
    {
	damageScreen (w->screen);
	status = TRUE;
    }

    UNWRAP (ss, w->screen, damageWindowRect);
    status |= (*w->screen->damageWindowRect) (w, initial, rect);
    WRAP (ss, w->screen, damageWindowRect, shelfDamageWindowRect);

    return status;
}

/* Scale the window if it is supposed to be scaled. 
 */
static Bool
shelfPaintWindow (CompWindow *w,
	       const WindowPaintAttrib *attrib,
	       const CompTransform *transform,
	       Region region,
	       unsigned int mask)
{
    Bool status;
    CompScreen *s = w->screen;
    SHELF_SCREEN (s);
    SHELF_WINDOW (w);

    if (sw->scale != 1.0f)
    {
	CompTransform mTransform = *transform;
	int xOrigin, yOrigin;
	xOrigin = w->attrib.x - w->input.left;
	yOrigin = w->attrib.y - w->input.top;
	matrixTranslate (&mTransform, xOrigin, yOrigin,  0);
	matrixScale (&mTransform, sw->scale, sw->scale, 0);
	matrixTranslate (&mTransform, -xOrigin, -yOrigin,  0);
	mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, &mTransform, region, mask);
	WRAP (ss, s, paintWindow, shelfPaintWindow);
    }
    else
    {
	UNWRAP (ss, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (ss, s, paintWindow, shelfPaintWindow);
    }
    return status;
}

/* Configuration, initialization, boring stuff. --------------------- */
static void
shelfFiniScreen (CompPlugin *p,
		    CompScreen *s)
{
    SHELF_SCREEN (s);
    if (!ss)
	return ;
    UNWRAP (ss, s, paintWindow);
    UNWRAP (ss, s, damageWindowRect);
    if (ss->windowPrivateIndex)
	freeWindowPrivateIndex (s, ss->windowPrivateIndex);
    free (ss);
}
static Bool
shelfInitScreen (CompPlugin *p,
		    CompScreen *s)
{
    shelfScreen *ss;
    ss = malloc (sizeof (shelfScreen));
    if (!ss)
	return FALSE; // fixme: error message.
    ss->windowPrivateIndex = allocateWindowPrivateIndex (s);
    s->base.privates[screenPrivateIndex].ptr = ss;
    WRAP (ss, s, paintWindow, shelfPaintWindow); 
    WRAP (ss, s, damageWindowRect, shelfDamageWindowRect);
    return TRUE; 
}

static void
shelfFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    if (screenPrivateIndex >= 0)
        freeScreenPrivateIndex (d, screenPrivateIndex);
}

static Bool
shelfInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;
    screenPrivateIndex = allocateScreenPrivateIndex (d); 
    if (screenPrivateIndex < 0)
	return FALSE;
    shelfSetTriggerKeyInitiate (d, shelfTrigger);
    return TRUE;
}

static void
shelfFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
       freeDisplayPrivateIndex (displayPrivateIndex);
    return ;
}

static Bool
shelfInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;
    return TRUE;
}

static void
shelfFiniWindow (CompPlugin *p,
		    CompWindow *w)
{
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    if (sw)
	free (sw);
    return;
}

static Bool
shelfInitWindow (CompPlugin *p,
		    CompWindow *w)
{
    SHELF_SCREEN(w->screen);

    shelfWindow *sw;
    sw = malloc (sizeof (shelfWindow));
    if (!sw)
	return FALSE;

    sw->scale = 1.0f;
    w->base.privates[ss->windowPrivateIndex].ptr = sw;
    return TRUE;
}

static CompBool
shelfInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	    (InitPluginObjectProc) shelfInit, /* InitCore */
	    (InitPluginObjectProc) shelfInitDisplay,
	    (InitPluginObjectProc) shelfInitScreen, 
	    (InitPluginObjectProc) shelfInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
shelfFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) shelfFini, /* InitCore */
	(FiniPluginObjectProc) shelfFiniDisplay,
	(FiniPluginObjectProc) shelfFiniScreen, 
	(FiniPluginObjectProc) shelfFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable shelfVTable = {
    "shelf",
    0,
    0,
    0,
    shelfInitObject,
    shelfFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &shelfVTable;
}
