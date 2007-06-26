/**
 *
 * Beryl 3d plugin
 *
 * 3d.c
 *
 * Copyright : (C) 2006 by Roi Cohen
 * E-mail    : roico12@gmail.com
 *
 * Modified and maintained by : Robert Carr <racarr@beryl-project.org>
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
 **/

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
static int cubeDisplayPrivateIndex;

typedef struct _revertReorder
{
	struct _revertReorder *next;
	struct _revertReorder *prev;

	CompWindow *window;

	CompWindow *nextWindow;
	CompWindow *prevWindow;
} RevertReorder;


typedef struct _tdDisplay
{
	int screenPrivateIndex;
} tdDisplay;

typedef struct _tdScreen
{
	int windowPrivateIndex;

	Bool tdWindowExists;
	//Bool reorder;

	PreparePaintScreenProc		preparePaintScreen;
	PaintTransformedOutputProc	paintTransformedOutput;
	PaintOutputProc			paintOutput;
	DonePaintScreenProc		donePaintScreen;
	InitWindowWalkerProc		initWindowWalker;

	CompWindow* firstFTB;
	CompWindow* lastBTF;

	PaintWindowProc paintWindow;

	RevertReorder *revertReorder;

	float maxZ;

	int currentViewportNum;
	float xMove;

	Bool currentDifferentResolutions;

	int currentScreenNum;

	Bool active;

	Bool reorderWindowPainting;
	CompOutput *tmpOutput;
} tdScreen;

typedef struct _tdWindow
{
	float z;
	float currentZ;
	Bool ftb;
} tdWindow;


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



#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

static Bool windowIs3D(CompWindow * w)
{
	if (w->attrib.override_redirect)
		return FALSE;

	if (!(w->shaded || w->attrib.map_state == IsViewable))
		return FALSE;

	if (w->state & (CompWindowStateSkipPagerMask | CompWindowStateSkipTaskbarMask))
		return FALSE;
	
	if (!matchEval(tdGetWindowMatch(w->screen), w))
		return FALSE;

	return TRUE;
}

static Bool differentResolutions(CompScreen * s)
{
	//This code is taken from cube plugin... thanks for whoever wrote it (davidr i guess).

	BoxPtr pBox0, pBox1;
	int i, j, k;

	k = 0;

	for (i = 0; i < s->nOutputDev; i++)
	{
		/* dimensions must match first output */
		if (s->outputDev[i].width != s->outputDev[0].width ||
			s->outputDev[i].height != s->outputDev[0].height)
			continue;

		pBox0 = &s->outputDev[0].region.extents;
		pBox1 = &s->outputDev[i].region.extents;

		/* top and bottom line must match first output */
		if (pBox0->y1 != pBox1->y1 || pBox0->y2 != pBox1->y2)
			continue;

		k++;

		for (j = 0; j < s->nOutputDev; j++)
		{
			pBox0 = &s->outputDev[j].region.extents;

			/* must not intersect other output region */
			if (i != j && pBox0->x2 > pBox1->x1 && pBox0->x1 < pBox1->x2)
			{
				k--;
				break;
			}
		}
	}

	if (k != s->nOutputDev)
		return TRUE;

	return FALSE;
}

#define REAL_POSITION(x, s) ( (x >= 0)? x: x + (s)->hsize * (s)->width )

#define VIEWPORT(x, s) ( ( REAL_POSITION(x, s) / (s)->width ) % (s)->hsize )
#define SCREEN(x, s)   ( ( REAL_POSITION(x, s) % (s)->width ) / (s)->outputDev[0].width )

#define RIGHT_VIEWPORT(w) VIEWPORT( (w)->attrib.x + (w)->attrib.width + (w)->input.right -1, (w)->screen)
#define LEFT_VIEWPORT(w) VIEWPORT( (w)->attrib.x + 1 - (w)->input.left, (w)->screen)

#define RIGHT_SCREEN(w) SCREEN( (w)->attrib.x + (w)->attrib.width -1+w->input.right, (w)->screen)
#define LEFT_SCREEN(w) SCREEN( (w)->attrib.x + 1-w->input.left , (w)->screen)

#define IS_IN_VIEWPORT(w, i) ( ( LEFT_VIEWPORT(w) > RIGHT_VIEWPORT(w) && !(LEFT_VIEWPORT(w) > i && i > RIGHT_VIEWPORT(w)) ) \
                                || ( LEFT_VIEWPORT(w) <= i && i <= RIGHT_VIEWPORT(w) ) )

