/*
 * Compiz cube reflection plugin
 *
 * cubereflex.c
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
 *
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include <compiz-core.h>
#include <compiz-cube.h>

#include "cubereflex_options.h"

static int displayPrivateIndex;
static int cubeDisplayPrivateIndex;

#define CUBEREFLEX_GRID_SIZE    100
#define CUBEREFLEX_CAP_ELEMENTS 30

typedef struct _CubereflexDisplay
{
    int screenPrivateIndex;
} CubereflexDisplay;

typedef struct _CubereflexScreen
{
    DonePaintScreenProc        donePaintScreen;
    PaintOutputProc            paintOutput;
    PaintScreenProc            paintScreen;
    PreparePaintScreenProc     preparePaintScreen;
    PaintTransformedOutputProc paintTransformedOutput;
    AddWindowGeometryProc      addWindowGeometry;

    CubeClearTargetOutputProc   clearTargetOutput;
    CubeGetRotationProc	        getRotation;
    CubeCheckOrientationProc    checkOrientation;
    CubeShouldPaintViewportProc shouldPaintViewport;
    CubePaintTopProc	        paintTop;
    CubePaintBottomProc	        paintBottom;

    Bool reflection;
    Bool first;

    CompOutput *last;

    float yTrans;
    float zTrans;

    float backVRotate;
    float vRot;

    float deform;

    Region tmpRegion;

    GLfloat capFill[CUBEREFLEX_CAP_ELEMENTS * 12];
} CubereflexScreen;

#define GET_CUBEREFLEX_DISPLAY(d) \
    ((CubereflexDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define CUBEREFLEX_DISPLAY(d) \
    CubereflexDisplay *rd = GET_CUBEREFLEX_DISPLAY (d);

#define GET_CUBEREFLEX_SCREEN(s, rd) \
    ((CubereflexScreen *) (s)->base.privates[(rd)->screenPrivateIndex].ptr)
#define CUBEREFLEX_SCREEN(s) \
    CubereflexScreen *rs = GET_CUBEREFLEX_SCREEN (s,           \
			   GET_CUBEREFLEX_DISPLAY (s->display))

static void
drawBasicGround (CompScreen *s)
{
    float i;

    glPushMatrix ();

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLoadIdentity ();
    glTranslatef (0.0, 0.0, -DEFAULT_Z_CAMERA);

    i = cubereflexGetIntensity (s) * 2;

    glBegin (GL_QUADS);
    glColor4f (0.0, 0.0, 0.0, MAX (0.0, 1.0 - i) );
    glVertex2f (0.5, 0.0);
    glVertex2f (-0.5, 0.0);
    glColor4f (0.0, 0.0, 0.0, MIN (1.0, 1.0 - (i - 1.0) ) );
    glVertex2f (-0.5, -0.5);
    glVertex2f (0.5, -0.5);
    glEnd ();

    if (cubereflexGetGroundSize (s) > 0.0)
    {
	glBegin (GL_QUADS);
	glColor4usv (cubereflexGetGroundColor1 (s) );
	glVertex2f (-0.5, -0.5);
	glVertex2f (0.5, -0.5);
	glColor4usv (cubereflexGetGroundColor2 (s) );
	glVertex2f (0.5, -0.5 + cubereflexGetGroundSize (s) );
	glVertex2f (-0.5, -0.5 + cubereflexGetGroundSize (s) );
	glEnd ();
    }

    glColor4usv (defaultColor);

    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_BLEND);
    glPopMatrix ();
}

static Bool
cubereflexCheckOrientation (CompScreen              *s,
			    const ScreenPaintAttrib *sAttrib,
			    const CompTransform     *transform,
			    CompOutput              *outputPtr,
			    CompVector              *points)
{
    Bool status;

    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    UNWRAP (rs, cs, checkOrientation);
    status = (*cs->checkOrientation) (s, sAttrib, transform,
				      outputPtr, points);
    WRAP (rs, cs, checkOrientation, cubereflexCheckOrientation);

    if (rs->reflection)
	return !status;

    return status;
}

static void
cubereflexGetRotation (CompScreen *s,
		       float      *x,
		       float      *v,
		       float      *progress)
{
    CUBE_SCREEN (s);
    CUBEREFLEX_SCREEN (s);

    UNWRAP (rs, cs, getRotation);
    (*cs->getRotation) (s, x, v, progress);
    WRAP (rs, cs, getRotation, cubereflexGetRotation);

    if (cubereflexGetMode (s) == ModeAbove && *v > 0.0 && rs->reflection)
    {
	rs->vRot = *v;
	*v = 0.0;
    }
    else
	rs->vRot = 0.0;
}

static void
cubereflexClearTargetOutput (CompScreen *s,
			     float      xRotate,
			     float      vRotate)
{
    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    if (rs->reflection)
	glCullFace (GL_BACK);

    UNWRAP (rs, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, rs->backVRotate);
    WRAP (rs, cs, clearTargetOutput, cubereflexClearTargetOutput);

    if (rs->reflection)
	glCullFace (GL_FRONT);
}

static Bool
cubereflexShouldPaintViewport (CompScreen              *s,
			       const ScreenPaintAttrib *sAttrib,
			       const CompTransform     *transform,
			       CompOutput              *outputPtr,
			       PaintOrder              order)
{
    Bool rv = FALSE;

    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    if (rs->deform > 0.0)
    {
	float z[3];
	Bool  ftb1, ftb2, ftb3;

	z[0] = cs->invert * cs->distance;
	z[1] = z[0] + (0.25 / cs->distance);
	z[2] = cs->invert * sqrt (0.25 + (cs->distance * cs->distance));

	CompVector vPoints[3][3] = { { {.v = { -0.5,  0.0, z[0], 1.0 } },
				       {.v = {  0.0,  0.5, z[1], 1.0 } },
				       {.v = {  0.0,  0.0, z[1], 1.0 } } },
				     { {.v = {  0.5,  0.0, z[0], 1.0 } },
				       {.v = {  0.0, -0.5, z[1], 1.0 } },
				       {.v = {  0.0,  0.0, z[1], 1.0 } } },
				     { {.v = { -0.5,  0.0, z[2], 1.0 } },
				       {.v = {  0.0,  0.5, z[2], 1.0 } },
				       {.v = {  0.0,  0.0, z[2], 1.0 } } } };

	ftb1 = (*cs->checkOrientation) (s, sAttrib, transform,
					outputPtr, vPoints[0]);
	ftb2 = (*cs->checkOrientation) (s, sAttrib, transform,
					outputPtr, vPoints[1]);
	ftb3 = (*cs->checkOrientation) (s, sAttrib, transform,
					outputPtr, vPoints[2]);

	return (order == FTB && (ftb1 || ftb2 || ftb3)) ||
	       (order == BTF && (!ftb1 || !ftb2 || !ftb3));
    }
    else
    {
	UNWRAP (rs, cs, shouldPaintViewport);
	rv = (*cs->shouldPaintViewport) (s, sAttrib, transform,
					 outputPtr, order);
	WRAP (rs, cs, shouldPaintViewport, cubereflexShouldPaintViewport);
    }

    return rv;
}

static void
cubereflexPaintTop (CompScreen		  *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform     *transform,
		  CompOutput		  *output,
		  int			  size)
{
    ScreenPaintAttrib sa;
    CompTransform     sTransform;
    int               i, opacity;
    Bool              wasCulled = glIsEnabled (GL_CULL_FACE);

    CUBE_SCREEN (s);
    CUBEREFLEX_SCREEN (s);

    UNWRAP (rs, cs, paintTop);
    (*cs->paintTop) (s, sAttrib, transform, output, size);
    WRAP (rs, cs, paintTop, cubereflexPaintTop);

    if (rs->deform == 0.0 || !cubereflexGetFillCaps (s))
	return;

    for (i = 0; i < CUBEREFLEX_CAP_ELEMENTS * 4; i++)
	rs->capFill[(i * 3) + 1] = 0.5;

    opacity = (cs->desktopOpacity * cubereflexGetTopColor(s)[3]) / 0xffff;
    glColor4us (cubereflexGetTopColor(s)[0],
		cubereflexGetTopColor(s)[1],
		cubereflexGetTopColor(s)[2],
		opacity);

    glPushMatrix ();

    if (cs->desktopOpacity != OPAQUE)
    {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, rs->capFill);

    glDisable (GL_CULL_FACE);

    for (i = 0; i < size; i++)
    {
	sa = *sAttrib;
	sTransform = *transform;
	sa.yRotate += (360.0f / size) * i;

	(*s->applyScreenTransform) (s, &sa, output, &sTransform);

        glLoadMatrixf (sTransform.m);

	glDrawArrays (GL_TRIANGLE_FAN, 0, CUBEREFLEX_CAP_ELEMENTS + 2);
    }
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (wasCulled)
	glEnable (GL_CULL_FACE);

    glPopMatrix ();

    glColor4usv (defaultColor);
}

/*
 * Paint bottom cube face
 */
