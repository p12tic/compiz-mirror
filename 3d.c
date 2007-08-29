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

#include <compiz.h>
#include <cube.h>
#include "3d_options.h"

#define PI 3.14159265359f

static int displayPrivateIndex;
static int cubeDisplayPrivateIndex = -1;

typedef struct _tdDisplay
{
    int screenPrivateIndex;

    InitPluginForDisplayProc initPluginForDisplay;
    FiniPluginForDisplayProc finiPluginForDisplay;
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

    PreparePaintScreenProc     preparePaintScreen;
    PaintTransformedOutputProc paintTransformedOutput;
    PaintOutputProc	       paintOutput;
    DonePaintScreenProc	       donePaintScreen;
    InitWindowWalkerProc       initWindowWalker;
    ApplyScreenTransformProc   applyScreenTransform;
    PaintWindowProc            paintWindow;

    CompWindow *first;
    CompWindow *last;

    Bool  test;
    float currentScale;

    float basicScale;
    float maxDepth;
} tdScreen;

#define GET_TD_DISPLAY(d)       \
    ((tdDisplay *) (d)->privates[displayPrivateIndex].ptr)

#define TD_DISPLAY(d)   \
    tdDisplay *tdd = GET_TD_DISPLAY (d)

#define GET_TD_SCREEN(s, tdd)   \
    ((tdScreen *) (s)->privates[(tdd)->screenPrivateIndex].ptr)

#define TD_SCREEN(s)    \
    tdScreen *tds = GET_TD_SCREEN (s, GET_TD_DISPLAY (s->display))

#define GET_TD_WINDOW(w, tds)                                     \
    ((tdWindow *) (w)->privates[(tds)->windowPrivateIndex].ptr)

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
}

