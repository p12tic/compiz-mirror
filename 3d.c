/*
 *
 * Compiz 3d plugin
 *
 * 3d.c
 *
 * Copyright : (C) 2006 by Roi Cohen
 * E-mail    : roico12@gmail.com
 *
 * Modified by : Dennis Kasprzyk <onestone@opencompositing.org>
 *               Danny Baumann <mainiac@opencompositing.org>
 *               Robert Carr <racarr@beryl-project.org>
 *               Diogo Ferreira <diogo@underdev.org>
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

/**
TODO:
        1. Add 3d shadows / projections.
        3. Add an option to select z-order of windows not only by viewports but also by screens.
        4. Fix 3d for inside cube and planed / unfolded cube.
        5. Find a better solution for blur cache + 3d.
        6. Fix bugs with 3d + animations / wobbly.
        	- Wobbly will draw the window twice if its in 2 different viewports.
        	- Many Bugs with animations, some are solvable by changing the load order, but it will result with clipping when animations are done.
		7. *High priority* Fix windows in 3D appearing a second time to the left of their real position.
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-cube.h>
#include "3d_options.h"

#define PI 3.14159265359f

static int displayPrivateIndex;
static int cubeDisplayPrivateIndex = -1;

typedef struct _tdDisplay
{
    int screenPrivateIndex;
} tdDisplay;

typedef struct _tdWindow
{
    Bool ftb;
    Bool is3D;

    float depth;

    CompWindow *next;
    CompWindow *prev;
} tdWindow;

typedef struct _tdScreen
{
    int windowPrivateIndex;

    Bool tdWindowExists;
    Bool active;
    Bool wasActive;

    PreparePaintScreenProc    preparePaintScreen;
    PaintOutputProc	      paintOutput;
    CubePostPaintViewportProc postPaintViewport;
    DonePaintScreenProc	      donePaintScreen;
    InitWindowWalkerProc      initWindowWalker;
    ApplyScreenTransformProc  applyScreenTransform;
    PaintWindowProc           paintWindow;

    CompWindow *first;
    CompWindow *last;

    Bool  test;
    float currentScale;

    float basicScale;
    float maxDepth;
} tdScreen;

#define GET_TD_DISPLAY(d)       \
    ((tdDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define TD_DISPLAY(d)   \
    tdDisplay *tdd = GET_TD_DISPLAY (d)

#define GET_TD_SCREEN(s, tdd)   \
    ((tdScreen *) (s)->base.privates[(tdd)->screenPrivateIndex].ptr)

#define TD_SCREEN(s)    \
    tdScreen *tds = GET_TD_SCREEN (s, GET_TD_DISPLAY (s->display))

#define GET_TD_WINDOW(w, tds)                                     \
    ((tdWindow *) (w)->base.privates[(tds)->windowPrivateIndex].ptr)

#define TD_WINDOW(w)    \
    tdWindow *tdw = GET_TD_WINDOW  (w,                     \
		    GET_TD_SCREEN  (w->screen,             \
		    GET_TD_DISPLAY (w->screen->display)))

static Bool
windowIs3D (CompWindow *w)
{
    if (w->attrib.override_redirect)
	return FALSE;

    if (!(w->shaded || w->attrib.map_state == IsViewable))
	return FALSE;

    if (w->state & (CompWindowStateSkipPagerMask |
		    CompWindowStateSkipTaskbarMask))
	return FALSE;
	
    if (!matchEval (tdGetWindowMatch (w->screen), w))
	return FALSE;

    return TRUE;
}

static void
tdPreparePaintScreen (CompScreen *s,
		      int        msSinceLastPaint)
{
    CompWindow *w;
    float      amount;

    TD_SCREEN (s);
    CUBE_SCREEN (s);

    tds->active = (cs->rotationState != RotationNone) &&
	          !(tdGetManualOnly(s) &&
		    (cs->rotationState != RotationManual));

    amount = ((float)msSinceLastPaint * tdGetSpeed (s) / 1000.0);
    if (tds->active)
    {
	float maxDiv = 0.1; // should be a option;
	float minScale = 0.5; // should be a option;

	tds->maxDepth = 0;
	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);
	    tdw->is3D = FALSE;
	    tdw->depth = 0;

	    if (!windowIs3D (w))
		continue;

	    tdw->is3D = TRUE;
	    tds->maxDepth++;
	    tdw->depth = tds->maxDepth;
	    tds->tdWindowExists = TRUE;
	}

	minScale =  MAX (minScale, 1.0 - (tds->maxDepth * maxDiv));
	if (cs->invert == 1)
	    tds->basicScale = MAX (minScale, tds->basicScale - amount);
	else
 	    tds->basicScale = MIN (2 - minScale, tds->basicScale + amount);
    }
    else
    {
	if (cs->invert == 1)
	    tds->basicScale = MIN (1.0, tds->basicScale + amount);
	else
 	    tds->basicScale = MAX (1.0, tds->basicScale - amount);
    }

    UNWRAP (tds, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (tds, s, preparePaintScreen, tdPreparePaintScreen);

    cs->paintAllViewports |= tds->active | tds->tdWindowExists;
}

/* forward declaration */
static Bool tdPaintWindow (CompWindow              *w,
			   const WindowPaintAttrib *attrib,
			   const CompTransform     *transform,
			   Region                  region,
			   unsigned int            mask);

