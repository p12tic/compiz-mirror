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
 *  - Check for XShape events
 *  - Handle input in a sane way
 *  - Correct damage handeling
 *  - Mouse-over?
 */

#include <compiz-core.h>
#include <X11/extensions/shape.h>
#include <math.h>
#include "shelf_options.h"

typedef struct { 
    float scale;
    float targetScale;
    float steps;
    Window ipw;
} shelfWindow;

typedef struct {
    PaintWindowProc paintWindow;
    DamageWindowRectProc damageWindowRect;
    PreparePaintScreenProc preparePaintScreen;
    int windowPrivateIndex;
} shelfScreen;

static int displayPrivateIndex;
static int screenPrivateIndex;

#define SHELF_SCREEN(s) \
    shelfScreen *ss = s->base.privates[screenPrivateIndex].ptr;
#define SHELF_WINDOW(w) \
    shelfWindow *sw = w->base.privates[ss->windowPrivateIndex].ptr;

#define SHELF_MIN_SIZE 50.0f // Minimum pixelsize a window can be scaled to

/* Shape the input of the window when scaled */
static void
shelfShapeInput (CompWindow *w)
{
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);

    XRectangle rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = w->serverWidth * sw->targetScale;
    rect.height = w->serverHeight * sw->targetScale;

    XShapeSelectInput (w->screen->display->display, w->id, NoEventMask);
    XShapeCombineRectangles  (w->screen->display->display, w->id, 
			      ShapeInput, 0, 0, &rect, 1,  ShapeSet, 0);
    if (w->frame)
	XShapeCombineRectangles  (w->screen->display->display, w->frame, 
				  ShapeInput, 0, 0, &rect, 1,  ShapeSet, 0);
    XShapeSelectInput (w->screen->display->display, w->id, ShapeNotify);
}

static void
shelfPreparePaintScreen (CompScreen *s,
			 int	    msSinceLastPaint)
{
    CompWindow *w;
    float steps;
    SHELF_SCREEN (s);

    steps =  (float)msSinceLastPaint / (float)shelfGetAnimtime(s->display);

    if (steps < 0.005)
	steps = 0.005;

    for (w = s->windows; w; w = w->next)
	((shelfWindow *)w->base.privates[ss->windowPrivateIndex].ptr)->steps
	    = steps;
    
    UNWRAP (ss, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (ss, s, preparePaintScreen, shelfPreparePaintScreen);
}

/* Adjust size and location of the input prevention window
 */
static void 
shelfAdjustIPW (CompWindow *w)
{
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    XWindowChanges xwc;

    if (!sw->ipw)
	return;
    if (sw->targetScale == 1.0f)
    {
	XDestroyWindow (w->screen->display->display, sw->ipw);
	sw->ipw = None;
	return;
    }
    xwc.x = w->serverX - w->input.left;
    xwc.y = w->serverY - w->input.top;
    xwc.width = (int) ((float) (w->serverWidth + w->input.left +
				w->input.right) * sw->targetScale);
    xwc.height = (int) ((float) (w->serverHeight + w->input.top +
				 w->input.bottom) * sw->targetScale);

    xwc.stack_mode = Above;
    xwc.sibling = w->id;
    XConfigureWindow (w->screen->display->display, sw->ipw,
		      CWSibling | CWStackMode | CWX | CWY |
		      CWWidth | CWHeight, &xwc);
    XMapWindow (w->screen->display->display, sw->ipw);
    return;

}

/* Create an input prevention window */
static void
shelfMkIPW (CompWindow *w)
{
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);

    if (sw->ipw)
	return;
    XSetWindowAttributes attrib;
    attrib.override_redirect = TRUE;
    sw->ipw = XCreateWindow (w->screen->display->display, w->screen->root,
			     w->serverX - w->input.left, w->serverY -
			     w->input.top, w->serverWidth + w->input.left +
			     w->input.right, w->serverHeight + w->input.top
			     + w->input.bottom, 0, CopyFromParent,
			     InputOnly, CopyFromParent, CWOverrideRedirect,
			     &attrib);
}