static void
cubereflexPaintBottom (CompScreen		     *s,
		      const ScreenPaintAttrib *sAttrib,
		      const CompTransform     *transform,
		      CompOutput		     *output,
		      int		     size)
{
    ScreenPaintAttrib sa;
    CompTransform     sTransform;
    int               i, opacity;
    Bool              wasCulled = glIsEnabled (GL_CULL_FACE);

    CUBE_SCREEN (s);
    CUBEREFLEX_SCREEN (s);

    UNWRAP (rs, cs, paintBottom);
    (*cs->paintBottom) (s, sAttrib, transform, output, size);
    WRAP (rs, cs, paintBottom, cubereflexPaintBottom);

    if (rs->deform == 0.0 || !cubereflexGetFillCaps (s))
	return;

    for (i = 0; i < CUBEREFLEX_CAP_ELEMENTS * 4; i++)
	rs->capFill[(i * 3) + 1] = -0.5;

    opacity = (cs->desktopOpacity * cubereflexGetBottomColor(s)[3]) / 0xffff;
    glColor4us (cubereflexGetBottomColor(s)[0],
		cubereflexGetBottomColor(s)[1],
		cubereflexGetBottomColor(s)[2],
		opacity);

    glPushMatrix ();

    if (cs->desktopOpacity != OPAQUE)
    {
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, rs->capFill);

    glDisable (GL_CULL_FACE);

    for (i = 0; i < size; i++)
    {
	sa = *sAttrib;
	sTransform = *transform;
	sa.yRotate += (360.0f / size) * i;

	(*s->applyScreenTransform) (s, &sa, output, &sTransform);

        glLoadMatrixf (sTransform.m);

	glDrawArrays (GL_TRIANGLE_FAN, 0, CUBEREFLEX_CAP_ELEMENTS + 2);
    }
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisable (GL_BLEND);
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (wasCulled)
	glEnable (GL_CULL_FACE);

    glPopMatrix ();

    glColor4usv (defaultColor);
}