static Bool
tdPaintWindowWithDepth (CompWindow              *w,
		     	const WindowPaintAttrib *attrib,
			const CompTransform     *transform,
			Region                  region,
			unsigned int            mask)
{
    Bool wasCulled;
    Bool status;
    int wx, wy, wx2, wy2, ww, wh;
    int bevel;
    float wwidth;
    CompTransform wTransform = *transform;
    CompScreen *s = w->screen;

    TD_SCREEN (s);

    wasCulled = glIsEnabled (GL_CULL_FACE);

    wx = MAX (0, MIN (s->width, w->attrib.x - w->input.left));
    wx2 = MAX (0, MIN (s->width, w->attrib.x + w->width + w->input.right));

    wy = MAX (0, MIN (s->height, w->attrib.y - w->input.top));
    wy2 = MAX (0, MIN (s->height, w->attrib.y + w->height + w->input.bottom));

    ww = wx2 - wx;
    wh = wy2 - wy;

    wwidth = -(tdGetWidth (s)) / 30;
    bevel = tdGetBevel (s);

    if (ww && wh && !(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
    {
	TD_WINDOW (w);

	if (!tdw->ftb)
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_FRONT);

	    matrixTranslate (&wTransform, 0.0f, 0.0f, wwidth);

    	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
	    WRAP (tds, s, paintWindow, tdPaintWindow);

	    matrixTranslate (&wTransform, 0.0f, 0.0f, -wwidth);
	}
	else
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_BACK);

	    UNWRAP (tds, s, paintWindow);
    	    status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
    	    WRAP (tds, s, paintWindow, tdPaintWindow);
	}

	/* Paint window depth. */

	glPushMatrix ();
	glLoadMatrixf (wTransform.m);

	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);

	glBegin (GL_QUADS);
	glColor4f (0.90f, 0.90f, 0.90f, w->paint.opacity / OPAQUE);

