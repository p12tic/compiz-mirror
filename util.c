/**
 * Compiz Fusion Freewins plugin
 *
 * util.c
 *
 * Copyright (C) 2007  Rodolfo Granata <warlock.cc@gmail.com>
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
 * Rodolfo Granata <warlock.cc@gmail.com>
 * Sam Spilsbury <smspillaz@gmail.com>
 *
 * Button binding support and Reset added by:
 * enigma_0Z <enigma.0ZA@gmail.com>
 *
 * Most of the input handling here is based on
 * the shelf plugin by
 *        : Kristian Lyngstøl <kristian@bohemians.org>
 *        : Danny Baumann <maniac@opencompositing.org>
 *
 * Description:
 *
 * This plugin allows you to freely transform the texture of a window,
 * whether that be rotation or scaling to make better use of screen space
 * or just as a toy.
 *
 * Todo: 
 *  - Modifier key to rotate on the Z Axis
 *  - Fully implement an input redirection system by
 *    finding an inverse matrix, multiplying by it,
 *    translating to the actual window co-ords and 
 *    XSendEvent() the co-ords to the actual window.
 *  - Code could be cleaner
 *  - Add timestep and speed options to animation
 *  - Add window hover-over info via paintOutput : i.e
 *    - Resize borders
 *    - 'Reset' Button
 *    - 'Scale' Button
 *    - 'Rotate' Button
 */

#include "freewins.h"


/* ------ Utility Functions ---------------------------------------------*/