/* Sets the scale level and adjust the shape */
static void
shelfScaleWindow (CompWindow *w, float scale)
{
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);

    if (w->wmType & (CompWindowTypeDesktopMask | CompWindowTypeDockMask))
	return;
    sw->targetScale = scale;
    if (sw->targetScale > 1.0f)
	sw->targetScale = 1.0f;
    else if ((float)w->width * sw->targetScale < SHELF_MIN_SIZE )
	sw->targetScale = SHELF_MIN_SIZE / (float)w->width;
    shelfShapeInput (w);
    shelfMkIPW (w);
    shelfAdjustIPW (w);
    damageScreen (w->screen);
}

/* Binding for toggle mode. 
 * Toggles through three preset scale levels, 
 * currently hard coded to 1.0f (no scale), 0.5f and 0.25f.
 */
static Bool
shelfTrigger (CompDisplay     *d,
	      CompAction      *action,
	      CompActionState state,
	      CompOption      *option,
	      int             nOption)
{
    CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
    if (!w)
	return TRUE;
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    if (sw->targetScale > 0.5f)
	shelfScaleWindow (w, 0.5f);
    else if (sw->targetScale <= 0.5f && sw->targetScale > 0.25)
	shelfScaleWindow (w, 0.25f);
    else 
	shelfScaleWindow (w, 1.0f);
    return TRUE;
}

/* shelfInc and shelfDec are matcing functions and bindings;
 * They increase and decrease the scale factor by 'interval'.
 */
static Bool
shelfInc (CompDisplay     *d,
	  CompAction      *action,
	  CompActionState state,
	  CompOption      *option,
	  int             nOption)
{
    CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
    if (!w)
	return TRUE;
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    shelfScaleWindow (w, sw->targetScale / shelfGetInterval (d));
    return TRUE;
}

static Bool
shelfDec (CompDisplay     *d,
	  CompAction      *action,
	  CompActionState state,
	  CompOption      *option,
	  int             nOption)
{
    CompWindow *w = findWindowAtDisplay (d, d->activeWindow);
    if (!w)
	return TRUE;
    SHELF_SCREEN (w->screen);
    SHELF_WINDOW (w);
    shelfScaleWindow (w, sw->targetScale * shelfGetInterval (d));
    return TRUE;
}

/* The window was damaged, adjust the damage to fit the actual area we
 * care about.
 *
 * FIXME:
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
 * Translate into place.
 *
 * FIXME: Merge the two translations.
 */
static Bool
shelfPaintWindow (CompWindow		    *w,
		  const WindowPaintAttrib   *attrib,
		  const CompTransform	    *transform,
		  Region		    region,
		  unsigned int		    mask)
{
    Bool status;
    CompScreen *s = w->screen;
    SHELF_SCREEN (s);
    SHELF_WINDOW (w);

    if (sw->targetScale != sw->scale && sw->steps)
    {
	sw->scale += (float) sw->steps * (sw->targetScale - sw->scale);
	if (fabsf(sw->targetScale - sw->scale) < 0.005)
	    sw->scale = sw->targetScale;
    }
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
	if (sw->scale != sw->targetScale)
	    addWindowDamage (w);	    
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
    UNWRAP (ss, s, preparePaintScreen);
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
    WRAP (ss, s, preparePaintScreen, shelfPreparePaintScreen);
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
    if (!d->shapeExtension)
    {
	printf ("shelf: No Shape extension found. Shelfing not possible.\n");
	freeScreenPrivateIndex (d, screenPrivateIndex);
	return FALSE;
    }
    shelfSetTriggerKeyInitiate (d, shelfTrigger);
    shelfSetIncButtonInitiate (d, shelfInc);
    shelfSetDecButtonInitiate (d, shelfDec);
    return TRUE;
}

static void
shelfFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
       freeDisplayPrivateIndex (displayPrivateIndex);
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
    sw->targetScale = 1.0f;
    sw->ipw = None;
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