#define DOBEVEL(corner) (tdGetBevel##corner (s) ? bevel : 0)

	/* Top */
	glVertex3f (wx + DOBEVEL (Topleft), wy, 0);
	glVertex3f (wx + ww - DOBEVEL (Topright), wy, 0);
	glVertex3f (wx + ww - DOBEVEL (Topright), wy, wwidth);
	glVertex3f (wx + DOBEVEL (Topleft), wy, wwidth);

	/* Bottom */
	glVertex3f (wx + DOBEVEL (Bottomleft), wy + wh, 0);
	glVertex3f (wx + ww - DOBEVEL (Bottomright), wy + wh, 0);
	glVertex3f (wx + ww - DOBEVEL (Bottomright), wy + wh, wwidth);
	glVertex3f (wx + DOBEVEL (Bottomleft), wy + wh, wwidth);

	glColor4f (0.70f, 0.70f, 0.70f,  w->paint.opacity / OPAQUE);

	/* Left */
	if (w->attrib.x >= s->workArea.x)
	{
	    glVertex3f (wx, wy + DOBEVEL (Topleft), 0);
	    glVertex3f (wx, wy + wh - DOBEVEL (Bottomleft), 0);
	    glVertex3f (wx, wy + wh - DOBEVEL (Bottomleft), wwidth);
	    glVertex3f (wx, wy + DOBEVEL (Topleft), wwidth);
	}

	/* Right */
	if ((w->attrib.x + w->width + w->input.left + w->input.right) <=
	    s->workArea.width)
	{
	    glVertex3f (wx + ww, wy + DOBEVEL (Topright), 0);
	    glVertex3f (wx + ww, wy + wh - DOBEVEL (Bottomright), 0);
	    glVertex3f (wx + ww, wy + wh - DOBEVEL (Bottomright), wwidth);
	    glVertex3f (wx + ww, wy + DOBEVEL (Topright), wwidth);
	}

	glColor4f (0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);

	if (w->attrib.x >= s->workArea.x)
	{
	    /* Top left bevel */
	    if (tdGetBevelTopleft (s))
	    {
		glVertex3f (wx, wy + bevel, wwidth);
		glVertex3f (wx, wy + bevel, 0);
		glVertex3f (wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);
		glVertex3f (wx + bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, wwidth);

		glColor4f (1.0f, 1.0f, 1.0f,  w->paint.opacity / OPAQUE);

		glVertex3f (wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);
		glVertex3f (wx + bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, wwidth);
		glVertex3f (wx + bevel, wy, wwidth);
		glVertex3f (wx + bevel, wy, 0);

		glColor4f (0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);
	    }

	    /* Bottom left bevel */
	    if (tdGetBevelBottomleft (s))
	    {
		glVertex3f (wx, wy + wh - bevel, 0);
		glVertex3f (wx, wy + wh - bevel, wwidth);
		glVertex3f (wx + bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, wwidth);
		glVertex3f (wx + bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, 0);

		glColor4f (1.0f, 1.0f, 1.0f,  w->paint.opacity / OPAQUE);

		glVertex3f (wx + bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, wwidth);
		glVertex3f (wx + bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, 0);
		glVertex3f (wx + bevel, wy + wh, 0);
		glVertex3f (wx + bevel, wy + wh, wwidth);
	    }
	}

	glColor4f (0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);

	if ((w->attrib.x + w->width + w->input.left + w->input.right) <=
	    s->workArea.width)
	{
	    /* Bottom right bevel */
	    if (tdGetBevelBottomright (s))
	    {
		glVertex3f (wx + ww - bevel, wy + wh, 0);
		glVertex3f (wx + ww - bevel, wy + wh, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, 0);

		glColor4f (1.0f, 1.0f, 1.0f,  w->paint.opacity / OPAQUE);

		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + wh - bevel + bevel / 1.2f, 0);
		glVertex3f (wx + ww, wy + wh - bevel, 0);
		glVertex3f (wx + ww, wy + wh - bevel, wwidth);

		glColor4f (0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);
	    }
    
	    /* Top right bevel */
	    if (tdGetBevelTopright (s))
	    {
		glVertex3f (wx + ww - bevel, wy, 0);
		glVertex3f (wx + ww - bevel, wy, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, 0);

		glColor4f (1.0f, 1.0f, 1.0f,  w->paint.opacity / OPAQUE);

		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, wwidth);
		glVertex3f (wx + ww - bevel / 2.0f,
			    wy + bevel - bevel / 1.2f, 0);
		glVertex3f (wx + ww, wy + bevel, 0);
		glVertex3f (wx + ww, wy + bevel, wwidth);
    	    }
	}

	glEnd ();
	glPopMatrix ();

	if (tdw->ftb)
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_FRONT);

	    matrixTranslate (&wTransform, 0.0f, 0.0f, wwidth);

    	    UNWRAP (tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
	    WRAP(tds, s, paintWindow, tdPaintWindow);

	    matrixTranslate(&wTransform, 0.0f, 0.0f, -wwidth);
	}
	else
	{
	    glEnable (GL_CULL_FACE);
	    glCullFace (GL_BACK);

	    UNWRAP(tds, s, paintWindow);
	    status = (*s->paintWindow) (w, attrib, &wTransform, region, mask);
	    WRAP (tds, s, paintWindow, tdPaintWindow);
	}
    }
    else
    {
	UNWRAP(tds, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (tds, s, paintWindow, tdPaintWindow);
    }

    glCullFace (GL_BACK);

    if (!wasCulled)
	glDisable (GL_CULL_FACE);

    return status;
}