static void
cubereflexAddWindowGeometry (CompWindow *w,
			     CompMatrix *matrix,
			     int	nMatrix,
			     Region     region,
			     Region     clip)
{
    CompScreen *s = w->screen;

    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    if (rs->deform > 0.0 && s->desktopWindowCount)
    {
	int         x1, x2, i, oldVCount = w->vCount;
	REGION      reg;
	GLfloat     *v;
	int         offX = 0, offY = 0;
	int         sx1, sx2, sw;

	const float angle = atan (0.5 / cs->distance);
	const float rad = 0.5 / sin (angle);
			
	reg.numRects = 1;
	reg.rects = &reg.extents;

	reg.extents.y1 = region->extents.y1;
	reg.extents.y2 = region->extents.y2;
	
	x1 = (region->extents.x1 / CUBEREFLEX_GRID_SIZE) * CUBEREFLEX_GRID_SIZE;
	x1 = region->extents.x1;
	x2 = MIN (x1 + CUBEREFLEX_GRID_SIZE, region->extents.x2);
	
	UNWRAP (rs, s, addWindowGeometry);
	while (x1 < region->extents.x2)
	{
	    reg.extents.x1 = x1;
	    reg.extents.x2 = x2;

	    XIntersectRegion (region, &reg, rs->tmpRegion);

	    if (!XEmptyRegion (rs->tmpRegion))
	    {
		(*w->screen->addWindowGeometry) (w, matrix, nMatrix,
						 rs->tmpRegion, clip);
	    }

	    x1 = x2;
	    x2 = MIN (x2 + CUBEREFLEX_GRID_SIZE, region->extents.x2);
	}
	WRAP (rs, s, addWindowGeometry, cubereflexAddWindowGeometry);
	
	v  = w->vertices + (w->vertexStride - 3);
	v += w->vertexStride * oldVCount;

	if (!windowOnAllViewports (w))
	{
	    getWindowMovementForOffset (w, s->windowOffsetX,
                                        s->windowOffsetY, &offX, &offY);
	}

	if (cs->moMode == CUBE_MOMODE_ONE)
	{
	    sx1 = 0;
	    sx2 = s->width;
	    sw  = s->width;
	}
	else if (cs->moMode == CUBE_MOMODE_MULTI)
	{
	    sx1 = rs->last->region.extents.x1;
	    sx2 = rs->last->region.extents.x2;
	    sw  = sx2 - sx1;
	}
	else
	{
	    sx1 = s->outputDev[cs->srcOutput].region.extents.x1;
	    sx2 = s->outputDev[cs->srcOutput].region.extents.x2;
	    sw  = sx2 - sx1;
	}
	
	for (i = oldVCount; i < w->vCount; i++)
	{
	    if (v[0] + offX >= sx1 - CUBEREFLEX_GRID_SIZE &&
		v[0] + offX < sx2 + CUBEREFLEX_GRID_SIZE)
	    {
		v[2] = (cos (angle - ((v[0] + offX - sx1) /
		       (float)sw * angle * 2)) * rad) - cs->distance;
		v[2] *= rs->deform;
	    }

	    v += w->vertexStride;
	}
    }
    else
    {
	UNWRAP (rs, s, addWindowGeometry);
	(*w->screen->addWindowGeometry) (w, matrix, nMatrix, region, clip);
	WRAP (rs, s, addWindowGeometry, cubereflexAddWindowGeometry);
    }
}

