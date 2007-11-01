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

#include <compiz-core.h>
#include "maximumize_options.h"

static Region
maximumizeEmptyRegion (CompWindow *window, Region region)
{
    CompScreen *s = window->screen;
    CompWindow *w;
    Region newregion;
    newregion = XCreateRegion ();
    if (!newregion)
	return 0;
    int i;
    XSubtractRegion (region, &emptyRegion, newregion);

    for (w = s->windows; w; w = w->next)
    {
        if (w->id == w->screen->display->activeWindow)
            continue;
        if (w->invisible || w->hidden || w->minimized)
            continue;

	XSubtractRegion (newregion, w->region, newregion);
    for (i = 0; i <= newregion->numRects; i++)
	printf ("x1: %hd x2: %hd y1: %hd y2: %hd\n",
		newregion->rects[i].x1,
		newregion->rects[i].x2,
		newregion->rects[i].y1,
		newregion->rects[i].y2);
    }

    return newregion;
}

static Bool
maximumizeBoxCompare (BOX a, BOX b)
{
    if (((a.x2 - a.x1) * (a.y2 - a.y1)) > 
	((b.x2 - b.x1) * (b.y2 - b.y1)))
	return TRUE;
    return FALSE;
}

static Bool
maximumizeBoxInBox (BOX a, BOX b)
{
    if (a.x1 >= b.x1 && a.x1 < b.x2 &&
	a.x2 > b.x1 && a.x2 <= b.x2 &&
	a.y1 >= b.y1 && a.y1 < b.y2 &&
	a.y2 > b.y1 && a.y2 <= b.y2)
	return TRUE;
    return FALSE;
}
static BOX
maximumizeFindRect (CompWindow *w, Region r)
{
    int i;
    int current = -1;
    BOX windowBox;
    windowBox.x1 = w->serverX;
    windowBox.x2 = w->serverX + w->serverWidth;
    windowBox.y1 = w->serverY;
    windowBox.y2 = w->serverY + w->serverHeight;

    for (i = 0; i <= r->numRects; i++)
    {
	printf ("box!\n");
	printf ("x1: %d x2: %d y1: %d y2: %d\n",
		r->rects[i].x1,
		r->rects[i].x2,
		r->rects[i].y1,
		r->rects[i].y2);
	if (!maximumizeBoxInBox (windowBox, r->rects[i]))
	    continue;
	if (current == -1)
	    current = i;
	else if (maximumizeBoxCompare (r->rects[i], r->rects[current]))
	    current = i;
    }
    printf ("current: %d\n",current);
    if (current == -1) 
	return windowBox;
    return r->rects[current];

}
static void
maximumizeComputeResize(CompWindow *w,
			int *width,
			int *height,
			int *x,
			int *y)
{
    CompOutput *o = &w->screen->outputDev[outputDeviceForWindow (w)];

    Region r = maximumizeEmptyRegion (w, &o->region);
    BOX b = maximumizeFindRect (w, r);

    *width = b.x2 - b.x1; 
    *height = b.y2 - b.y1;
    *x = b.x1;
    *y = b.y1;
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
    maximumizeComputeResize (w, &width, &height, &x, &y);
 //  printf ("width: %d heigth: %d x: %d y: %d\n", width, height, x, y); 
    constrainNewWindowSize (w, width, height, &width, &height);
    xwc.x = x;
    xwc.y = y;
    xwc.width = width;
    xwc.height = height;
    sendSyncRequest (w);
   printf ("width: %d heigth: %d x: %d y: %d\n", width, height, x, y); 
    configureXWindow (w, (unsigned int) CWWidth | CWHeight, &xwc);
    printf ("wooo\n");
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