static void reorder(CompScreen * screen)
{
	CompWindow *firstReordered = NULL;
	CompWindow *next;
	CompWindow *w;

	TD_SCREEN(screen);

	for (w = screen->windows; w && w != firstReordered; w = next)
	{
		next = w->next;

		if (!windowIs3D(w))
			continue;

		if (!firstReordered)
			firstReordered = w;

		if (tds->revertReorder)
		{
			tds->revertReorder->next =
					(RevertReorder *) malloc(sizeof(RevertReorder));
			tds->revertReorder->next->prev = tds->revertReorder;

			tds->revertReorder = tds->revertReorder->next;
		}

		else
		{
			tds->revertReorder =
					(RevertReorder *) malloc(sizeof(RevertReorder));
			tds->revertReorder->prev = NULL;
		}

		tds->revertReorder->next = NULL;

		tds->revertReorder->window = w;
		tds->revertReorder->nextWindow = w->next;
		tds->revertReorder->prevWindow = w->prev;

		unhookWindowFromScreen(screen, w);

		/*This is a faster replacement to insertWindowIntoScreen (screen, w, screen->reverseWindows->id)
		   The original function will go through all the windows until it finds screen->reverseWindows->id
		   But we already know where that window is, so it's unnecessary to go through all the windows. */
		if (screen->windows)
		{
			screen->reverseWindows->next = w;
			w->next = NULL;
			w->prev = screen->reverseWindows;
			screen->reverseWindows = w;
		}

		else
		{
			screen->reverseWindows = screen->windows = w;
			w->prev = w->next = NULL;
		}
	}
}

static void revertReorder(CompScreen * screen)
{
	TD_SCREEN(screen);

	while (tds->revertReorder)
	{
		unhookWindowFromScreen(screen, tds->revertReorder->window);

		tds->revertReorder->window->next = tds->revertReorder->nextWindow;
		tds->revertReorder->window->prev = tds->revertReorder->prevWindow;

		if (tds->revertReorder->nextWindow)
			tds->revertReorder->nextWindow->prev = tds->revertReorder->window;
		else
			screen->reverseWindows = tds->revertReorder->window;

		if (tds->revertReorder->prevWindow)
			tds->revertReorder->prevWindow->next = tds->revertReorder->window;
		else
			screen->windows = tds->revertReorder->window;

		if (tds->revertReorder->prev)
		{
			tds->revertReorder = tds->revertReorder->prev;

			free(tds->revertReorder->next);
			tds->revertReorder->next = NULL;
		}

		else
		{
			free(tds->revertReorder);
			tds->revertReorder = NULL;
		}
	}
}

static void tdPaintAllViewportsEvent(CompScreen* s, Bool paintAllViewports)
{
	CompOption o[2];

	o[0].type = CompOptionTypeInt;
	o[0].name = "root";
	o[0].value.i = s->root;

	o[1].type = CompOptionTypeBool;
	o[1].name = "paintAllViewports";
	o[1].value.b = paintAllViewports;

	(*s->display->handleCompizEvent) (s->display, 
					  "3d", 
					  "paintAllViewportsEvent", 
					   o, 2);
}

static void tdDisableCapsEvent(CompScreen* s, Bool disableCaps)
{
	CompOption o[2];

	o[0].type = CompOptionTypeInt;
	o[0].name = "root";
	o[0].value.i = s->root;

	o[1].type = CompOptionTypeBool;
	o[1].name = "disableCaps";
	o[1].value.b = disableCaps;

	(*s->display->handleCompizEvent) (s->display, 
					  "3d", 
					  "disableCaps", 
					   o, 2);
}