static void
cubereflexPaintTransformedOutput (CompScreen              *s,
				  const ScreenPaintAttrib *sAttrib,
				  const CompTransform     *transform,
				  Region                  region,
				  CompOutput              *output,
				  unsigned int            mask)
{
    static GLfloat light0Position[] = { -0.5f, 0.5f, -9.0f, 1.0f };
    CompTransform  sTransform = *transform;
    float          x, progress;

    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    if (cubereflexGetCylinder (s) &&
	(cs->rotationState == RotationManual ||
	(cs->rotationState == RotationChange &&
	!cubereflexGetCylinderManualOnly (s)) || rs->deform > 0.0))
    {
	const float angle = atan (0.5 / cs->distance);
	const float rad = 0.5 / sin (angle);
	float       z, *quad;
	int         i;
	
	(*cs->getRotation) (s, &x, &x, &progress);
	rs->deform = progress;

	rs->capFill[0] = 0.0;
	rs->capFill[2] = cs->distance;

	for (i = 0; i <= CUBEREFLEX_CAP_ELEMENTS; i++)
	{
	    x = -0.5 + ((float)i / (float)CUBEREFLEX_CAP_ELEMENTS);

	    z = ((cos (angle - ((x + 0.5) * angle * 2)) * rad) -
		 cs->distance) * rs->deform;


	    quad = &rs->capFill[(i + 1) * 3];
	    quad[0] = x;
	    quad[2] = cs->distance + z;
	}
    }
    else
    {
	rs->deform = 0.0;
    }

    if (cs->invert == 1 && rs->first && cubereflexGetReflection (s))
    {
	rs->first = FALSE;
	rs->reflection = TRUE;

	if (cs->grabIndex)
	{
	    CompTransform rTransform = *transform;

	    matrixTranslate (&rTransform, 0.0, -1.0, 0.0);
	    matrixScale (&rTransform, 1.0, -1.0, 1.0);
	    glCullFace (GL_FRONT);

	    UNWRAP (rs, s, paintTransformedOutput);
	    (*s->paintTransformedOutput) (s, sAttrib, &rTransform,
					  region, output, mask);
	    WRAP (rs, s, paintTransformedOutput,
		  cubereflexPaintTransformedOutput);

	    glCullFace (GL_BACK);
	    drawBasicGround (s);
	}
	else
	{
	    CompTransform rTransform = *transform;
	    CompTransform pTransform;
	    float         angle = 360.0 / ((float) s->hsize * cs->nOutput);
	    float         xRot, vRot, xRotate, xRotate2, vRotate, p;
	    float         rYTrans;
	    CompVector    point  = { .v = { -0.5, -0.5, cs->distance, 1.0 } };
	    CompVector    point2 = { .v = { -0.5,  0.5, cs->distance, 1.0 } };
	    float         deform;

	    (*cs->getRotation) (s, &xRot, &vRot, &p);

	    rs->backVRotate = 0.0;

	    xRotate  = xRot;
	    xRotate2 = xRot;
	    vRotate  = vRot;

	    if (vRotate < 0.0)
		xRotate += 180;

	    vRotate = fmod (fabs (vRotate), 180.0);
	    xRotate = fmod (fabs (xRotate), angle);
	    xRotate2 = fmod (fabs (xRotate2), angle);

	    if (vRotate >= 90.0)
		vRotate = 180.0 - vRotate;

	    if (xRotate >= angle / 2.0)
		xRotate = angle - xRotate;

	    if (xRotate2 >= angle / 2.0)
		xRotate2 = angle - xRotate2;

	    xRotate = (rs->deform * angle * 0.5) + ((1.0 - rs->deform) * xRotate);
	    xRotate2 = (rs->deform * angle * 0.5) + ((1.0 - rs->deform) * xRotate2);


	    matrixGetIdentity (&pTransform);
	    matrixRotate (&pTransform, xRotate, 0.0f, 1.0f, 0.0f);
	    matrixRotate (&pTransform,
			  vRotate, cosf (xRotate * DEG2RAD),
			  0.0f, sinf (xRotate * DEG2RAD));

	    matrixMultiplyVector (&point, &point, &pTransform);

	    matrixGetIdentity (&pTransform);
	    matrixRotate (&pTransform, xRotate2, 0.0f, 1.0f, 0.0f);
	    matrixRotate (&pTransform,
			  vRotate, cosf (xRotate2 * DEG2RAD),
			  0.0f, sinf (xRotate2 * DEG2RAD));

	    matrixMultiplyVector (&point2, &point2, &pTransform);

	    switch (cubereflexGetMode (s)) {
	    case ModeJumpyReflection:
		rs->yTrans     = 0.0;
		rYTrans        = point.y * 2.0;
		break;
	    case ModeDistance:
		rs->yTrans     = 0.0;
		rYTrans        = sqrt (0.5 + (cs->distance * cs->distance)) *
				 -2.0;
		break;
	    default:
		rs->yTrans     = -point.y - 0.5;
		rYTrans        = point.y - 0.5;
		break;
	    }

	    if (!cubereflexGetAutoZoom (s) ||
		((cs->rotationState != RotationManual) &&
		 cubereflexGetZoomManualOnly (s)))
	    {
		rs->zTrans = 0.0;
	    }
	    else
		rs->zTrans = -point2.z + cs->distance;

	    if (cubereflexGetMode (s) == ModeAbove)
		rs->zTrans = 0.0;

	    deform = (sqrt (0.25 + (cs->distance * cs->distance)) -
		     cs->distance) * -rs->deform;

	    rs->zTrans = MIN (rs->zTrans, deform);

	    if (cubereflexGetMode (s) == ModeAbove && rs->vRot > 0.0)
	    {
		rs->backVRotate = rs->vRot;
		rs->yTrans      = 0.0;
		rYTrans         = 0.0;

		matrixGetIdentity (&pTransform);
		(*s->applyScreenTransform) (s, sAttrib, output, &pTransform);
		point.x = point.y = 0.0;
		point.z = -cs->distance;
		point.z += deform;
		point.w = 1.0;
		matrixMultiplyVector (&point, &point, &pTransform);
		
		matrixTranslate (&rTransform, 0.0, 0.0, point.z);
		matrixRotate (&rTransform, rs->vRot, 1.0, 0.0, 0.0);
		matrixScale (&rTransform, 1.0, -1.0, 1.0);
		matrixTranslate (&rTransform, 0.0, 1.0, 0.0);
		matrixTranslate (&rTransform, 0.0, 0.0, -point.z + rs->zTrans);
	    }
	    else
	    {
		matrixTranslate (&rTransform, 0.0, rYTrans, rs->zTrans);
		matrixScale (&rTransform, 1.0, -1.0, 1.0);
	    }

	    glPushMatrix ();
	    glLoadIdentity ();
	    glScalef (1.0, -1.0, 1.0);
	    glLightfv (GL_LIGHT0, GL_POSITION, light0Position);
	    glPopMatrix ();
	    glCullFace (GL_FRONT);

	    UNWRAP (rs, s, paintTransformedOutput);
	    (*s->paintTransformedOutput) (s, sAttrib, &rTransform,
					  region, output, mask);
	    WRAP (rs, s, paintTransformedOutput,
		  cubereflexPaintTransformedOutput);

	    glCullFace (GL_BACK);
	    glPushMatrix ();
	    glLoadIdentity ();
	    glLightfv (GL_LIGHT0, GL_POSITION, light0Position);
	    glPopMatrix ();

	    if (cubereflexGetMode (s) == ModeAbove && rs->vRot > 0.0)
	    {
		int   j;
		float i, c;
		float v = MIN (1.0, rs->vRot / 30.0);
		float col1[4], col2[4];

		glPushMatrix ();

		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLoadIdentity ();
		glTranslatef (0.0, 0.0, -DEFAULT_Z_CAMERA);

		i = cubereflexGetIntensity (s) * 2;
		c = cubereflexGetIntensity (s);

		glBegin (GL_QUADS);
		glColor4f (0.0, 0.0, 0.0,
			   ((1 - v) * MAX (0.0, 1.0 - i)) + (v * c));
		glVertex2f (0.5, v / 2.0);
		glVertex2f (-0.5, v / 2.0);
		glColor4f (0.0, 0.0, 0.0,
			   ((1 - v) * MIN (1.0, 1.0 - (i - 1.0))) + (v * c));
		glVertex2f (-0.5, -0.5);
		glVertex2f (0.5, -0.5);
		glEnd ();

		for (j = 0; j < 4; j++)
		{
		    col1[j] = (1.0 - v) * cubereflexGetGroundColor1 (s) [j] +
			      (v * (cubereflexGetGroundColor1 (s) [j] +
				    cubereflexGetGroundColor2 (s) [j]) * 0.5);
		    col1[j] /= 0xffff;
		    col2[j] = (1.0 - v) * cubereflexGetGroundColor2 (s) [j] +
			      (v * (cubereflexGetGroundColor1 (s) [j] +
				    cubereflexGetGroundColor2 (s) [j]) * 0.5);
		    col2[j] /= 0xffff;
		}

		if (cubereflexGetGroundSize (s) > 0.0)
		{
		    glBegin (GL_QUADS);
		    glColor4fv (col1);
		    glVertex2f (-0.5, -0.5);
		    glVertex2f (0.5, -0.5);
		    glColor4fv (col2);
		    glVertex2f (0.5, -0.5 +
				((1 - v) * cubereflexGetGroundSize (s)) + v);
		    glVertex2f (-0.5, -0.5 +
				((1 - v) * cubereflexGetGroundSize (s)) + v);
		    glEnd ();
		}

		glColor4usv (defaultColor);

		glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glDisable (GL_BLEND);
		glPopMatrix ();
	    }
	    else
		drawBasicGround (s);
	}
	
	memset (cs->capsPainted, 0, sizeof (Bool) * s->nOutputDev);
	rs->reflection = FALSE;
    }

    if (!cubereflexGetReflection (s))
    {
	rs->yTrans = 0.0;
	rs->zTrans = (sqrt (0.25 + (cs->distance * cs->distance)) -
		     cs->distance) * -rs->deform;
    }

    matrixTranslate (&sTransform, 0.0, rs->yTrans, rs->zTrans);

    UNWRAP (rs, s, paintTransformedOutput);
    (*s->paintTransformedOutput) (s, sAttrib, &sTransform,
				  region, output, mask);
    WRAP (rs, s, paintTransformedOutput, cubereflexPaintTransformedOutput);
}