static Bool
tdPaintWindow (CompWindow              *w,
	       const WindowPaintAttrib *attrib,
	       const CompTransform     *transform,
	       Region                  region,
	       unsigned int            mask)
{
    Bool       status;
    Bool       wasCulled;
    CompScreen *s = w->screen;

    wasCulled = glIsEnabled (GL_CULL_FACE);

    TD_SCREEN (s);
    TD_WINDOW (w);

#if 0
	if (tdw->currentZ != 0.0f)
	{
		Bool wasCulled;
		int width = w->screen->width;
		int wx, wy, wx2, wy2, ww, wh;
		CompTransform wTransform = *transform;

		CUBE_SCREEN (w->screen);

		if (!IS_IN_VIEWPORT(w, 0))
			return TRUE;
	
		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		

		matrixTranslate(&wTransform, 0.0f, 0.0f, tdw->currentZ);

		if (wasCulled && tdGetDisableCulling(w->screen))
			glDisable(GL_CULL_FACE);

		/*
		if (!IS_IN_VIEWPORT(w, 0))
		{
			float angle = 360 / tds->currentViewportNum;

			matrixScale(&wTransform, 1.0f, 1.0f, 1.0f / width);

			if (RIGHT_VIEWPORT(w) == w->screen->hsize - 1)
			{
				matrixTranslate(&wTransform, -width * tdw->currentZ * tds->xMove, 0.0f, 0.0f);
				matrixRotate(&wTransform, -angle, 0.0f, 1.0f, 0.0f);
				matrixTranslate(&wTransform, -width * tdw->currentZ * tds->xMove, 0.0f, 0.0f);
			}

			else if (LEFT_VIEWPORT(w) == 1)
			{
				matrixTranslate(&wTransform, width +
							 width * tdw->currentZ * tds->xMove, 0.0f, 0.0f);
				matrixRotate(&wTransform, angle, 0.0f, 1.0f, 0.0f);
				matrixTranslate(&wTransform, width * tdw->currentZ *
							 tds->xMove - width, 0.0f, 0.0f);
			}
		}
		*/

		if ((LEFT_VIEWPORT(w) != RIGHT_VIEWPORT(w)) && !cs->unfolded)
		{
			if (LEFT_VIEWPORT(w) == 0)
				matrixTranslate(&wTransform, width * tdw->currentZ * tds->xMove, 0.0f, 0.0f);

			if (RIGHT_VIEWPORT(w) == 0)
				matrixTranslate(&wTransform, -width * tdw->currentZ * tds->xMove, 0.0f, 0.0f);
		}

		wx = MAX(0, MIN(w->screen->width, w->attrib.x - w->input.left));
		wx2 = MAX(0, MIN(w->screen->width, w->attrib.x + w->attrib.width + w->input.right));

		wy = MAX(0, MIN(w->screen->height, w->attrib.y - w->input.top));
		wy2 = MAX(0, MIN(w->screen->height, w->attrib.y + w->attrib.height + w->input.bottom));

		ww = wx2 - wx;
		wh = wy2 - wy;

		float wwidth = -(tdGetWidth(w->screen)) / 30;
		int bevel = tdGetBevel(w->screen);

		if ((wwidth != 0) && ww && wh && !w->shaded &&
			!(mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK))
		{
			if (!tdw->ftb)
			{
				glEnable(GL_CULL_FACE);	// Make sure culling is on.
				glCullFace(GL_FRONT);

				matrixTranslate(&wTransform, 0.0f, 0.0f, wwidth);

				UNWRAP(tds, w->screen, paintWindow);
				status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
				WRAP(tds, w->screen, paintWindow, tdPaintWindow);

				matrixTranslate(&wTransform, 0.0f, 0.0f, -wwidth);
			}

			else
			{
				glEnable(GL_CULL_FACE);	// Make sure culling is on.
				glCullFace(GL_BACK);

				UNWRAP(tds, w->screen, paintWindow);
				status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
				WRAP(tds, w->screen, paintWindow, tdPaintWindow);
			}

			/* Paint window depth. */

			glPushMatrix();
    			glLoadMatrixf(wTransform.m);

			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);

			glBegin(GL_QUADS);
			glColor4f(0.90f, 0.90f, 0.90f, w->paint.opacity/OPAQUE);

	#define DOBEVEL(corner) (tdGetBevel##corner(w->screen) ? bevel : 0)

			/* Top */
			glVertex3f(wx + DOBEVEL(Topleft), wy, 0);
			glVertex3f(wx + ww - DOBEVEL(Topright), wy, 0);
			glVertex3f(wx + ww - DOBEVEL(Topright), wy, (wwidth));
			glVertex3f(wx + DOBEVEL(Topleft), wy, (wwidth));

			/* Bottom */
			glVertex3f(wx + DOBEVEL(Bottomleft), wy + wh, 0);
			glVertex3f(wx + ww - DOBEVEL(Bottomright), wy + wh, 0);
			glVertex3f(wx + ww - DOBEVEL(Bottomright), wy + wh, wwidth);
			glVertex3f(wx + DOBEVEL(Bottomleft), wy + wh, wwidth);


			glColor4f(0.70f, 0.70f, 0.70f,  w->paint.opacity/OPAQUE);

			/* Left */
			if (!(w->attrib.x < w->screen->workArea.x))
			{
				glVertex3f(wx, wy + DOBEVEL(Topleft), 0);
				glVertex3f(wx, wy + wh - DOBEVEL(Bottomleft), 0);
				glVertex3f(wx, wy + wh - DOBEVEL(Bottomleft), wwidth);
				glVertex3f(wx, wy + DOBEVEL(Topleft), wwidth);
			}


			/* Right */
			if (!((w->attrib.x + w->attrib.width + w->input.left +
				  w->input.right) > w->screen->workArea.width))
			{
				glVertex3f(wx + ww, wy + DOBEVEL(Topright), 0);
				glVertex3f(wx + ww, wy + wh - DOBEVEL(Bottomright), 0);
				glVertex3f(wx + ww, wy + wh - DOBEVEL(Bottomright), wwidth);
				glVertex3f(wx + ww, wy + DOBEVEL(Topright), wwidth);
			}

			glColor4f(0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);

			if (!(w->attrib.x < w->screen->workArea.x))
			{
				/* Top left bevel */
				if (tdGetBevelTopleft(w->screen))
				{
					glVertex3f(wx, wy + bevel, wwidth);
					glVertex3f(wx, wy + bevel, 0);
					glVertex3f(wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);
					glVertex3f(wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, wwidth);

					glColor4f(1.0f, 1.0f, 1.0f,  w->paint.opacity/OPAQUE);

					glVertex3f(wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);
					glVertex3f(wx + bevel / 2.0f, wy + bevel - bevel / 1.2f, wwidth);
					glVertex3f(wx + bevel, wy, wwidth);
					glVertex3f(wx + bevel, wy, 0);

					glColor4f(0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);
				}
				/* Bottom left bevel */
				if (tdGetBevelBottomleft(w->screen))
				{
					glVertex3f(wx, wy + wh - bevel, 0);
					glVertex3f(wx, wy + wh - bevel, wwidth);
					glVertex3f(wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, wwidth);
					glVertex3f(wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, 0);

					glColor4f(1.0f, 1.0f, 1.0f,  w->paint.opacity / OPAQUE);

					glVertex3f(wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, wwidth);
					glVertex3f(wx + bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, 0);
					glVertex3f(wx + bevel, wy + wh, 0);
					glVertex3f(wx + bevel, wy + wh, wwidth);
				}
			}

			glColor4f(0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);


			if (!((w->attrib.x + w->attrib.width + w->input.left +
				  w->input.right) > w->screen->workArea.width))
			{

				/* Bottom right bevel */
				if (tdGetBevelBottomright(w->screen))
				{
					glVertex3f(wx + ww - bevel, wy + wh, 0);
					glVertex3f(wx + ww - bevel, wy + wh, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f, wy + wh - bevel + bevel / 1.2f, 0);

					glColor4f(1.0f, 1.0f, 1.0f,  w->paint.opacity/OPAQUE);

					glVertex3f(wx + ww - bevel / 2.0f,
							   wy + wh - bevel + bevel / 1.2f, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f,
							   wy + wh - bevel + bevel / 1.2f, 0);
					glVertex3f(wx + ww, wy + wh - bevel, 0);
					glVertex3f(wx + ww, wy + wh - bevel, wwidth);

					glColor4f(0.95f, 0.95f, 0.95f,  w->paint.opacity / OPAQUE);
				}
				/* Top right bevel */
				if (tdGetBevelTopright(w->screen))
				{
					glVertex3f(wx + ww - bevel, wy, 0);
					glVertex3f(wx + ww - bevel, wy, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);

					glColor4f(1.0f, 1.0f, 1.0f,  w->paint.opacity/OPAQUE);

					glVertex3f(wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f, wwidth);
					glVertex3f(wx + ww - bevel / 2.0f, wy + bevel - bevel / 1.2f, 0);
					glVertex3f(wx + ww, wy + bevel, 0);
					glVertex3f(wx + ww, wy + bevel, wwidth);
				}
			}

			glEnd();
			glPopMatrix();

			if (tdw->ftb)
			{
				glEnable(GL_CULL_FACE);	// Re-enable culling.
				glCullFace(GL_FRONT);

				matrixTranslate(&wTransform, 0.0f, 0.0f, wwidth);

				UNWRAP(tds, w->screen, paintWindow);
				status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
				WRAP(tds, w->screen, paintWindow, tdPaintWindow);

				matrixTranslate(&wTransform, 0.0f, 0.0f, -wwidth);
			}
			else
			{
				glEnable(GL_CULL_FACE);	// Re-enable culling.
				glCullFace(GL_BACK);

				UNWRAP(tds, w->screen, paintWindow);
				status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
				WRAP(tds, w->screen, paintWindow, tdPaintWindow);
			}

			glCullFace(GL_BACK);

			if (!wasCulled)
				glDisable(GL_CULL_FACE);

			return status;
		}

		UNWRAP(tds, w->screen, paintWindow);
		status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
		WRAP(tds, w->screen, paintWindow, tdPaintWindow);
	}
	else