static void tdPreparePaintScreen(CompScreen * screen, int msSinceLastPaint)
{
	tdWindow **lastInViewport;

	CompWindow *w;

	tdWindow *tdw;

	int i;

	float maxZoom;

	TD_SCREEN(screen);
	CUBE_SCREEN (screen);

	tds->active = (cs->rotationState != RotationNone) && 
	              !(tdGetManualOnly(screen) && 
			(cs->rotationState != RotationManual));

	if (tds->currentViewportNum != screen->hsize
		|| tds->currentScreenNum != screen->nOutputDev
		|| tds->currentDifferentResolutions != differentResolutions(screen))
	{
		tds->currentViewportNum = screen->hsize;
		tds->currentScreenNum = screen->nOutputDev;
		tds->currentDifferentResolutions = differentResolutions(screen);

		if (tds->currentViewportNum > 2
			&& (screen->nOutputDev == 1))
			tds->xMove =
					1.0f / (tan (PI * (tds->currentViewportNum - 2.0f) / (2.0f * tds->currentViewportNum)));
		else
			tds->xMove = 0.0f;
	}

	if (tds->active)
	{
		//tds->reorder = TRUE;

		if (tds->tdWindowExists)
			reorder(screen);

		UNWRAP(tds, screen, preparePaintScreen);
		(*screen->preparePaintScreen) (screen, msSinceLastPaint);
		WRAP(tds, screen, preparePaintScreen, tdPreparePaintScreen);

		return;
	}
	
	tdPaintAllViewportsEvent(screen, TRUE);

	lastInViewport = (tdWindow **) malloc(sizeof(tdWindow *) * screen->hsize);

	for (i = 0; i < screen->hsize; i++)
		lastInViewport[i] = NULL;

	tds->maxZ = 0.0f;

	for (w = screen->windows; w; w = w->next)
	{
		if (!windowIs3D(w))
			continue;

		tdw = GET_TD_WINDOW(w, tds);
		maxZoom = 0.0f;

		for (i = 0; i < screen->hsize; i++)
		{
			if (IS_IN_VIEWPORT(w, i))
			{
				if (lastInViewport[i] && lastInViewport[i]->z > maxZoom)
					maxZoom = lastInViewport[i]->z;

				lastInViewport[i] = tdw;
			}
		}

		tdw->z = maxZoom + tdGetSpace(screen);

		if (tdw->z > tds->maxZ)
			tds->maxZ = tdw->z;
	}

	if (tds->maxZ > 0.0f && cs->invert == 1 && tdGetDisableCaps(screen))
		tdDisableCapsEvent(screen, TRUE);

	reorder(screen);

	//tds->reorder = FALSE;

	free(lastInViewport);

	UNWRAP(tds, screen, preparePaintScreen);
	(*screen->preparePaintScreen) (screen, msSinceLastPaint);
	WRAP(tds, screen, preparePaintScreen, tdPreparePaintScreen);
}

static Bool
tdPaintWindow(CompWindow * w,
			  const WindowPaintAttrib * attrib,
			  const CompTransform    *transform,
			  Region region, unsigned int mask)
{
	Bool occlusionDetection = mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK;
	Bool status;
	Bool wasCulled = glIsEnabled(GL_CULL_FACE);

	TD_SCREEN(w->screen);
	CUBE_SCREEN (w->screen);
	TD_WINDOW(w);

	// int output = (tds->tmpOutput->id == ~0) ? 0 : tds->tmpOutput->id;
	int width = w->screen->width;

	CompTransform wTransform = *transform;


	if (tdw->currentZ != 0.0f)
	{
		if (!IS_IN_VIEWPORT(w, 0))
			return TRUE;

		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		if (tdw->ftb)
			glNormal3f(0.0, 0.0, 1.0);
		else
			glNormal3f(0.0, 0.0, -1.0);

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

		int wx, wy, wx2, wy2, ww, wh;

		wx = MAX(0, MIN(w->screen->width, w->attrib.x - w->input.left));
		wx2 = MAX(0, MIN(w->screen->width, w->attrib.x + w->attrib.width + w->input.right));

		wy = MAX(0, MIN(w->screen->height, w->attrib.y - w->input.top));
		wy2 = MAX(0, MIN(w->screen->height, w->attrib.y + w->attrib.height + w->input.bottom));

		ww = wx2 - wx;
		wh = wy2 - wy;

		float wwidth = -(tdGetWidth(w->screen)) / 30;
		int bevel = tdGetBevel(w->screen);


		if ((wwidth != 0) && ww && wh && !w->shaded && !occlusionDetection)
		{
			if (tdw->ftb)
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

			glNormal3f(0.0, 0.0, -1.0);

			return status;
		}
	}

	UNWRAP(tds, w->screen, paintWindow);
	status = (*w->screen->paintWindow) (w, attrib, &wTransform, region, mask);
	WRAP(tds, w->screen, paintWindow, tdPaintWindow);


	if (wasCulled)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);


	return status;
}