static Bool
tdPaintWindow (CompWindow              *w,
	       const WindowPaintAttrib *attrib,
	       const CompTransform     *transform,
	       Region                  region,
	       unsigned int            mask)
{
    Bool       status;
    CompScreen *s = w->screen;

    TD_SCREEN (s);
    TD_WINDOW (w);

    if (tdw->depth != 0.0f && !tds->test && tds->basicScale != 1.0)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (tds->test)
    {
	if (tdGetWidth (s) && (tdw->depth != 0.0f))
	{
	    status = tdPaintWindowWithDepth (w, attrib, transform,
					     region, mask);
	}
	else
	{
	    Bool wasCulled;

    	    wasCulled = glIsEnabled (GL_CULL_FACE);
    	    glDisable (GL_CULL_FACE);

    	    UNWRAP (tds, s, paintWindow);
    	    status = (*s->paintWindow) (w, attrib, transform, region, mask);
    	    WRAP (tds, s, paintWindow, tdPaintWindow);

    	    if (wasCulled)
    		glEnable (GL_CULL_FACE);
	}
    }
    else
    {
	UNWRAP (tds, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP(tds, s, paintWindow, tdPaintWindow);
    }

    return status;
}

static void
tdApplyScreenTransform (CompScreen		*s,
			const ScreenPaintAttrib *sAttrib,
			CompOutput		*output,
			CompTransform	        *transform)
{
    TD_SCREEN (s);

    UNWRAP (tds, s, applyScreenTransform);
    (*s->applyScreenTransform) (s, sAttrib, output, transform);
    WRAP (tds, s, applyScreenTransform, tdApplyScreenTransform);

    matrixScale (transform,
		 tds->currentScale, tds->currentScale, tds->currentScale);
}

static void
tdAddWindow (CompWindow *w)
{
    TD_SCREEN (w->screen);
    TD_WINDOW (w);

    if (!tds->first)
    {
	tds->first = tds->last = w;
	return;
    }

    GET_TD_WINDOW (tds->last, tds)->next = w;
    tdw->prev = tds->last;
    tds->last = w;
}

static void
tdPostPaintViewport (CompScreen              *s,
	   	     const ScreenPaintAttrib *sAttrib,
   		     const CompTransform     *transform,
		     CompOutput              *output,
		     Region                  region)
{
    CompWindow* w;
    CompWindow* firstFTB = NULL;

    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, cs, postPaintViewport);
    (*cs->postPaintViewport) (s, sAttrib, transform, output, region);
    WRAP (tds, cs, postPaintViewport, tdPostPaintViewport);

    if (tds->active || tds->tdWindowExists)
    {
	float vPoints[3][3] = {{ -0.5, 0.0, (cs->invert * cs->distance)},
	                       { 0.0, 0.5, (cs->invert * cs->distance)},
		               { 0.0, 0.0, (cs->invert * cs->distance)}};

	/* all non 3d windows first */
	tds->first = NULL;
	tds->last = NULL;

	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);

    	    tdw->next = NULL;
	    tdw->prev = NULL;
	}

	for (w = s->windows; w; w = w->next)
	{
	    TD_WINDOW (w);

	    if (!tdw->is3D)
		tdAddWindow (w);
	}

	/* all BTF windows in normal order */
	for (w = s->windows; w && !firstFTB; w = w->next)
	{
	    TD_WINDOW (w);

    	    if (!tdw->is3D)
		continue;

	    tds->currentScale = tds->basicScale +
		                (tdw->depth * ((1.0 - tds->basicScale) /
					       tds->maxDepth));

	    tdw->ftb = (*cs->checkOrientation) (s, sAttrib, transform,
						output, vPoints);

	    if (tdw->ftb)
		firstFTB = w;
	    else
		tdAddWindow (w);
	}

	/* all FTB windows in reversed order */
	if (firstFTB)
	{
	    for (w = s->reverseWindows; w && w != firstFTB->prev; w = w->prev)
	    {
		TD_WINDOW (w);

		if (!tdw->is3D)
		    continue;

		tdw->ftb = TRUE;

		tdAddWindow (w);
	    }
	}
    }

    tds->currentScale = tds->basicScale;
    tds->test = TRUE;

    {
	CompTransform sTransform = *transform;
	CompWindow    *w;
	CompWalker    walk;

	screenLighting (s, TRUE);

	(*s->initWindowWalker) (s, &walk);

    	/* paint all windows from bottom to top */
	for (w = (*walk.first) (s); w; w = (*walk.next) (w))
	{
	    CompTransform mTransform = sTransform;

    	    TD_WINDOW (w);

    	    if (w->destroyed)
    		continue;

    	    if (!w->shaded)
    	    {
    		if (w->attrib.map_state != IsViewable || !w->damaged)
    		    continue;
    	    }

	    if (tdw->depth != 0.0f)
	    {
		tds->currentScale = tds->basicScale +
		                    (tdw->depth * ((1.0 - tds->basicScale) /
						   tds->maxDepth));

		(*s->applyScreenTransform) (s, sAttrib, output, &mTransform);
		(*s->enableOutputClipping) (s, &mTransform, region, output);

		transformToScreenSpace (s, output, -sAttrib->zTranslate,
					&mTransform);

		glPushMatrix ();
		glLoadMatrixf (mTransform.m);

		(*s->paintWindow) (w, &w->paint, &mTransform, &infiniteRegion,
				   PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK);
		glPopMatrix ();

		(*s->disableOutputClipping) (s);
	    }
	}
    }

    tds->test = FALSE;
    tds->currentScale = tds->basicScale;
}