#endif

    if (tdw->depth != 0.0f && !tds->test && tds->basicScale != 1.0)
	mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (tds->test)
    {
	glDisable (GL_CULL_FACE);

	UNWRAP (tds, s, paintWindow);
	status = (*s->paintWindow) (w, attrib, transform, region, mask);
	WRAP (tds, s, paintWindow, tdPaintWindow);

	if (wasCulled)
	    glEnable (GL_CULL_FACE);
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
tdPaintTransformedOutput (CompScreen              *s,
			  const ScreenPaintAttrib *sAttrib,
			  const CompTransform     *transform,
			  Region                  region,
			  CompOutput              *output,
			  unsigned int            mask)
{
    CompWindow* w;
    CompWindow* firstFTB = NULL;

    TD_SCREEN (s);
    CUBE_SCREEN (s);

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
    UNWRAP (tds, s, paintTransformedOutput);
    (*s->paintTransformedOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (tds, s, paintTransformedOutput, tdPaintTransformedOutput);

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

#if 0
		//(*s->enableOutputClipping) (s, &mTransform, region, output);
#else
		GLdouble h = s->height;

		GLdouble p1[2] = { region->extents.x1, h - region->extents.y2 };
		GLdouble p2[2] = { region->extents.x2, h - region->extents.y1 };

		GLdouble halfW = output->width / 2.0;
		GLdouble halfH = output->height / 2.0;

		GLdouble cx = output->region.extents.x1 + halfW;
		GLdouble cy = (h - output->region.extents.y2) + halfH;

		GLdouble top[4]    = { 0.0, halfH / (cy - p1[1]), 0.0, 0.5 };
		GLdouble bottom[4] = { 0.0, halfH / (cy - p2[1]), 0.0, 0.5 };
		GLdouble left[4]   = { halfW / (cx - p1[0]), 0.0, 0.0, 0.5 };
		GLdouble right[4]  = { halfW / (cx - p2[0]), 0.0, 0.0, 0.5 };

		glPushMatrix ();
		glLoadMatrixf (mTransform.m);

		glClipPlane (GL_CLIP_PLANE0, top);
		glClipPlane (GL_CLIP_PLANE1, bottom);
		glClipPlane (GL_CLIP_PLANE2, left);
		glClipPlane (GL_CLIP_PLANE3, right);

		glEnable (GL_CLIP_PLANE0);
		glEnable (GL_CLIP_PLANE1);
		glEnable (GL_CLIP_PLANE2);
		glEnable (GL_CLIP_PLANE3);
#endif

		transformToScreenSpace (s, output, -sAttrib->zTranslate,
					&mTransform);

		glLoadMatrixf (mTransform.m);

		(*s->paintWindow) (w, &w->paint, &mTransform, &infiniteRegion,
				   PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK);
		glPopMatrix ();

#if 0
		//(*s->disableOutputClipping) (s);
#else
		glDisable (GL_CLIP_PLANE0);
		glDisable (GL_CLIP_PLANE1);
		glDisable (GL_CLIP_PLANE2);
		glDisable (GL_CLIP_PLANE3);
#endif
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
tdInitPluginForDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    Bool status;

    TD_DISPLAY (d);

    if (strcmp (p->vTable->name, "cube") == 0)
    {
	CompOption *option;
	int        nOption;

	if (!p->vTable->getDisplayOptions)
	{
	    compLogMessage (d, "3d", CompLogLevelError,
			    "Can't get cube plugin vTable");
	}
	else
	{
	    int abi;

	    option = (*p->vTable->getDisplayOptions) (p, d, &nOption);
	    abi = getIntOptionNamed (option, nOption, "abi", 0);

	    if (abi != CUBE_ABIVERSION)
	    {
		compLogMessage (d, "3d", CompLogLevelError,
				"cube ABI version mismatch");
	    }
	    else
	    {
		cubeDisplayPrivateIndex = getIntOptionNamed (option, nOption,
							     "index", -1);
	    }
	}

	if (cubeDisplayPrivateIndex >= 0)
	{
	    CompScreen *s;

	    /* we have to wrap those functions here in order to have
	       them wrapped before the cube functions */
	    for (s = d->screens; s; s = s->next)
	    {
		TD_SCREEN (s);

		WRAP (tds, s, paintTransformedOutput, tdPaintTransformedOutput);
		WRAP (tds, s, paintWindow, tdPaintWindow);
		WRAP (tds, s, paintOutput, tdPaintOutput);
		WRAP (tds, s, donePaintScreen, tdDonePaintScreen);
		WRAP (tds, s, preparePaintScreen, tdPreparePaintScreen);
		WRAP (tds, s, initWindowWalker, tdInitWindowWalker);
		WRAP (tds, s, applyScreenTransform, tdApplyScreenTransform);
	    }
	}
    }

    UNWRAP (tdd, d, initPluginForDisplay);
    status = (*d->initPluginForDisplay) (p, d);
    WRAP (tdd, d, initPluginForDisplay, tdInitPluginForDisplay);

    return status;
}

static void
tdFiniPluginForDisplay (CompPlugin  *p,
			CompDisplay *d)
{
    TD_DISPLAY (d);

    UNWRAP (tdd, d, finiPluginForDisplay);
    (*d->finiPluginForDisplay) (p, d);
    WRAP (tdd, d, finiPluginForDisplay, tdFiniPluginForDisplay);

    if (strcmp (p->vTable->name, "cube") == 0)
    {
	CompScreen *s;
	for (s = d->screens; s; s = s->next)
	{
	    TD_SCREEN (s);
	    UNWRAP (tds, s, paintTransformedOutput);
	    UNWRAP (tds, s, paintWindow);
	    UNWRAP (tds, s, paintOutput);
	    UNWRAP (tds, s, donePaintScreen);
	    UNWRAP (tds, s, preparePaintScreen);
	    UNWRAP (tds, s, initWindowWalker);
	    UNWRAP (tds, s, applyScreenTransform);
	}
	cubeDisplayPrivateIndex = -1;
    }
}

static Bool
tdInitDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    tdDisplay *tdd;

    tdd = malloc (sizeof (tdDisplay));
    if (!tdd)
	return FALSE;

    tdd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (tdd->screenPrivateIndex < 0)
    {
	free (tdd);
	return FALSE;
    }

    d->privates[displayPrivateIndex].ptr = tdd;

    WRAP (tdd, d, initPluginForDisplay, tdInitPluginForDisplay);
    WRAP (tdd, d, finiPluginForDisplay, tdFiniPluginForDisplay);

    return TRUE;
}

static void
tdFiniDisplay (CompPlugin  *p,
	       CompDisplay *d)
{
    TD_DISPLAY (d);

    freeScreenPrivateIndex (d, tdd->screenPrivateIndex);

    UNWRAP (tdd, d, initPluginForDisplay);
    UNWRAP (tdd, d, finiPluginForDisplay);

    free (tdd);
}

static Bool
tdInitScreen (CompPlugin *p,
	      CompScreen *s)
{
    tdScreen *tds;

    TD_DISPLAY (s->display);

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

    s->privates[tdd->screenPrivateIndex].ptr = tds;

    return TRUE;
}

static void
tdFiniScreen (CompPlugin *p,
	      CompScreen *s)
{
    TD_SCREEN (s);

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

    w->privates[tds->windowPrivateIndex].ptr = tdw;

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

static int
tdGetVersion (CompPlugin *p,
	      int        version)
{
    return ABIVERSION;
}

static CompPluginVTable tdVTable = {
    "3d",
    tdGetVersion,
    0,
    tdInit,
    tdFini,
    tdInitDisplay,
    tdFiniDisplay,
    tdInitScreen,
    tdFiniScreen,
    tdInitWindow,
    tdFiniWindow,
    0, /*tdGetDisplayOptions */
    0, /*tdSetDisplayOption */
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &tdVTable;
}