static void
tdPaintTransformedOutput(CompScreen * s,
			 const ScreenPaintAttrib * sAttrib,
			 const CompTransform    *transform,
			 Region region, CompOutput *output,
			 unsigned int mask)
{
	TD_SCREEN(s);
	CUBE_SCREEN(s);

	CompWindow* now;
	CompWindow* firstFTB = NULL;
	CompWindow* lastBTF = NULL;

	tds->reorderWindowPainting = FALSE;

	tds->tmpOutput = output;

	if (tds->active ||  tds->tdWindowExists)
	{
		if (tdGetMipmaps(s))
			s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;

		for (now = s->windows; now; now = now->next)
		{
			TD_WINDOW(now);

			float vPoints[3][3] = { { -0.5, 0.0, cs->distance + tdw->currentZ},
						{ 0.0, 0.5, cs->distance + tdw->currentZ},
						{ 0.0, 0.0, cs->distance + tdw->currentZ}};
			
			tdw->ftb = cs->checkOrientation (s, sAttrib, transform, output, vPoints);
			if (tdw->ftb && !firstFTB)
				firstFTB = now;
			else if (!tdw->ftb)
				lastBTF = now;
		}
	}

	tds->firstFTB = firstFTB;

	UNWRAP(tds, s, paintTransformedOutput);
	(*s->paintTransformedOutput) (s, sAttrib, transform, region, output, mask);
	WRAP(tds, s, paintTransformedOutput, tdPaintTransformedOutput);
}

