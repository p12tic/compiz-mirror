/*
 * Compiz Fusion Maximumize plugin
 *
 * Copyright (c) 2007 Kristian Lyngstøl <kristian@bohemians.org>
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
 */

/* 
 * Maximumize resizes a window so it fills as much of the free space as
 * possible without overlapping with other windows.
 *
 * Todo:
 *  - The algorithm moves one pixel at a time, it should be smarter.
 */

#include <compiz-core.h>
#include "maximumize_options.h"

/* Generates a region containing free space (here the
 * active window counts as free space). The region argument
 * is the start-region (ie: the output dev).
 * Logic borrowed from opacify (courtesy of myself).
 */
static Region
maximumizeEmptyRegion (CompWindow *window, Region region)
{
    CompScreen *s = window->screen;
    CompWindow *w;
    Region newregion, tmpregion, eMpty;
    XRectangle tmprect;

    newregion = XCreateRegion ();
    if (!newregion)
	return NULL;
    tmpregion = XCreateRegion ();
    if (!tmpregion)
    {
	XDestroyRegion (newregion);
	return NULL;
    }
    eMpty = XCreateRegion ();
    if (!eMpty)
    {
	XDestroyRegion (newregion);
	XDestroyRegion (tmpregion);
	return NULL;
    }
    XSubtractRegion (region, &emptyRegion, newregion);
    for (w = s->windows; w; w = w->next)
    {
        if (w->id == w->screen->display->activeWindow)
            continue;
        if (w->invisible || w->hidden || w->minimized)
            continue;
	if (w->wmType & CompWindowTypeDesktopMask )
	    continue;
	tmprect.x = w->serverX - w->input.left;
	tmprect.y = w->serverY - w->input.top;
	tmprect.width = w->serverWidth + w->input.right + w->input.left;
	tmprect.height = w->serverHeight + w->input.top + w->input.bottom;
	XUnionRectWithRegion (&tmprect, tmpregion, tmpregion);
	XSubtractRegion (newregion, tmpregion, newregion);
	XSubtractRegion (eMpty, eMpty, tmpregion);
    }
    XDestroyRegion (tmpregion);
    XDestroyRegion (eMpty);
    
    return newregion;
}

/* Returns true if box a has a larger area than box b.
 */
static Bool
maximumizeBoxCompare (BOX a, BOX b)
{
    if (((a.x2 - a.x1) * (a.y2 - a.y1)) > 
	((b.x2 - b.x1) * (b.y2 - b.y1)))
	return TRUE;
    return FALSE;
}

/* Extends the given box for Window w to fit as much space in region r.
 * If XFirst is true, it will first expand in the X direction,
 * then Y. This is because it gives different results. 
 * PS: Decorations are icky.
 */
static BOX
maximumizeExtendBox (BOX tmp, CompWindow *w, Region r, Bool Xfirst)
{
    short int counter = 0;

#define CHECKREC \
	XRectInRegion (r, tmp.x1 - w->input.left, tmp.y1 - w->input.top, \
		       tmp.x2 - tmp.x1 + w->input.left + w->input.right, \
		       tmp.y2 - tmp.y1 + w->input.top + w->input.bottom) \
	    == RectangleIn
    Bool touch = FALSE;
    while (counter < 1)
    {
	if ((Xfirst && counter == 0) || (!Xfirst && counter == 1))
	{
	    while (CHECKREC)
	    {
		tmp.x1--;
	        touch = TRUE;
	    }
	    if (touch)
		tmp.x1++;
	    touch = FALSE;
	    while (CHECKREC)
	    {
		    tmp.x2++;
		    touch = TRUE;
	    }
	    if (touch)
		tmp.x2--;
	    touch = FALSE;
	    counter++;
	}
	if ((Xfirst && counter == 1) || (!Xfirst && counter == 0))
	{
	    while (CHECKREC)
	    {
		tmp.y2++;
		touch = TRUE;
	    }
	    if (touch)
		tmp.y2--;
	    touch = FALSE;
	    while (CHECKREC)
	    {
		tmp.y1--;
		touch = TRUE;
	    }
	    if (touch)
		tmp.y1++;
	    touch = FALSE;
	    counter++;
	}
    }
#undef CHECKREC
    return tmp;
}

/* Create a box for resizing in the given region
 */
static BOX
maximumizeFindRect (CompWindow *w, Region r)
{
    BOX windowBox, ansa, ansb;
    windowBox.x1 = w->serverX;
    windowBox.x2 = w->serverX + w->serverWidth ;
    windowBox.y1 = w->serverY;
    windowBox.y2 = w->serverY + w->serverHeight;
    ansa = maximumizeExtendBox (windowBox, w, r, TRUE);
    ansb = maximumizeExtendBox (windowBox, w, r, FALSE);
    if (maximumizeBoxCompare(ansa,ansb))
	return ansa;
    return ansb;

}

/* Calls out to compute the resize */
static Bool
maximumizeComputeResize(CompWindow *w,
			int *width,
			int *height,
			int *x,
			int *y)
{
    CompOutput *o = &w->screen->outputDev[outputDeviceForWindow (w)];
    Region r = maximumizeEmptyRegion (w, &o->region);
    if (!r)
	return FALSE;
    BOX b = maximumizeFindRect (w, r);
    XDestroyRegion (r);

    *width = b.x2 - b.x1; 
    *height = b.y2 - b.y1;
    *x = b.x1;
    *y = b.y1;
    return TRUE;
}

/* 
 * Initially triggered keybinding.
 * Fetch the window, fetch the resize, constrain it.
 *
 */
static Bool
maximumizeTrigger(CompDisplay     *d,
		 CompAction      *action,
		 CompActionState state,
		 CompOption      *option,
		 int             nOption)
{
    CompWindow *w;
    int width, height, x, y;
    XWindowChanges xwc;
    w = findWindowAtDisplay (d, d->activeWindow);
    if (! maximumizeComputeResize (w, &width, &height, &x, &y))
	return TRUE;
    constrainNewWindowSize (w, width, height, &width, &height);
    xwc.x = x;
    xwc.y = y;
    xwc.width = width;
    xwc.height = height;
    sendSyncRequest (w);
    configureXWindow (w, (unsigned int) CWWidth | CWHeight | CWX | CWY,
		      &xwc);
    return TRUE;
}

/* Configuration, initialization, boring stuff. ----------------------- */
static Bool
maximumizeInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    maximumizeSetTriggerKeyInitiate (d, maximumizeTrigger);
    return TRUE;
}

static CompBool
maximumizeInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) maximumizeInitDisplay,
	0, 
	0 
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
maximumizeFiniObject (CompPlugin *p,
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

CompPluginVTable maximumizeVTable = {
    "maximumize",
    0,
    0,
    0,
    maximumizeInitObject,
    maximumizeFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &maximumizeVTable;
}
