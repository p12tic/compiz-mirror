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
#include "3d_options.h"

#define PI 3.14159265359f

#define MULTM(x,y,z) \
z[0] = x[0] * y[0] + x[4] * y[1] + x[8] * y[2] + x[12] * y[3]; \
z[1] = x[1] * y[0] + x[5] * y[1] + x[9] * y[2] + x[13] * y[3]; \
z[2] = x[2] * y[0] + x[6] * y[1] + x[10] * y[2] + x[14] * y[3]; \
z[3] = x[3] * y[0] + x[7] * y[1] + x[11] * y[2] + x[15] * y[3]; \
z[4] = x[0] * y[4] + x[4] * y[5] + x[8] * y[6] + x[12] * y[7]; \
z[5] = x[1] * y[4] + x[5] * y[5] + x[9] * y[6] + x[13] * y[7]; \
z[6] = x[2] * y[4] + x[6] * y[5] + x[10] * y[6] + x[14] * y[7]; \
z[7] = x[3] * y[4] + x[7] * y[5] + x[11] * y[6] + x[15] * y[7]; \
z[8] = x[0] * y[8] + x[4] * y[9] + x[8] * y[10] + x[12] * y[11]; \
z[9] = x[1] * y[8] + x[5] * y[9] + x[9] * y[10] + x[13] * y[11]; \
z[10] = x[2] * y[8] + x[6] * y[9] + x[10] * y[10] + x[14] * y[11]; \
z[11] = x[3] * y[8] + x[7] * y[9] + x[11] * y[10] + x[15] * y[11]; \
z[12] = x[0] * y[12] + x[4] * y[13] + x[8] * y[14] + x[12] * y[15]; \
z[13] = x[1] * y[12] + x[5] * y[13] + x[9] * y[14] + x[13] * y[15]; \
z[14] = x[2] * y[12] + x[6] * y[13] + x[10] * y[14] + x[14] * y[15]; \
z[15] = x[3] * y[12] + x[7] * y[13] + x[11] * y[14] + x[15] * y[15];

#define MULTMV(m, v) { \
float v0 = m[0]*v[0] + m[4]*v[1] + m[8]*v[2] + m[12]*v[3]; \
float v1 = m[1]*v[0] + m[5]*v[1] + m[9]*v[2] + m[13]*v[3]; \
float v2 = m[2]*v[0] + m[6]*v[1] + m[10]*v[2] + m[14]*v[3]; \
float v3 = m[3]*v[0] + m[7]*v[1] + m[11]*v[2] + m[15]*v[3]; \
v[0] = v0; v[1] = v1; v[2] = v2; v[3] = v3; }

#define DIVV(v) \
v[0] /= v[3]; \
v[1] /= v[3]; \
v[2] /= v[3]; \
v[3] /= v[3];

static int displayPrivateIndex;

typedef struct _revertReorder
{
	struct _revertReorder *next;
	struct _revertReorder *prev;

	CompWindow *window;

	CompWindow *nextWindow;
	CompWindow *prevWindow;
} RevertReorder;


typedef enum _MultiMonitorMode
{
	Multiple,
	OneBig,
} MultiMonitorMode;

typedef enum _RotationMode
{
	NoRotation = 0,
	RotationChange,
	RotationManual
} RotationMode;

typedef struct _tdDisplay
{
	HandleCompizEventProc handleCompizEvent;
	int screenPrivateIndex;
} tdDisplay;

typedef struct _tdScreen
{
	int windowPrivateIndex;

	Bool tdWindowExists;
	//Bool reorder;

	PreparePaintScreenProc preparePaintScreen;
	PaintTransformedScreenProc paintTransformedScreen;
	PaintScreenProc paintScreen;
	DonePaintScreenProc donePaintScreen;

	PaintWindowProc paintWindow;
	SetScreenOptionForPluginProc setScreenOptionForPlugin;

	RevertReorder *revertReorder;

	float maxZ;

	int currentViewportNum;
	float xMove;

	MultiMonitorMode currentMmMode;
	MultiMonitorMode mmMode;
	Bool insideCube;
	Bool sticky;
	Bool cubeUnfolded;

	RotationMode rotationMode;

	Bool currentDifferentResolutions;

	int currentScreenNum;

	Bool reorderWindowPainting;
	int tmpOutput;
} tdScreen;