static Bool
tdPaintOutput(CompScreen * s,
			  const ScreenPaintAttrib * sAttrib,
			  const CompTransform    *transform,
			  Region region, CompOutput *output, unsigned int mask)
{
	Bool status;

	TD_SCREEN(s);

	if (tds->active || tds->tdWindowExists)
	{
		mask |= PAINT_SCREEN_TRANSFORMED_MASK |
				PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	}

	UNWRAP(tds, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
	WRAP(tds, s, paintOutput, tdPaintOutput);

	return status;
}

static void tdDonePaintScreen(CompScreen * s)
{
	CompWindow *w;
	tdWindow *tdw;

	TD_SCREEN(s);
	CUBE_SCREEN (s);

	tdDisableCapsEvent(s, FALSE);
	tdPaintAllViewportsEvent(s, FALSE);

	if (tds->active || tds->tdWindowExists)
	{
		float aim = 0.0f;

		damageScreen(s);

		tds->tdWindowExists = FALSE;

		for (w = s->windows; w; w = w->next)
		{
			tdw = GET_TD_WINDOW(w, GET_TD_SCREEN(w->screen,
									GET_TD_DISPLAY(w->screen->display)));

			if (tds->active)
			{
				if (cs->invert == 1)
					aim = tdw->z - tds->maxZ;
				else
					aim = tdw->z;
			}

			if (fabs(tdw->currentZ - aim) < tdGetSpeed(s))
				tdw->currentZ = aim;

			else if (tdw->currentZ < aim)
				tdw->currentZ += tdGetSpeed(s);

			else if (tdw->currentZ > aim)
				tdw->currentZ -= tdGetSpeed(s);

			if (tdw->currentZ)
				tds->tdWindowExists = TRUE;
		}
	}

	revertReorder(s);

	UNWRAP(tds, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP(tds, s, donePaintScreen, tdDonePaintScreen);
}

static CompWindow *
tdWalkFirst (CompScreen *s)
{
	TD_SCREEN(s);

	if (tds->lastBTF == NULL)
		return s->reverseWindows;
	return s->windows;
}

static CompWindow *
tdWalkLast (CompScreen *s)
{
	TD_SCREEN(s);

	if (tds->firstFTB == NULL)
		return s->reverseWindows;
	return tds->firstFTB;
}

static CompWindow *
tdWalkNext (CompWindow *w)
{
	TD_SCREEN(w->screen);
	TD_WINDOW(w);

	if (tdw->ftb)
	{
		if (w == tds->firstFTB)
			return NULL;
		return w->prev;
	}

	if (w == tds->lastBTF)
		return w->screen->reverseWindows;
	return w->next;
}

static CompWindow *
tdWalkPrev (CompWindow *w)
{
	TD_SCREEN(w->screen);
	TD_WINDOW(w);

	if (tdw->ftb)
	{
		if (w == w->screen->reverseWindows)
			return tds->lastBTF;
		return w->next;
	}

	return w->prev;
}

static void
tdInitWindowWalker (CompScreen *s, CompWalker* walker)
{
	TD_SCREEN (s);

	if (tds->active || tds->tdWindowExists)
	{
		walker->first = tdWalkFirst;
		walker->last =  tdWalkLast;
		walker->next =  tdWalkNext;
		walker->prev =  tdWalkPrev;

		return;
	}

	UNWRAP (tds, s, initWindowWalker);
	(*s->initWindowWalker) (s, walker);
	WRAP (tds, s, initWindowWalker, tdInitWindowWalker);
}

static Bool tdInitDisplay(CompPlugin * p, CompDisplay * d)
{
	tdDisplay *tdd;
	CompPlugin *cube = findActivePlugin ("cube");
	CompOption *option;
	int nOption;

	if (!cube || !cube->vTable->getDisplayOptions)
		return FALSE;

	option = (*cube->vTable->getDisplayOptions) (cube, d, &nOption);

	if (getIntOptionNamed (option, nOption, "abi", 0) != CUBE_ABIVERSION)
	{
		compLogMessage (d, "3d", CompLogLevelError,
						"cube ABI version mismatch");
		return FALSE;
	}

	cubeDisplayPrivateIndex = getIntOptionNamed (option, nOption, "index", -1);
	if (cubeDisplayPrivateIndex < 0)
		return FALSE;

	tdd = malloc(sizeof(tdDisplay));
	if (!tdd)
		return FALSE;

	tdd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (tdd->screenPrivateIndex < 0)
	{
		free(tdd);
		return FALSE;
	}

	d->privates[displayPrivateIndex].ptr = tdd;

	return TRUE;
}

static void tdFiniDisplay(CompPlugin * p, CompDisplay * d)
{
	TD_DISPLAY(d);

	freeScreenPrivateIndex(d, tdd->screenPrivateIndex);

	free(tdd);
}

static Bool tdInitScreen(CompPlugin * p, CompScreen * s)
{
	TD_DISPLAY(s->display);

	tdScreen *tds;

	tds = malloc(sizeof(tdScreen));
	if (!tds)
		return FALSE;

	tds->windowPrivateIndex = allocateWindowPrivateIndex(s);
	if (tds->windowPrivateIndex < 0)
	{
		free(tds);
		free(tdd);
		return FALSE;
	}

	tds->tdWindowExists = FALSE;
	//tds->reorder = TRUE;
	tds->revertReorder = NULL;

	tds->currentViewportNum = s->hsize;
	tds->currentScreenNum = s->nOutputDev;
	tds->currentDifferentResolutions = differentResolutions(s);

	tds->xMove = 0.0f;

	WRAP(tds, s, paintTransformedOutput, tdPaintTransformedOutput);
	WRAP(tds, s, paintWindow, tdPaintWindow);
	WRAP(tds, s, paintOutput, tdPaintOutput);
	WRAP(tds, s, donePaintScreen, tdDonePaintScreen);
	WRAP(tds, s, preparePaintScreen, tdPreparePaintScreen);
	WRAP(tds, s, initWindowWalker, tdInitWindowWalker);

	s->privates[tdd->screenPrivateIndex].ptr = tds;

	return TRUE;
}

static void tdFiniScreen(CompPlugin * p, CompScreen * s)
{
	TD_SCREEN(s);

	freeWindowPrivateIndex(s, tds->windowPrivateIndex);

	UNWRAP(tds, s, paintTransformedOutput);
	UNWRAP(tds, s, paintWindow);
	UNWRAP(tds, s, paintOutput);
	UNWRAP(tds, s, donePaintScreen);
	UNWRAP(tds, s, preparePaintScreen);
	UNWRAP(tds, s, initWindowWalker);

	free(tds);
}

static Bool tdInitWindow(CompPlugin * p, CompWindow * w)
{
	tdWindow *tdw;

	TD_SCREEN(w->screen);

	tdw = malloc(sizeof(tdWindow));
	if (!tdw)
		return FALSE;

	tdw->z = 0.0f;
	tdw->currentZ = 0.0f;

	w->privates[tds->windowPrivateIndex].ptr = tdw;

	return TRUE;
}

static void tdFiniWindow(CompPlugin * p, CompWindow * w)
{
	TD_WINDOW(w);

	free(tdw);
}

static Bool tdInit(CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();
	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void tdFini(CompPlugin * p)
{
	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex(displayPrivateIndex);
}

CompPluginDep tdDeps[] = {
	{CompPluginRuleBefore, "cube"}
	,
};

static int tdGetVersion(CompPlugin *p, int version)
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
	/*tdGetDisplayOptions */ 0,
	/*tdSetDisplayOption */ 0,
	0,
	0,
	tdDeps,
	sizeof(tdDeps) / sizeof(tdDeps[0]),
	0,
	0
};

CompPluginVTable *getCompPluginInfo(void)
{
	return &tdVTable;
}