static Bool
cubereflexPaintOutput (CompScreen              *s,
		       const ScreenPaintAttrib *sAttrib,
		       const CompTransform     *transform,
		       Region                  region,
		       CompOutput              *output,
		       unsigned int            mask)
{
    Bool status;

    CUBEREFLEX_SCREEN (s);

    if (rs->last != output)
	rs->first = TRUE;

    rs->last = output;

    UNWRAP (rs, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
    WRAP (rs, s, paintOutput, cubereflexPaintOutput);

    return status;
}

static void
cubereflexDonePaintScreen (CompScreen * s)
{
    CUBEREFLEX_SCREEN (s);

    rs->first      = TRUE;
    rs->yTrans     = 0.0;
    rs->zTrans     = 0.0;

    UNWRAP (rs, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (rs, s, donePaintScreen, cubereflexDonePaintScreen);
}


static Bool
cubereflexInitDisplay (CompPlugin  *p,
		       CompDisplay *d)
{
    CubereflexDisplay *rd;

    if (!checkPluginABI ("core", CORE_ABIVERSION) ||
	!checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    rd = malloc (sizeof (CubereflexDisplay));

    if (!rd)
	return FALSE;

    rd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (rd->screenPrivateIndex < 0)
    {
	free (rd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = rd;

    return TRUE;
}

static void
cubereflexFiniDisplay (CompPlugin  *p,
		       CompDisplay *d)
{
    CUBEREFLEX_DISPLAY (d);

    freeScreenPrivateIndex (d, rd->screenPrivateIndex);
    free (rd);
}

static Bool
cubereflexInitScreen (CompPlugin *p,
		      CompScreen *s)
{
    CubereflexScreen *rs;

    CUBEREFLEX_DISPLAY (s->display);
    CUBE_SCREEN (s);

    rs = malloc (sizeof (CubereflexScreen));

    if (!rs)
	return FALSE;

    s->base.privates[rd->screenPrivateIndex].ptr = rs;

    rs->reflection = FALSE;
    rs->first      = TRUE;
    rs->last       = NULL;
    rs->yTrans     = 0.0;
    rs->zTrans     = 0.0;
    rs->tmpRegion  = XCreateRegion ();
    rs->deform     = 0.0;

    WRAP (rs, s, paintTransformedOutput, cubereflexPaintTransformedOutput);
    WRAP (rs, s, paintOutput, cubereflexPaintOutput);
    WRAP (rs, s, donePaintScreen, cubereflexDonePaintScreen);
    WRAP (rs, s, addWindowGeometry, cubereflexAddWindowGeometry);

    WRAP (rs, cs, clearTargetOutput, cubereflexClearTargetOutput);
    WRAP (rs, cs, getRotation, cubereflexGetRotation);
    WRAP (rs, cs, checkOrientation, cubereflexCheckOrientation);
    WRAP (rs, cs, shouldPaintViewport, cubereflexShouldPaintViewport);
    WRAP (rs, cs, paintTop, cubereflexPaintTop);
    WRAP (rs, cs, paintBottom, cubereflexPaintBottom);

    return TRUE;
}

static void
cubereflexFiniScreen (CompPlugin *p,
		      CompScreen *s)
{
    CUBEREFLEX_SCREEN (s);
    CUBE_SCREEN (s);

    XDestroyRegion (rs->tmpRegion);

    UNWRAP (rs, s, paintTransformedOutput);
    UNWRAP (rs, s, paintOutput);
    UNWRAP (rs, s, donePaintScreen);
    UNWRAP (rs, s, addWindowGeometry);

    UNWRAP (rs, cs, clearTargetOutput);
    UNWRAP (rs, cs, getRotation);
    UNWRAP (rs, cs, checkOrientation);
    UNWRAP (rs, cs, shouldPaintViewport);
    UNWRAP (rs, cs, paintTop);
    UNWRAP (rs, cs, paintBottom);

    free (rs);
}

static Bool
cubereflexInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
cubereflexFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
cubereflexInitObject (CompPlugin *p,
		      CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) cubereflexInitDisplay,
	(InitPluginObjectProc) cubereflexInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
cubereflexFiniObject (CompPlugin *p,
		      CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) cubereflexFiniDisplay,
	(FiniPluginObjectProc) cubereflexFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable cubereflexVTable = {
    "cubereflex",
    0,
    cubereflexInit,
    cubereflexFini,
    cubereflexInitObject,
    cubereflexFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &cubereflexVTable;
}