typedef struct _tdWindow
{
	float z;
	float currentZ;
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
	TD_SCREEN(w->screen);
	
	if (w->attrib.override_redirect)
		return FALSE;

	if (!(w->shaded || w->attrib.map_state == IsViewable))
		return FALSE;

	if (tds->sticky &&
		(w->state & CompWindowStateStickyMask && !(w->state & CompWindowStateBelowMask)))
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

#define DO_3D(s) ((tds->rotationMode != NoRotation) && !(tdGetManualOnly(s) && \
			     (tds->rotationMode != RotationManual)))

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

static void tdHandleCompizEvent(CompDisplay * d, char *pluginName,
				 char *eventName, CompOption * option, int nOption)
{
	TD_DISPLAY(d);

	UNWRAP (tdd, d, handleCompizEvent);
	(*d->handleCompizEvent) (d, pluginName, eventName, option, nOption);
	WRAP (tdd, d, handleCompizEvent, tdHandleCompizEvent);

	if (strcmp(pluginName, "cube") == 0)
	{
		if (strcmp(eventName, "unfoldEvent") == 0)
		{
			Window xid = getIntOptionNamed(option, nOption, "root", 0);
			CompScreen *s = findScreenAtDisplay(d, xid);

			if (s)
			{
				TD_SCREEN(s);
				tds->cubeUnfolded = getBoolOptionNamed(option, nOption, "unfolded", FALSE);
			}
		}
	} 
	else if (strcmp(pluginName, "rotate") == 0)
	{
		if (strcmp(eventName, "rotateEvent") == 0)
		{
			Window xid = getIntOptionNamed(option, nOption, "root", 0);
			CompScreen *s = findScreenAtDisplay(d, xid);

			if (s)
			{
				TD_SCREEN(s);
				Bool rotate = getBoolOptionNamed(option, nOption, "rotating", FALSE);
				Bool manual = getBoolOptionNamed(option, nOption, "manual", FALSE);

				if (rotate && manual)
					tds->rotationMode = RotationManual;
				else if (rotate)
					tds->rotationMode = RotationChange;
				else
					tds->rotationMode = NoRotation;
			}
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

	if (tds->currentMmMode != tds->mmMode
		|| tds->currentViewportNum != screen->hsize
		|| tds->currentScreenNum != screen->nOutputDev
		|| tds->currentDifferentResolutions != differentResolutions(screen))
	{
		tds->currentViewportNum = screen->hsize;
		tds->currentMmMode = tds->mmMode;
		tds->currentScreenNum = screen->nOutputDev;
		tds->currentDifferentResolutions = differentResolutions(screen);

		if (tds->currentViewportNum > 2
			&& (tds->currentMmMode != Multiple || screen->nOutputDev == 1))
			tds->xMove =
					1.0f / (tan (PI * (tds->currentViewportNum - 2.0f) / (2.0f * tds->currentViewportNum)));
		else
			tds->xMove = 0.0f;
	}

	if (!DO_3D(screen))
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

	if (tds->maxZ > 0.0f && tds->insideCube && tdGetDisableCaps(screen))
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
	TD_WINDOW(w);

	int output = tds->tmpOutput;
	int width = w->screen->width;

	if (DO_3D(w->screen) && tds->reorderWindowPainting)
	{
		// Window painting is done twice, once in reverse mode and one in normal.
		// We should paint it only in the needed mode.

		if (((w->attrib.y + w->attrib.height) > w->screen->workArea.height)
			&& (w->attrib.x > w->screen->workArea.x) &&
			((w->attrib.x + w->attrib.width) < w->screen->workArea.width))

			mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		float mvp[16];

		MULTM(w->screen->projection, transform->m, mvp);

		float pntA[4] = { w->screen->outputDev[output].region.extents.x1,
			w->screen->outputDev[output].region.extents.y1,
			tdw->currentZ, 1
		};

		float pntB[4] = { w->screen->outputDev[output].region.extents.x2,
			w->screen->outputDev[output].region.extents.y1,
			tdw->currentZ, 1
		};

		float pntC[4] =
				{ w->screen->outputDev[output].region.extents.x1 +
			w->screen->outputDev[output].width / 2.0f,
			w->screen->outputDev[output].region.extents.y1 +
					w->screen->outputDev[output].height / 2.0f,
			tdw->currentZ, 1
		};

		MULTMV(mvp, pntA);
		DIVV(pntA);

		MULTMV(mvp, pntB);
		DIVV(pntB);

		MULTMV(mvp, pntC);
		DIVV(pntC);

		float vecA[3] = { pntC[0] - pntA[0], pntC[1] - pntA[1],
			pntC[2] - pntA[2]
		};
		float vecB[3] = { pntC[0] - pntB[0], pntC[1] - pntB[1],
			pntC[2] - pntB[2]
		};

		float ortho[3] = { vecA[1] * vecB[2] - vecA[2] * vecB[1],
			vecA[2] * vecB[0] - vecA[0] * vecB[2],
			vecA[0] * vecB[1] - vecA[1] * vecB[0]
		};

		if (ortho[2] > 0.0f)	//The window is reversed, should be painted front to back.
		{
			if (mask & PAINT_WINDOW_BACK_TO_FRONT_MASK)
				return (occlusionDetection)? FALSE: TRUE;
		}
		
		else if (mask & PAINT_WINDOW_FRONT_TO_BACK_MASK)
			return (occlusionDetection)? FALSE: TRUE;
	}

	CompTransform wTransform = *transform;


	if (tdw->currentZ != 0.0f)
	{
		if (!IS_IN_VIEWPORT(w, 0))
			return TRUE;

		mask |= PAINT_WINDOW_TRANSFORMED_MASK;

		if (mask & PAINT_WINDOW_FRONT_TO_BACK_MASK)
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

		if ((LEFT_VIEWPORT(w) != RIGHT_VIEWPORT(w)) && !tds->cubeUnfolded)
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
			if (mask & PAINT_WINDOW_BACK_TO_FRONT_MASK)
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

			if (mask & PAINT_WINDOW_FRONT_TO_BACK_MASK)
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

static Bool
tdSetScreenOptionForPlugin(CompScreen *s, char *plugin,
						   char *name, CompOptionValue *value)
{
    Bool status;
    TD_SCREEN (s);

    UNWRAP (tds, s, setScreenOptionForPlugin);
    status = (*s->setScreenOptionForPlugin) (s, plugin, name, value);
    WRAP (tds, s, setScreenOptionForPlugin, tdSetScreenOptionForPlugin);

    if (status && strcmp (plugin, "cube") == 0)
	{
		if (strcmp (name, "in") == 0)
			tds->insideCube = value->b;
		else if (strcmp (name, "multimonitor_mode") == 0)
			tds->mmMode = value->i;
		else if (strcmp (name, "stuck_to_screen") == 0)
			tds->sticky = value->b;
	}

    return status;
}

static void
tdPaintTransformedScreen(CompScreen * s,
						 const ScreenPaintAttrib * sAttrib,
						 const CompTransform    *transform,
						 Region region, int output, unsigned int mask)
{
	TD_SCREEN(s);

	tds->reorderWindowPainting = FALSE;

	tds->tmpOutput = output;

	if (DO_3D(s))
	{
		if (tdGetMipmaps(s))
			s->display->textureFilter = GL_LINEAR_MIPMAP_LINEAR;

		/* Front to back should always be done.
		   If FTB is already in mask, then the viewport is reversed, and all windows should be reversed.
		   If BTF is in mask, the viewport isn't reversed, but some of the windows there might be, so we set FTB in addition to BTF, and check for each window what mode it should use... */

		if (!(mask & PAINT_SCREEN_ORDER_FRONT_TO_BACK_MASK) && !tds->insideCube)
		{
			tds->reorderWindowPainting = TRUE;
			mask |= PAINT_SCREEN_ORDER_FRONT_TO_BACK_MASK | PAINT_SCREEN_ORDER_BACK_TO_FRONT_MASK;
		}
	}

	UNWRAP(tds, s, paintTransformedScreen);
	(*s->paintTransformedScreen) (s, sAttrib, transform, region, output, mask);
	WRAP(tds, s, paintTransformedScreen, tdPaintTransformedScreen);
}

static Bool
tdPaintScreen(CompScreen * s,
			  const ScreenPaintAttrib * sAttrib,
			  const CompTransform    *transform,
			  Region region, int output, unsigned int mask)
{
	Bool status;

	TD_SCREEN(s);

	if (DO_3D(s) || tds->tdWindowExists)
	{
		mask |= PAINT_SCREEN_TRANSFORMED_MASK |
				PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
	}

	UNWRAP(tds, s, paintScreen);
	status = (*s->paintScreen) (s, sAttrib, transform, region, output, mask);
	WRAP(tds, s, paintScreen, tdPaintScreen);

	return status;
}

static void tdDonePaintScreen(CompScreen * s)
{
	CompWindow *w;
	tdWindow *tdw;

	TD_SCREEN(s);

	tdDisableCapsEvent(s, FALSE);
	tdPaintAllViewportsEvent(s, FALSE);

	if (DO_3D(s) || tds->tdWindowExists)
	{
		float aim = 0.0f;

		damageScreen(s);

		tds->tdWindowExists = FALSE;

		for (w = s->windows; w; w = w->next)
		{
			tdw = GET_TD_WINDOW(w, GET_TD_SCREEN(w->screen,
									GET_TD_DISPLAY(w->screen->display)));

			if (DO_3D(s))
			{
				if (tds->sticky && ((w->type & CompWindowTypeDockMask) ||
					(w->state & CompWindowStateStickyMask && !(w->state & CompWindowStateBelowMask))))
						aim = 0;
				else if (tds->insideCube)
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

static Bool tdInitDisplay(CompPlugin * p, CompDisplay * d)
{
	tdDisplay *tdd;

	tdd = malloc(sizeof(tdDisplay));
	if (!tdd)
		return FALSE;

	tdd->screenPrivateIndex = allocateScreenPrivateIndex(d);
	if (tdd->screenPrivateIndex < 0)
	{
		free(tdd);
		return FALSE;
	}

	WRAP (tdd, d, handleCompizEvent, tdHandleCompizEvent);

	d->privates[displayPrivateIndex].ptr = tdd;

	return TRUE;
}

static void tdFiniDisplay(CompPlugin * p, CompDisplay * d)
{
	TD_DISPLAY(d);

	UNWRAP (tdd, d, handleCompizEvent);

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

	tds->mmMode = tds->currentMmMode = OneBig;
	tds->rotationMode = NoRotation;
	tds->insideCube = FALSE;
	tds->sticky = FALSE;
	tds->cubeUnfolded = FALSE;

	tds->currentViewportNum = s->hsize;
	tds->currentScreenNum = s->nOutputDev;
	tds->currentDifferentResolutions = differentResolutions(s);

	if (tds->currentViewportNum > 2 && tds->currentMmMode != Multiple)
		tds->xMove =
				1.0f /
				(tan
				 (PI * (tds->currentViewportNum - 2.0f) /
				  (2.0f * tds->currentViewportNum)));
	else
		tds->xMove = 0.0f;

	WRAP(tds, s, paintTransformedScreen, tdPaintTransformedScreen);
	WRAP(tds, s, paintWindow, tdPaintWindow);
	WRAP(tds, s, paintScreen, tdPaintScreen);
	WRAP(tds, s, donePaintScreen, tdDonePaintScreen);
	WRAP(tds, s, preparePaintScreen, tdPreparePaintScreen);
	WRAP(tds, s, setScreenOptionForPlugin, tdSetScreenOptionForPlugin);

	s->privates[tdd->screenPrivateIndex].ptr = tds;

	return TRUE;
}

static void tdFiniScreen(CompPlugin * p, CompScreen * s)
{
	TD_SCREEN(s);

	freeWindowPrivateIndex(s, tds->windowPrivateIndex);

	UNWRAP(tds, s, paintTransformedScreen);
	UNWRAP(tds, s, paintWindow);
	UNWRAP(tds, s, paintScreen);
	UNWRAP(tds, s, donePaintScreen);
	UNWRAP(tds, s, preparePaintScreen);
	UNWRAP(tds, s, setScreenOptionForPlugin);

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
	{CompPluginRuleAfter, "decoration"}
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