static Bool
tdPaintOutput (CompScreen              *s,
	       const ScreenPaintAttrib *sAttrib,
	       const CompTransform     *transform,
	       Region                  region,
	       CompOutput              *output,
	       unsigned int            mask)
{
    Bool status;

    TD_SCREEN (s);

    if (tds->basicScale != 1.0)
    {
	mask |= PAINT_SCREEN_TRANSFORMED_MASK |
	        PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    }

    UNWRAP (tds, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (tds, s, paintOutput, tdPaintOutput);

    return status;
}

static void
tdDonePaintScreen (CompScreen *s)
{
    TD_SCREEN (s);

    if (tds->basicScale != 1.0f)
	damageScreen (s);

    UNWRAP (tds, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (tds, s, donePaintScreen, tdDonePaintScreen);
}

static CompWindow*
tdWalkFirst (CompScreen *s)
{
    TD_SCREEN (s);
    return tds->first;
}

static CompWindow*
tdWalkLast (CompScreen *s)
{
    TD_SCREEN (s);
    return tds->last;
}

static CompWindow*
tdWalkNext (CompWindow *w)
{
    TD_WINDOW (w);
    return tdw->next;
}

static CompWindow*
tdWalkPrev (CompWindow *w)
{
    TD_WINDOW (w);
    return tdw->prev;
}

static void
tdInitWindowWalker (CompScreen *s,
		    CompWalker *walker)
{
    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, s, initWindowWalker);
    (*s->initWindowWalker) (s, walker);
    WRAP (tds, s, initWindowWalker, tdInitWindowWalker);

    if ((tds->active || tds->tdWindowExists) &&
	cs->paintOrder == BTF && tds->test)
    {
	walker->first = tdWalkFirst;
	walker->last =  tdWalkLast;
	walker->next =  tdWalkNext;
	walker->prev =  tdWalkPrev;
    }
}

static Bool
tdInitDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    tdDisplay *tdd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    if (!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    tdd = malloc (sizeof (tdDisplay));
    if (!tdd)
	return FALSE;

    tdd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (tdd->screenPrivateIndex < 0)
    {
	free (tdd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = tdd;

    return TRUE;
}

static void
tdFiniDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    TD_DISPLAY (d);

    freeScreenPrivateIndex (d, tdd->screenPrivateIndex);

    free (tdd);
}

static Bool
tdInitScreen (CompPlugin *p,
	      CompScreen *s)
{
    tdScreen *tds;

    TD_DISPLAY (s->display);
    CUBE_SCREEN (s);

    tds = malloc (sizeof (tdScreen));
    if (!tds)
	return FALSE;

    tds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (tds->windowPrivateIndex < 0)
    {
	free (tds);
	return FALSE;
    }

    tds->active = tds->wasActive = FALSE;

    tds->basicScale     = 1.0;
    tds->currentScale   = 1.0;
    tds->tdWindowExists = FALSE;

    tds->first = NULL;
    tds->last  = NULL;

    s->base.privates[tdd->screenPrivateIndex].ptr = tds;

    WRAP (tds, s, paintWindow, tdPaintWindow);
    WRAP (tds, s, paintOutput, tdPaintOutput);
    WRAP (tds, s, donePaintScreen, tdDonePaintScreen);
    WRAP (tds, s, preparePaintScreen, tdPreparePaintScreen);
    WRAP (tds, s, initWindowWalker, tdInitWindowWalker);
    WRAP (tds, s, applyScreenTransform, tdApplyScreenTransform);
    WRAP (tds, cs, postPaintViewport, tdPostPaintViewport);

    return TRUE;
}

static void
tdFiniScreen (CompPlugin *p,
	      CompScreen *s)
{
    TD_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (tds, s, paintWindow);
    UNWRAP (tds, s, paintOutput);
    UNWRAP (tds, s, donePaintScreen);
    UNWRAP (tds, s, preparePaintScreen);
    UNWRAP (tds, s, initWindowWalker);
    UNWRAP (tds, s, applyScreenTransform);
    UNWRAP (tds, cs, postPaintViewport);

    freeWindowPrivateIndex (s, tds->windowPrivateIndex);
	
    free (tds);
}

static Bool
tdInitWindow (CompPlugin *p,
	      CompWindow *w)
{
    tdWindow *tdw;

    TD_SCREEN (w->screen);

    tdw = malloc (sizeof (tdWindow));
    if (!tdw)
	return FALSE;

    tdw->is3D = FALSE;
    tdw->prev = NULL;
    tdw->next = NULL;

    w->base.privates[tds->windowPrivateIndex].ptr = tdw;

    return TRUE;
}

static void
tdFiniWindow (CompPlugin *p,
	      CompWindow *w)
{
    TD_WINDOW (w);

    free (tdw);
}

static Bool
tdInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
tdFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
tdInitObject (CompPlugin *p,
	      CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) tdInitDisplay,
	(InitPluginObjectProc) tdInitScreen,
	(InitPluginObjectProc) tdInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
tdFiniObject (CompPlugin *p,
	      CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) tdFiniDisplay,
	(FiniPluginObjectProc) tdFiniScreen,
	(FiniPluginObjectProc) tdFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static CompPluginVTable tdVTable = {
    "3d",
    0,
    tdInit,
    tdFini,
    tdInitObject,
    tdFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &tdVTable;
}