/* Rotate and project individual vectors */
void FWRotateProjectVector (CompWindow *w, CompVector vector, CompTransform transform,
                                   GLdouble *resultX, GLdouble *resultY, GLdouble *resultZ, Bool report)
{

    //report = TRUE;

    /*if (report)
    fprintf(stderr, "Vector info %f %f %f %f\n", vector.v[0], vector.v[1], vector.v[2], vector.v[3]);*/

    matrixMultiplyVector(&vector, &vector, &transform);

    /*if (report)
    fprintf(stderr, "Multiplied info %f %f %f\n", vector.x, vector.y, vector.z);*/

    GLint viewport[4]; // Viewport
    GLdouble modelview[16]; // Modelview Matrix
    GLdouble projection[16]; // Projection Matrix

    glGetIntegerv (GL_VIEWPORT, viewport);
    glGetDoublev (GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev (GL_PROJECTION_MATRIX, projection);

    gluProject (vector.x, vector.y, vector.z,
                modelview, projection, viewport,
                resultX, resultY, resultZ);

    /*if (report)
    fprintf(stderr, "Projected info %f %f %f\n", *resultX, *resultY, *resultZ);*/

    /* Y must be negated */
    *resultY = w->screen->height - *resultY;
}

/* Transform a co-ordinate by a particular transformation matrix */
void
FWCreateMatrix  (CompWindow *w, CompTransform *mTransform,
                 float angX, float angY, float angZ,
                 float tX, float tY, float tZ,
                 float scX, float scY, float scZ)
{
    /* Create our transformation Matrix */

    matrixScale (mTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
    matrixTranslate(mTransform, 
	    tX, 
	    tY, 0.0);
    matrixRotate (mTransform, angX, 1.0f, 0.0f, 0.0f);
    matrixRotate (mTransform, angY, 0.0f, 1.0f, 0.0f);
    matrixRotate (mTransform, angZ, 0.0f, 0.0f, 1.0f);
    matrixScale(mTransform, scX, 1.0f, 0.0f);
    matrixScale(mTransform, 1.0f, scY, 0.0f);
    matrixTranslate(mTransform, 
            -(tX), 
            -(tY), 0.0f);
}
/*
static float det3(float m00, float m01, float m02,
		 float m10, float m11, float m12,
		 float m20, float m21, float m22){

    float ret = 0.0;

    ret += m00 * m11 * m22 - m21 * m12 * m00;
    ret += m01 * m12 * m20 - m22 * m10 * m01;
    ret += m02 * m10 * m21 - m20 * m11 * m02;

    return ret;
}

static void FWFindInverseMatrix(CompTransform *m, CompTransform *r){
    float *mm = m->m;
    float d, c[16];

    d = mm[0] * det3(mm[5], mm[6], mm[7],
			mm[9], mm[10], mm[11],
			 mm[13], mm[14], mm[15]) -

	  mm[1] * det3(mm[4], mm[6], mm[7],
			mm[8], mm[10], mm[11],
			 mm[12], mm[14], mm[15]) +

	  mm[2] * det3(mm[4], mm[5], mm[7],
			mm[8], mm[9], mm[11],
			 mm[12], mm[13], mm[15]) -

	  mm[3] * det3(mm[4], mm[5], mm[6],
			mm[8], mm[9], mm[10],
			 mm[12], mm[13], mm[14]);

    c[0] = det3(mm[5], mm[6], mm[7],
		  mm[9], mm[10], mm[11],
		  mm[13], mm[14], mm[15]);
    c[1] = -det3(mm[4], mm[6], mm[7],
		  mm[8], mm[10], mm[11],
		  mm[12], mm[14], mm[15]);
    c[2] = det3(mm[4], mm[5], mm[7],
		  mm[8], mm[9], mm[11],
		  mm[12], mm[13], mm[15]);
    c[3] = -det3(mm[4], mm[5], mm[6],
		  mm[8], mm[9], mm[10],
		  mm[12], mm[13], mm[14]);

    c[4] = -det3(mm[1], mm[2], mm[3],
		  mm[9], mm[10], mm[11],
		  mm[13], mm[14], mm[15]);
    c[5] = det3(mm[0], mm[2], mm[3],
		  mm[8], mm[10], mm[11],
		  mm[12], mm[14], mm[15]);
    c[6] = -det3(mm[0], mm[1], mm[3],
		  mm[8], mm[9], mm[11],
		  mm[12], mm[13], mm[15]);
    c[7] = det3(mm[0], mm[1], mm[2],
		  mm[8], mm[9], mm[10],
		  mm[12], mm[13], mm[14]);

    c[8] = det3(mm[1], mm[2], mm[3],
		  mm[5], mm[6], mm[7],
		  mm[13], mm[14], mm[15]);
    c[9] = -det3(mm[0], mm[2], mm[3],
		  mm[4], mm[6], mm[7],
		  mm[12], mm[14], mm[15]);
    c[10] = det3(mm[0], mm[1], mm[3],
		  mm[4], mm[5], mm[7],
		  mm[12], mm[13], mm[15]);
    c[11] = -det3(mm[0], mm[1], mm[2],
		  mm[4], mm[5], mm[6],
		  mm[12], mm[13], mm[14]);

    c[12] = -det3(mm[1], mm[2], mm[3],
		  mm[5], mm[6], mm[7],
		  mm[9], mm[10], mm[11]);
    c[13] = det3(mm[0], mm[2], mm[3],
		  mm[4], mm[6], mm[7],
		  mm[8], mm[10], mm[11]);
    c[14] = -det3(mm[0], mm[1], mm[3],
		  mm[4], mm[5], mm[7],
		  mm[8], mm[9], mm[11]);
    c[15] = det3(mm[0], mm[1], mm[2],
		  mm[4], mm[5], mm[6],
		  mm[8], mm[9], mm[10]);


    r->m[0] = c[0] / d;
    r->m[1] = c[4] / d;
    r->m[2] = c[8] / d;
    r->m[3] = c[12] / d;

    r->m[4] = c[1] / d;
    r->m[5] = c[5] / d;
    r->m[6] = c[9] / d;
    r->m[7] = c[13] / d;

    r->m[8] = c[2] / d;
    r->m[9] = c[6] / d;
    r->m[10] = c[10] / d;
    r->m[11] = c[14] / d;

    r->m[12] = c[3] / d;
    r->m[13] = c[7] / d;
    r->m[14] = c[11] / d;
    r->m[15] = c[15] / d;

    return;
}

*/
/* Create a rect from 4 screen points */
Box FWCreateSizedRect (float xScreen1, float xScreen2, float xScreen3, float xScreen4,
                              float yScreen1, float yScreen2, float yScreen3, float yScreen4)
{
        float leftmost, rightmost, topmost, bottommost;
        Box rect;

        /* Left most point */

        leftmost = xScreen1;

        if (xScreen2 <= leftmost)
            leftmost = xScreen2;

        if (xScreen3 <= leftmost)
            leftmost = xScreen3;

        if (xScreen4 <= leftmost)
            leftmost = xScreen4;

        /* Right most point */

        rightmost = xScreen1;

        if (xScreen2 >= rightmost)
            rightmost = xScreen2;

        if (xScreen3 >= rightmost)
            rightmost = xScreen3;

        if (xScreen4 >= rightmost)
            rightmost = xScreen4;

        /* Top most point */

        topmost = yScreen1;

        if (yScreen2 <= topmost)
            topmost = yScreen2;

        if (yScreen3 <= topmost)
            topmost = yScreen3;

        if (yScreen4 <= topmost)
            topmost = yScreen4;

        /* Bottom most point */

        bottommost = yScreen1;

        if (yScreen2 >= bottommost)
            bottommost = yScreen2;

        if (yScreen3 >= bottommost)
            bottommost = yScreen3;

        if (yScreen4 >= bottommost)
            bottommost = yScreen4;

        rect.x1 = leftmost;
        rect.x2 = rightmost;
        rect.y1 = topmost;
        rect.y2 = bottommost;

        return rect;
}

Box FWCalculateWindowRect (CompWindow *w, CompVector c1, CompVector c2,
                                   CompVector c3, CompVector c4)
{

        FREEWINS_WINDOW (w);

        CompTransform transform;
        GLdouble xScreen1, yScreen1, zScreen1;
        GLdouble xScreen2, yScreen2, zScreen2;
        GLdouble xScreen3, yScreen3, zScreen3;
        GLdouble xScreen4, yScreen4, zScreen4;

        matrixGetIdentity(&transform);
        FWCreateMatrix (w, &transform,
                        fww->transform.angX,
                        fww->transform.angY,
                        fww->transform.angZ,
                        fww->iMidX, fww->iMidY, 0.0f,
                        fww->transform.scaleX,
                        fww->transform.scaleY, 0.0f);  

        /*CompTransform inverse;
        FWFindInverseMatrix (&transform, &inverse);

        CompTransform identify, identity;

        matrixGetIdentity(&identity);

        matrixMultiply(&identify, &transform, &inverse);*/

        /*

        fprintf(stderr, "Inverse Matrix\n\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n %f %f %f %f %f\n\n",
                inverse.m[0],   inverse.m[1],   inverse.m[2],   inverse.m[3],   inverse.m[4],   
                inverse.m[5],   inverse.m[6],   inverse.m[7],   inverse.m[8],   inverse.m[9],   
                inverse.m[10],   inverse.m[11],   inverse.m[12],   inverse.m[13],   inverse.m[14],   
                inverse.m[15],   inverse.m[16]); 

        fprintf(stderr, "Real Identity\n\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n %f %f %f %f %f\n\n",
                identity.m[0],   identity.m[1],   identity.m[2],   identity.m[3],   identity.m[4],   
                identity.m[5],   identity.m[6],   identity.m[7],   identity.m[8],   identity.m[9],   
                identity.m[10],   identity.m[11],   identity.m[12],   identity.m[13],   identity.m[14],   
                identity.m[15],   identity.m[16]); 

        fprintf(stderr, "Supposed Identity\n\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n %f %f %f %f %f\n\n",
                identify.m[0],   identify.m[1],   identify.m[2],   identify.m[3],   identify.m[4],   
                identify.m[5],   identify.m[6],   identify.m[7],   identify.m[8],   identify.m[9],   
                identify.m[10],   identify.m[11],   identify.m[12],   identify.m[13],   identify.m[14],   
                identify.m[15],   identify.m[16]); 

        */

        //CompVector pointer = { .v = { pointerX, pointerY, 1.0f, 1.0f } };

        FWRotateProjectVector(w, c1, transform, &xScreen1, &yScreen1, &zScreen1, FALSE);
        FWRotateProjectVector(w, c2, transform, &xScreen2, &yScreen2, &zScreen2, FALSE);
        FWRotateProjectVector(w, c3, transform, &xScreen3, &yScreen3, &zScreen3, FALSE);
        FWRotateProjectVector(w, c4, transform, &xScreen4, &yScreen4, &zScreen4, FALSE);

        /* TODO: Use inverse matrix to calculate pointer position
                 on original window
        */

        /*

        FWRotateProjectVector(w, pointer, transform, &xScreen5, &yScreen5, &zScreen5, report);

        //fprintf(stderr, "Resultants for pointer are %f %f %f\n", xScreen5, yScreen5, zScreen5);

        FREEWINS_DISPLAY (w->screen->display);

        fwd->transformed_px = (fww->inputRect.x1 - xScreen5) + w->screen->width;
        fwd->transformed_py = (fww->inputRect.y1 - yScreen5) + w->screen->height;*/

        return FWCreateSizedRect(xScreen1, xScreen2, xScreen3, xScreen4,
                                 yScreen1, yScreen2, yScreen3, yScreen4);
           
}

void FWCalculateOutputRect (CompWindow *w)
{
    if (w)
    {

    FREEWINS_WINDOW (w);

    CompVector corner1 = { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner2 = { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner3 = { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };
    CompVector corner4 = { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };

    fww->outputRect = FWCalculateWindowRect (w, corner1, corner2, corner3, corner4);
    }
}

void FWCalculateInputRect (CompWindow *w)
{

    if (w)
    {

    FREEWINS_WINDOW (w);

    CompVector corner1 = { .v = { WIN_REAL_X (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
    CompVector corner2 = { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
    CompVector corner3 = { .v = { WIN_REAL_X (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };
    CompVector corner4 = { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };

    fww->inputRect = FWCalculateWindowRect (w, corner1, corner2, corner3, corner4);
    }

}

void FWCalculateInputOrigin (CompWindow *w, float x, float y)
{

    FREEWINS_WINDOW (w);

    fww->iMidX = x;
    fww->iMidY = y;
}

void FWCalculateOutputOrigin (CompWindow *w, float x, float y)
{

    FREEWINS_WINDOW (w);

    float dx, dy;

    dx = x - WIN_OUTPUT_X (w);
    dy = y - WIN_OUTPUT_Y (w);

    fww->oMidX = WIN_OUTPUT_X (w) + dx * fww->transform.scaleX;
    fww->oMidY = WIN_OUTPUT_Y (w) + dy * fww->transform.scaleY;
}

/* Change angles more than 360 into angles out of 360 */
/*static int FWMakeIntoOutOfThreeSixty (int value)
{
    while (value > 0)
    {
        value -= 360;
    }

    if (value < 0)
        value += 360;

    return value;
}*/

/* Determine if we clicked in the z-axis region */
void FWDetermineZAxisClick (CompWindow *w, int px, int py)
{
    FREEWINS_WINDOW (w);

    Bool directionChange = FALSE;

    if (!fww->can2D)
    {

        static int steps;

        /* Check if we are going in a particular 3D direction
         * because if we are going left/right and we suddenly 
         * change to 2D mode this would not be expected behaviour.
         * It is only if we have a change in direction that we want
         * to change to 2D rotation.
         */

        /* FIXME: Improve Detection */

        Direction direction;

        static int ddx, ddy;

        unsigned int dx = pointerX - lastPointerX;
        unsigned int dy = pointerY - lastPointerY;

        ddx += dx;
        ddy += dy;

        if (steps >= 10)
        {
            if (ddx > ddy)
                direction = LeftRight;
            else
                direction = UpDown;

            if (fww->direction != direction)
                directionChange = TRUE;

            fww->direction = direction;
        }

        steps++;

    }
    else
        directionChange = TRUE;

    if (directionChange)
    {

        float clickRadiusFromCenter;

        int x = (WIN_REAL_X(w) + WIN_REAL_W(w)/2.0) + fww->adjustX;
	    int y = (WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0) + fww->adjustY;

        clickRadiusFromCenter = sqrt(pow((x - px), 2) + pow((y - py), 2));

        if (clickRadiusFromCenter > fww->radius * (freewinsGet3dPercent (w->screen) / 100))
        {
            fww->can2D = TRUE;
            fww->can3D = FALSE;
        }
        else
        {
            fww->can2D = FALSE;
            fww->can3D = TRUE;
        }
    }
}

/* Check to see if we can shape a window */
Bool FWCanShape (CompWindow *w)
{

    if (!freewinsGetShapeInput (w->screen))
    return FALSE;
    
    if (!w->screen->display->shapeExtension)
    return FALSE;
    
    if (!matchEval (freewinsGetShapeWindowTypes (w->screen), w))
    return FALSE;
    
    return TRUE;
}

/* Checks if w is a ipw and returns the real window */
CompWindow *
FWGetRealWindow (CompWindow *w)
{
    FWWindowInputInfo *info;

    FREEWINS_SCREEN (w->screen);

    for (info = fws->transformedWindows; info; info = info->next)
    {
	if (w->id == info->ipw)
	    return info->w;
    }

    return NULL;
}

/*static CompWindow *
FWGetRealWindowFromID (CompDisplay *d,
		       Window      wid)
{
    CompWindow *orig, *w;
    CompScreen *s;

    orig = findWindowAtDisplay (d, wid);

    for (s = d->screens; s; s = s->next)
        for (w = s->windows; w; w = w->next)
            if (wid == w->id)
                orig = w;
    if (!orig)
	return NULL;


    return FWGetRealWindow (orig);
}*/
