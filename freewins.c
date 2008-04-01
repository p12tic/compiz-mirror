/*
 * Compiz Fusion Freewins plugin
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
 *
 * Button binding support and Reset added by:
 * enigma_0Z <enigma.0ZA@gmail.com>
 *
 * Scaling, Animation, Input-Shaping, Snapping
 * and Key-Based Transformation added by:
 * Sam Spilsbury <smspillaz@gmail.com>
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
 *  - Code could be cleaner
 *  - Add timestep and speed options to animation
 *  - Activate the move action in move to move the window
 *  - Add window hover-over info via paintOutput : i.e
 *    - Resize borders
 *    - 'Reset' Button
 *    - 'Scale' Button
 *    - 'Rotate' Button
 */

#include <compiz-core.h>
#include <math.h>

#include <stdio.h>

#include <X11/cursorfont.h>
#include <X11/extensions/shape.h>

#include <GL/glu.h>
#include <GL/gl.h>

#include "freewins_options.h"

#define ABS(x) ((x)>0?(x):-(x))
#define D2R(x) ((x) * (M_PI / 180.0))
#define R2D(x) ((x) * (180 / M_PI))

/* ------ Macros ---------------------------------------------------------*/
#define GET_FREEWINS_DISPLAY(d)                                       \
    ((FWDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define FREEWINS_DISPLAY(d)                      \
    FWDisplay *fwd = GET_FREEWINS_DISPLAY (d)

#define GET_FREEWINS_SCREEN(s, fwd)                                        \
    ((FWScreen *) (s)->base.privates[(fwd)->screenPrivateIndex].ptr)

#define FREEWINS_SCREEN(s)                                                      \
    FWScreen *fws = GET_FREEWINS_SCREEN (s, GET_FREEWINS_DISPLAY (s->display))

#define GET_FREEWINS_WINDOW(w, fws)                                        \
    ((FWWindow *) (w)->base.privates[(fws)->windowPrivateIndex].ptr)

#define FREEWINS_WINDOW(w)                                         \
    FWWindow *fww = GET_FREEWINS_WINDOW  (w,                    \
                       GET_FREEWINS_SCREEN  (w->screen,            \
                       GET_FREEWINS_DISPLAY (w->screen->display)))



#define WIN_OUTPUT_X(w) (w->attrib.x - w->output.left)
#define WIN_OUTPUT_Y(w) (w->attrib.y - w->output.top)

#define WIN_OUTPUT_W(w) (w->width + w->output.left + w->output.right)
#define WIN_OUTPUT_H(w) (w->height + w->output.top + w->output.bottom)

#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/* ------ Structures and Enums ------------------------------------------*/

/* Enums */
typedef enum _StartCorner {
    CornerTopLeft = 0,
    CornerTopRight = 1,
    CornerBottomLeft = 2,
    CornerBottomRight = 3
} StartCorner;

typedef enum _FWGrabType {
    grabNone = 0,
    grabRotate,
    grabScale,
    grabMove,
    grabResize
} FWGrabType;
    

/* Shape info / restoration */
typedef struct _FWWindowInputInfo {
    CompWindow                *w;
    struct _FWWindowInputInfo *next;

    Window     ipw;

    XRectangle *inputRects;
    int        nInputRects;
    int        inputRectOrdering;

    XRectangle *frameInputRects;
    int        frameNInputRects;
    int        frameInputRectOrdering;
} FWWindowInputInfo;

/* Transformation info */
typedef struct _FWTransformedWindowInfo
{
    float angX;
    float angY;
    float angZ;
    
    float scaleY;
    float scaleX;

    /* Used for snapping */

    float unsnapAngX;
    float unsnapAngY;
    float unsnapAngZ;

    float unsnapScaleX;
    float unsnapScaleY;

} FWTransformedWindowInfo;

typedef struct _FWAnimationInfo
{
    // Old values to animate from
    float oldAngX;
    float oldAngY;
    float oldAngZ;
    
    float oldScaleX;
    float oldScaleY;
    
    // New values to animate to
    float destAngX;
    float destAngY;
    float destAngZ;
    
    float destScaleX;
    float destScaleY;

    // For animation
    float aTimeRemaining; // Actual time remaining (is decremented)
    float cTimeRemaining; // Constant time remaining (is referenced and not decremented)
    float steps;
} FWAnimationInfo;

/* Freewins Display Structure */
typedef struct _FWDisplay{
    int screenPrivateIndex;

    int click_root_x;
    int click_root_y;

    int click_win_x;
    int click_win_y;

    // Used for determining cursor direction
    int oldX;
    int oldY;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
    CompWindow *focusWindow;

    FWGrabType grab;
    
    Bool axisHelp;

} FWDisplay;

/* Freewins Screen Structure */
typedef struct _FWScreen{
    int windowPrivateIndex;

    PreparePaintScreenProc preparePaintScreen;
    PaintOutputProc paintOutput;
    PaintWindowProc paintWindow;

    DamageWindowRectProc damageWindowRect;

    WindowResizeNotifyProc windowResizeNotify;
    WindowMoveNotifyProc   windowMoveNotify;

    FWWindowInputInfo *transformedWindows;
    
    Cursor rotateCursor;

    int grabIndex;
    int rotatedWindows;

} FWScreen;

/* Freewins Window Structure */
typedef struct _FWWindow{

    float midX;
    float midY;
    
    // Used for determining window movement

    int oldWinX;
    int oldWinY;
    
    // Used to determine starting point
    StartCorner corner;

    // Transformation info
    FWTransformedWindowInfo transform;

    // Animation Info
    FWAnimationInfo animate;

    // Input Info
    FWWindowInputInfo *input;

    Box outputRect;
    Box inputRect;

    // Used to determine whether to animate the window
    Bool doAnimate;
    Bool resetting;
    
    // Used to determine whether rotating on X and Y axis, or just on Z
    Bool zaxis;

    Bool grabLeft;
    Bool grabTop;

    Bool rotated;
    Bool allowScaling;
    Bool allowRotation;

} FWWindow;

int displayPrivateIndex;
static CompMetadata freewinsMetadata;

/* ------ Utility Functions ---------------------------------------------*/

/* Rotate and project individual vectors */
static void FWRotateProjectVector (CompWindow *w, CompVector vector, CompTransform transform,
                                   GLdouble *resultX, GLdouble *resultY, GLdouble *resultZ)
{
    matrixMultiplyVector(&vector, &vector, &transform);

    GLint viewport[4]; // Viewport
    GLdouble modelview[16]; // Modelview Matrix
    GLdouble projection[16]; // Projection Matrix

    glGetIntegerv (GL_VIEWPORT, viewport);
    glGetDoublev (GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev (GL_PROJECTION_MATRIX, projection);

    gluProject (vector.x, vector.y, vector.z,
                modelview, projection, viewport,
                resultX, resultY, resultZ);

    /* Y must be negated */
    *resultY = w->screen->height - *resultY;
}

/* Create a rect from 4 screen points */
static Box FWCreateSizedRect (float xScreen1, float xScreen2, float xScreen3, float xScreen4,
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

/* Check to see if we can shape a window */
static Bool FWCanShape (CompWindow *w)
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
static CompWindow *
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

static CompWindow *
FWGetRealWindowFromID (CompDisplay *d,
		       Window      wid)
{
    CompWindow *orig;

    orig = findWindowAtDisplay (d, wid);
    if (!orig)
	return NULL;

    return FWGetRealWindow (orig);
}

/* ------ Input Prevention -------------------------------------------*/

static void
FWSaveInputShape (CompWindow *w,
		     XRectangle **retRects,
		     int        *retCount,
		     int        *retOrdering)
{
    XRectangle *rects;
    int        count = 0, ordering;
    Display    *dpy = w->screen->display->display;

    rects = XShapeGetRectangles (dpy, w->id, ShapeInput, &count, &ordering);

    /* check if the returned shape exactly matches the window shape -
       if that is true, the window currently has no set input shape */
    if ((count == 1) &&
	(rects[0].x == -w->serverBorderWidth) &&
	(rects[0].y == -w->serverBorderWidth) &&
	(rects[0].width == (w->serverWidth + w->serverBorderWidth)) &&
	(rects[0].height == (w->serverHeight + w->serverBorderWidth)))
    {
	count = 0;
    }
    
    *retRects    = rects;
    *retCount    = count;
    *retOrdering = ordering;
}

static void
FWUnshapeInput (CompWindow *w)
{
    Display *dpy = w->screen->display->display;

    FREEWINS_WINDOW (w);

    if (fww->input->nInputRects)
    {
	XShapeCombineRectangles (dpy, w->id, ShapeInput, 0, 0,
				 fww->input->inputRects, fww->input->nInputRects,
				 ShapeSet, fww->input->inputRectOrdering);
    }
    else
    {
	XShapeCombineMask (dpy, w->id, ShapeInput, 0, 0, None, ShapeSet);
    }

    if (fww->input->frameNInputRects >= 0)
    {
	if (fww->input->frameInputRects)
	{
	    XShapeCombineRectangles (dpy, w->frame, ShapeInput, 0, 0,
				     fww->input->frameInputRects,
				     fww->input->frameNInputRects,
				     ShapeSet,
				     fww->input->frameInputRectOrdering);
	}
	else
	{
	    XShapeCombineMask (dpy, w->frame, ShapeInput, 0, 0, None, ShapeSet);
	}
    }
}


/* Input Shaper. This no longer adjusts the shape of the window
   but instead shapes it to 0 as the IPW deals with the input.
   Old code is available commented, but will be removed as soon
   as input prevention becomes stable enough  */
static void FWShapeInput (CompWindow *w)
{
    /*FREEWINS_WINDOW(w);
    XRectangle Rectangle;
    float ScaleX;
    float ScaleY;
    
    ScaleX = fww->transform.scaleX;
    ScaleY = fww->transform.scaleY;

    float widthScale = (float) (fww->inputRect.x2 - fww->inputRect.x1) / (float) WIN_OUTPUT_W (w); 
    float heightScale = (float) (fww->inputRect.y2 - fww->inputRect.y1) / (float) WIN_OUTPUT_H (w); 

    ScaleX = widthScale - (1 - ScaleX);
    ScaleY = heightScale - (1 - ScaleY);

    Rectangle.x = (int)(0 + ((1 - ScaleX) / 2) * w->width);
    Rectangle.y = (int)(0 + ((1 - ScaleY) / 2) * w->height);
    Rectangle.width = (int) (w->serverWidth * ((ScaleX)));
    Rectangle.height = (int) (w->serverHeight * ((ScaleY)));
    
    XShapeSelectInput (w->screen->display->display, w->id, NoEventMask);
    XShapeCombineRectangles  (w->screen->display->display, w->id, 
			      ShapeInput, 0, 0, &Rectangle, 1,  ShapeSet, 0);
	if (w->frame)
	{
	    XShapeCombineRectangles  (w->screen->display->display, w->frame, 
			      ShapeInput, 0, 0, &Rectangle, 1,  ShapeSet, 0);
	    XShapeSelectInput (w->screen->display->display, w->id, ShapeNotify);
	}*/

    CompWindow *fw;
    Display    *dpy = w->screen->display->display;

    FREEWINS_WINDOW (w);

    /* save old shape */
    FWSaveInputShape (w, &fww->input->inputRects,
			 &fww->input->nInputRects, &fww->input->inputRectOrdering);

    fw = findWindowAtDisplay (w->screen->display, w->frame);
    if (fw)
    {
	FWSaveInputShape(fw, &fww->input->frameInputRects,
			    &fww->input->frameNInputRects,
			    &fww->input->frameInputRectOrdering);
    }
    else
    {
	fww->input->frameInputRects        = NULL;
	fww->input->frameNInputRects       = -1;
	fww->input->frameInputRectOrdering = 0;
    }

    /* clear shape */
    XShapeSelectInput (dpy, w->id, NoEventMask);
    XShapeCombineRectangles  (dpy, w->id, ShapeInput, 0, 0,
			      NULL, 0, ShapeSet, 0);
    
    if (w->frame)
	XShapeCombineRectangles  (dpy, w->frame, ShapeInput, 0, 0,
				  NULL, 0, ShapeSet, 0);

    XShapeSelectInput (dpy, w->id, ShapeNotify);
}

/* Add the input info to the list of input info */
static void
FWAddWindowToList (FWWindowInputInfo *info)
{
    CompScreen        *s = info->w->screen;
    FWWindowInputInfo *run;

    FREEWINS_SCREEN (s);

    run = fws->transformedWindows;
    if (!run)
	fws->transformedWindows = info;
    else
    {
	for (; run->next; run = run->next)
	run->next = info;
    }
}

/* Remove the input info from the list of input info */
static void
FWRemoveWindowFromList (FWWindowInputInfo *info)
{
    CompScreen        *s = info->w->screen;
    FWWindowInputInfo *run;

    FREEWINS_SCREEN (s);

    if (!fws->transformedWindows)
	return;

    if (fws->transformedWindows == info)
	fws->transformedWindows = info->next;
    else
    {
	for (run = fws->transformedWindows; run->next; run = run->next)
	{
	    if (run->next == info)
	    {
		run->next = info->next;
		break;
	    }
	}
    }
}

/* Create an input prevention window */
static void
FWCreateIPW (CompWindow *w)
{
    Window               ipw;
    XSetWindowAttributes attrib;

    FREEWINS_WINDOW (w);

    if (!fww->input || fww->input->ipw)
	return;

    attrib.override_redirect = TRUE;
    attrib.event_mask        = 0;

    ipw = XCreateWindow (w->screen->display->display,
			 w->screen->root,
			 w->serverX - w->input.left,
			 w->serverY - w->input.top,
			 w->serverWidth + w->input.left + w->input.right,
			 w->serverHeight + w->input.top + w->input.bottom,
			 0, CopyFromParent, InputOnly, CopyFromParent,
			 CWEventMask | CWOverrideRedirect,
			 &attrib);
 
    fww->input->ipw = ipw;
}

/* Adjust size and location of the input prevention window
 */
static void 
FWAdjustIPW (CompWindow *w)
{
    XWindowChanges xwc;
    Display        *dpy = w->screen->display->display;
    float          width, height;

    FREEWINS_WINDOW (w);

    if (!fww->input || !fww->input->ipw)
	return;

    width  = fww->inputRect.x2 - fww->inputRect.x1;
    height = fww->inputRect.y2 - fww->inputRect.y1;

    xwc.x          = (fww->inputRect.x1);
    xwc.y          = (fww->inputRect.y1);
    xwc.width      = ceil(width);
    xwc.height     = ceil(height);
    xwc.stack_mode = Below;
    xwc.sibling    = w->id;

    XConfigureWindow (dpy, fww->input->ipw,
		      CWSibling | CWStackMode | CWX | CWY | CWWidth | CWHeight,
		      &xwc);

    XMapWindow (dpy, fww->input->ipw);
}

/* Ensure that the input prevention window
 * is always on top of the transformed 
 * window
 */

static void
FWAdjustIPWStacking (CompScreen *s)
{
    FWWindowInputInfo *input;

    FREEWINS_SCREEN (s);

    for (input = fws->transformedWindows; input; input = input->next)
    {
	if (!input->w->prev || input->w->prev->id != input->ipw)
	    FWAdjustIPW (input->w);
    }
}

static void FWHandleIPWButtonPress (CompWindow *w)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    (*w->screen->activateWindow) (w);
    fwd->grab = grabMove;
    fws->rotateCursor = XCreateFontCursor (w->screen->display->display, XC_fleur);	
	if(!otherScreenGrabExist(w->screen, "freewins", "move", 0))
	    if(!fws->grabIndex)
        {
        unsigned int mods;
        mods &= CompNoMask;
		fws->grabIndex = pushScreenGrab(w->screen, fws->rotateCursor, "move");
	    (w->screen->windowGrabNotify) (w,  w->attrib.x + (w->width / 2),
                                           w->attrib.y + (w->height / 2), mods,
					                       CompWindowGrabMoveMask |
					                       CompWindowGrabButtonMask);
        }
    fwd->grabWindow = w;
}

static void FWHandleMotionEvent (CompWindow *w, unsigned int x, unsigned int y)
{
    FREEWINS_SCREEN (w->screen);

    //static int oldPointerX, oldPointerY;

    int dx = x - lastPointerX;
    int dy = y - lastPointerY;

    if (!fws->grabIndex)
        return;


    moveWindow(w, dx,
                  dy, TRUE, FALSE);
    syncWindowPosition (w);
}

static void FWHandleButtonReleaseEvent (CompWindow *w)
{
    FREEWINS_SCREEN (w->screen);
    FREEWINS_DISPLAY (w->screen->display);

    if (fwd->grab == grabMove)
    {
        removeScreenGrab (w->screen, fws->grabIndex, NULL);
        (w->screen->windowUngrabNotify) (w);
        moveInputFocusToWindow (w);
        fws->grabIndex = 0;
        fwd->grabWindow = NULL;
        fwd->grab = grabNone;
    }
}
/* This should be called instead of FWShapeInput or FWCreateIPW
 * as it does all the setup beforehand.
 */
static Bool
FWHandleFWInputInfo (CompWindow *w)
{
    FREEWINS_WINDOW (w);

    if (!fww->rotated && fww->input)
    {
	if (fww->input->ipw)
	    XDestroyWindow (w->screen->display->display, fww->input->ipw);

	FWUnshapeInput (w);
	FWRemoveWindowFromList (fww->input);

	free (fww->input);
	fww->input = NULL;

	return FALSE;
    }
    else if (fww->rotated && !fww->input)
    {
	fww->input = calloc (1, sizeof (FWWindowInputInfo));
	if (!fww->input)
	    return FALSE;

	fww->input->w = w;
	FWShapeInput (w);
	FWCreateIPW (w);
	FWAddWindowToList (fww->input);
    }

    return TRUE;
}

/* ------ Wrappable Functions -------------------------------------------*/

/* X Event Handler */
static void FWHandleEvent(CompDisplay *d, XEvent *ev){

    float dx, dy;
    CompWindow *oldPrev, *oldNext, *w;
    FREEWINS_DISPLAY(d);

    switch(ev->type){
	
	/* Motion Notify Event */
	case MotionNotify:
	    
	    if(fwd->grab != grabNone)
        {
		    FREEWINS_WINDOW(fwd->grabWindow);

		    dx = (float)(ev->xmotion.x_root - fwd->oldX) / fwd->grabWindow->screen->width;
		    dy = (float)(ev->xmotion.y_root - fwd->oldY) / fwd->grabWindow->screen->height;


            if (fwd->grab == grabMove)
            {
                FREEWINS_SCREEN (fwd->grabWindow->screen);
                FWWindowInputInfo *info;
                CompWindow *w = fwd->grabWindow;
                for (info = fws->transformedWindows; info; info = info->next)
                {
                    if (fwd->grabWindow->id == info->ipw)
                    /* The window we just grabbed was actually
                     * an IPW, get the real window instead
                     */
                    w = FWGetRealWindow (fwd->grabWindow);
                }
                FWHandleMotionEvent (w, pointerX, pointerY);
                fww->allowRotation = FALSE;
                fww->allowScaling = FALSE;
            }

            if (fwd->grab == grabRotate)
            {        
                if(fww->zaxis)
                {
		            if(ABS(dy)>ABS(dx))
		            {
			            if(fww->grabLeft)
			            {
			                fww->transform.angZ -= 360 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			                fww->transform.unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			            }
			            else
			            {
			                fww->transform.angZ += 360 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			                fww->transform.unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			            }
		                }
		                else
		                {
			            if(fww->grabTop)
			            {
			                fww->transform.angZ += 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			                fww->transform.unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			            }
			            else
		                {
			                fww->transform.angZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			                fww->transform.unsnapAngZ -= 360 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
			            }
		            }
		        }
		        else
		        {
		            fww->transform.angX -= 360.0 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            fww->transform.unsnapAngX -= 360.0 * (dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            fww->transform.angY += 360.0 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		            fww->transform.unsnapAngY += 360.0 * (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		        }
		    }
		    if (fwd->grab == grabScale)
		    {
		        switch (fww->corner)
		        {
		        
		        case CornerTopLeft:
		        		        
		                // Check X Direction
		                if ((ev->xmotion.x - 100.0) < fwd->oldX)
		                {
		                    fww->transform.scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                else if ((ev->xmotion.x - 100.0) > fwd->oldX)
		                {
		                    fww->transform.scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                
		                // Check Y Direction
		                if ((ev->xmotion.y - 100.0) < fwd->oldY)
		                {
		                    fww->transform.scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                else if ((ev->xmotion.y - 100.0) > fwd->oldY)
		                {
		                    fww->transform.scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                break;            
		        		            
                case CornerTopRight:
                 
		                // Check X Direction
		                if ((ev->xmotion.x - 100.0) < fwd->oldX)
		                {
		                    fww->transform.scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                else if ((ev->xmotion.x - 100.0) > fwd->oldX)
		                {
		                    fww->transform.scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                
		                
		                // Check Y Direction
		                if ((ev->xmotion.y - 100.0) < fwd->oldY)
		                {
		                    fww->transform.scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                else if ((ev->xmotion.y - 100.0) > fwd->oldY)
		                {
		                    fww->transform.scaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY -= dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                    
		                break;
		                
		         case CornerBottomLeft:
		            
		                // Check X Direction
		                if ((ev->xmotion.x - 100.0) < fwd->oldX)
		                {
		                    fww->transform.scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                else if ((ev->xmotion.x - 100.0) > fwd->oldX)
		                {
		                    fww->transform.scaleX -= dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX -= (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                
		                // Check Y Direction
		                if ((ev->xmotion.y - 100.0) < fwd->oldY)
		                {
		                    fww->transform.scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                else if ((ev->xmotion.y - 100.0) > fwd->oldY)
		                {
		                    fww->transform.scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                    
		                break;
		                
		         case CornerBottomRight:
		            
		                // Check X Direction
		                if ((ev->xmotion.x - 100.0) < fwd->oldX)
		                {
		                    fww->transform.scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                else if ((ev->xmotion.x - 100.0) > fwd->oldX)
		                {
		                    fww->transform.scaleX += dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleX += (dx * freewinsGetMouseSensitivity (fwd->grabWindow->screen));
		                }
		                
		                // Check Y Direction
		                if ((ev->xmotion.y - 100.0) < fwd->oldY)
		                {
		                    fww->transform.scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }
		                else if ((ev->xmotion.y - 100.0) > fwd->oldY)
		                {
		                    fww->transform.scaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                    fww->transform.unsnapScaleY += dy * freewinsGetMouseSensitivity (fwd->grabWindow->screen);
		                }		                    
		                break;
		         }
		      
		    }
		
            fwd->oldX += (ev->xmotion.x_root - fwd->oldX);
		    fwd->oldY += (ev->xmotion.y_root - fwd->oldY);

		    fww->grabLeft = (ev->xmotion.x - fww->midX > 0 ? FALSE : TRUE);
		    fww->grabTop = (ev->xmotion.y - fww->midY > 0 ? FALSE : TRUE);

            /* Stop scale at threshold specified */
            if (!freewinsGetAllowNegative (fwd->grabWindow->screen) || (FWCanShape (fwd->grabWindow)))
            {
                float minScale = freewinsGetMinScale (fwd->grabWindow->screen);
	            if (fww->transform.scaleX < minScale)
	              fww->transform.scaleX = minScale; // Don't allow negative values either, which could be referenced
	
	            if (fww->transform.scaleY < minScale)
	              fww->transform.scaleY = minScale;
            }

            /* Change scales for maintaining aspect ratio */
            if (freewinsGetScaleUniform (fwd->grabWindow->screen))
            {
                float tempscaleX = fww->transform.scaleX;
                float tempscaleY = fww->transform.scaleY;
                fww->transform.scaleX = (tempscaleX + tempscaleY) / 2;
                fww->transform.scaleY = (tempscaleX + tempscaleY) / 2;
                fww->transform.unsnapScaleX = (tempscaleX + tempscaleY) / 2;
                fww->transform.unsnapScaleY = (tempscaleX + tempscaleY) / 2;
            }

	        /* Handle Snapping */
	        if (freewinsGetSnap (fwd->grabWindow->screen))
	        {
	            int snapFactor = freewinsGetSnapThreshold (fwd->grabWindow->screen);
                fww->transform.angX = ((int) (fww->transform.unsnapAngX) / snapFactor) * snapFactor;
                fww->transform.angY = ((int) (fww->transform.unsnapAngY) / snapFactor) * snapFactor;
                fww->transform.angZ = ((int) (fww->transform.unsnapAngZ) / snapFactor) * snapFactor;
                fww->transform.scaleX = ((float) ( (int) (fww->transform.unsnapScaleX * (21 - snapFactor) + 0.5))) / (21 - snapFactor); 
                fww->transform.scaleY = ((float) ( (int) (fww->transform.unsnapScaleY * (21 - snapFactor) + 0.5))) / (21 - snapFactor);
            }
	
	        fww->animate.oldAngX = fww->transform.angX;
	        fww->animate.oldAngY = fww->transform.angY;
	        fww->animate.oldAngZ = fww->transform.angZ;
	
	        fww->animate.oldScaleX = fww->transform.scaleX;
	        fww->animate.oldScaleY = fww->transform.scaleY;

	        if(dx != 0.0 || dy != 0.0)
                addWindowDamage (fwd->grabWindow);

        }
    break;


	/* Button Press and Release */
	case ButtonPress:
    {
        CompWindow *btnW;
        btnW = FWGetRealWindowFromID (d, ev->xbutton.window);
        if (btnW)
            if (fwd->grab == grabNone)
                FWHandleIPWButtonPress (btnW);

        fwd->oldX =  ev->xbutton.x_root;
	    fwd->oldY =  ev->xbutton.y_root;
	    fwd->click_root_x = ev->xbutton.x_root;
	    fwd->click_root_y = ev->xbutton.y_root;
	    fwd->click_win_x = ev->xbutton.x;
	    fwd->click_win_y = ev->xbutton.y;       

	    break;
    }
	case ButtonRelease:
    {
        if (fwd->grabWindow)
        {
        FREEWINS_WINDOW (fwd->grabWindow);
        if (fwd->grab == grabMove)
            FWHandleButtonReleaseEvent (fwd->grabWindow);

	    if((fwd->grab == grabScale) || (fwd->grab == grabRotate)){
        CompScreen *s;
	    if (FWCanShape (fwd->grabWindow))
	        if (FWHandleFWInputInfo (fwd->grabWindow))
	            FWAdjustIPW (fwd->grabWindow);
		for(s = d->screens; s; s = s->next){
		    FREEWINS_SCREEN(s);

		    if(fws->grabIndex){
			removeScreenGrab(s, fws->grabIndex, 0);
			fws->grabIndex = 0;
		    }
		}

        fwd->grab = grabNone;
		fwd->grabWindow = 0;
        fww->rotated = TRUE;
	    }
        }
	    break;
    }

	case FocusOut:
	    break;

	case FocusIn:
	    if(ev->xfocus.mode != NotifyGrab)
		fwd->focusWindow = findWindowAtDisplay(d, ev->xfocus.window);
	    break;

	case ConfigureNotify:
	    w = findWindowAtDisplay (d, ev->xconfigure.window);
	    if (w)
	    {
		oldPrev = w->prev;
		oldNext = w->next;
	    }
	    break;

    default:
	    if (ev->type == d->shapeEvent + ShapeNotify)
	    {
	        XShapeEvent *se = (XShapeEvent *) ev;
	        if (se->kind == ShapeInput)
	        {
		        CompWindow *w;
		        w = findWindowAtDisplay (d, se->window);
		        if (w)
		        {
		            FREEWINS_WINDOW (w);

		            if (FWCanShape (w) && (fww->transform.scaleX != 1.0f || fww->transform.scaleY != 1.0f))
		            {
		                // Reset the window back to normal
		                fww->transform.scaleX = 1.0f;
		                fww->transform.scaleY = 1.0f;
                        fww->transform.angX = 0.0f;
                        fww->transform.angY = 0.0f;
                        fww->transform.angZ = 0.0f;
			            /*FWShapeInput (w); - Disabled due to problems it causes*/ 
			        }
		        }
	        }
	    }
    }

    UNWRAP(fwd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(fwd, d, handleEvent, FWHandleEvent);

    /* Now we can find out if a restacking occurred while we were handing events */

    switch (ev->type)
    {
	case ConfigureNotify:
	    if (w) /* already assigned above */
	    {
		if (w->prev != oldPrev || w->next != oldNext)
		{
		    /* restacking occured, ensure ipw stacking */
		    FWAdjustIPWStacking (w->screen);
		}
	    }
	    break;
    }
}

/* Animation Prep */
static void
FWPreparePaintScreen (CompScreen *s,
			 int	        ms)
{
    CompWindow *w;
    FREEWINS_SCREEN (s);

    /* FIXME: should only loop over all windows if at least one animation
       is running */
    for (w = s->windows; w; w = w->next)
    {
        FREEWINS_WINDOW (w);
        if (fww->doAnimate)
        {
	        fww->animate.steps = (float)ms / (float)fww->animate.cTimeRemaining;

            if (fww->animate.steps < 0.005)
	            fww->animate.steps = 0.005;
        }
    }
    
    UNWRAP (fws, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (fws, s, preparePaintScreen, FWPreparePaintScreen);
}

/* Paint the window rotated or scaled */
static Bool FWPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){

    CompTransform wTransform = *transform;
    CompTransform outTransform = wTransform;
    CompTransform inputTransform = wTransform;

    CompVector corner1 = { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner2 = { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w), 1.0f, 1.0f } };
    CompVector corner3 = { .v = { WIN_OUTPUT_X (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };
    CompVector corner4 = { .v = { WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w), WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w), 1.0f, 1.0f } };

    GLdouble xScreen1, yScreen1, zScreen1;
    GLdouble xScreen2, yScreen2, zScreen2;
    GLdouble xScreen3, yScreen3, zScreen3;
    GLdouble xScreen4, yScreen4, zScreen4;

    Bool wasCulled = glIsEnabled(GL_CULL_FACE);
    Bool status;

    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);
        
    /* Has something happened? */

    if((fww->transform.angX != 0.0 || fww->transform.angY != 0.0 || fww->transform.angZ != 0.0 ||
        fww->transform.scaleX != 1.0 || fww->transform.scaleY != 1.0 || fww->oldWinX != WIN_REAL_X (w) ||
        fww->oldWinY != WIN_REAL_Y (w)) && !(w->type == CompWindowTypeDesktopMask))
    {

        fww->oldWinX = WIN_REAL_X (w);
        fww->oldWinY = WIN_REAL_Y (w);

        /* Here we duplicate some of the work the openGL does
         * but for different reasons. We have access to the 
         * window's transformation matrix, so we will create
         * our own matrix and apply the same transformations
         * to it. From there, we create vectors for each point
         * that we wish to track and multiply them by this 
         * matrix to give us the rotated / scaled co-ordinates.
         * From there, we project these co-ordinates onto the flat
         * screen that we have using the OGL viewport, projection
         * matrix and model matrix. Projection gives us three
         * co-ordinates, but we ignore Z and just use X and Y
         * to store in a surrounding rectangle. We can use this
         * surrounding rectangle to make things like shaping and
         * damage a lot more accurate than they used to be.
         */

        matrixGetIdentity (&outTransform);
        matrixScale (&outTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
        matrixTranslate(&outTransform, 
            WIN_OUTPUT_X(w) + WIN_OUTPUT_W(w)/2.0, 
            WIN_OUTPUT_Y(w) + WIN_OUTPUT_H(w)/2.0, 0.0);
        matrixRotate (&outTransform, fww->transform.angX, 1.0f, 0.0f, 0.0f);
        matrixRotate (&outTransform, fww->transform.angY, 0.0f, 1.0f, 0.0f);
        matrixRotate (&outTransform, fww->transform.angZ, 0.0f, 0.0f, 1.0f);
        matrixScale(&outTransform, fww->transform.scaleX, 1.0, 0.0);
        matrixScale(&outTransform, 1.0, fww->transform.scaleY, 0.0);
        matrixTranslate(&outTransform, 
            -(WIN_OUTPUT_X(w) + WIN_OUTPUT_W(w)/2.0), 
            -(WIN_OUTPUT_Y(w) + WIN_OUTPUT_H(w)/2.0), 0.0);

        FWRotateProjectVector(w, corner1, outTransform, &xScreen1, &yScreen1, &zScreen1);
        FWRotateProjectVector(w, corner2, outTransform, &xScreen2, &yScreen2, &zScreen2);
        FWRotateProjectVector(w, corner3, outTransform, &xScreen3, &yScreen3, &zScreen3);
        FWRotateProjectVector(w, corner4, outTransform, &xScreen4, &yScreen4, &zScreen4);

        fww->outputRect = FWCreateSizedRect(xScreen1, xScreen2, xScreen3, xScreen4,
                                            yScreen1, yScreen2, yScreen3, yScreen4);

        /* Prepare for transformation by doing
         * any neccesary adjustments
         */

        float autoScaleX = 1.0f;
        float autoScaleY = 1.0f;

        if (freewinsGetAutoZoom (w->screen))
        {

            float apparantWidth = fww->outputRect.x2 - fww->outputRect.x1;
            float apparantHeight = fww->outputRect.y2 - fww->outputRect.y1;

            autoScaleX = (float) WIN_OUTPUT_W (w) / (float) apparantWidth;
            autoScaleY = (float) WIN_OUTPUT_H (w) / (float) apparantHeight;

            if (autoScaleX >= 1.0f)
                autoScaleX = 1.0f;
            if (autoScaleY >= 1.0f)
                autoScaleY = 1.0f;

            autoScaleX = autoScaleY = (autoScaleX + autoScaleY) / 2;

            /* Because we modified the scale after calculating
             * the output rect, we need to recalculate again
             */

            matrixGetIdentity (&outTransform);
            matrixScale (&outTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
            matrixTranslate(&outTransform, 
                WIN_OUTPUT_X(w) + WIN_OUTPUT_W(w)/2.0, 
                WIN_OUTPUT_Y(w) + WIN_OUTPUT_H(w)/2.0, 0.0);
            matrixRotate (&outTransform, fww->transform.angX, 1.0f, 0.0f, 0.0f);
            matrixRotate (&outTransform, fww->transform.angY, 0.0f, 1.0f, 0.0f);
            matrixRotate (&outTransform, fww->transform.angZ, 0.0f, 0.0f, 1.0f);
            matrixScale(&outTransform, autoScaleX - (1 - fww->transform.scaleX), 1.0, 0.0);
            matrixScale(&outTransform, 1.0, autoScaleY - (1 - fww->transform.scaleY), 0.0);
            matrixTranslate(&outTransform, 
                -(WIN_OUTPUT_X(w) + WIN_OUTPUT_W(w)/2.0), 
                -(WIN_OUTPUT_Y(w) + WIN_OUTPUT_H(w)/2.0), 0.0);

            FWRotateProjectVector(w, corner1, outTransform, &xScreen1, &yScreen1, &zScreen1);
            FWRotateProjectVector(w, corner2, outTransform, &xScreen2, &yScreen2, &zScreen2);
            FWRotateProjectVector(w, corner3, outTransform, &xScreen3, &yScreen3, &zScreen3);
            FWRotateProjectVector(w, corner4, outTransform, &xScreen4, &yScreen4, &zScreen4);

            fww->outputRect = FWCreateSizedRect(xScreen1, xScreen2, xScreen3, xScreen4,
                                                yScreen1, yScreen2, yScreen3, yScreen4);

        }

        float scaleX = autoScaleX - (1 - fww->transform.scaleX);
        float scaleY = autoScaleY - (1 - fww->transform.scaleY);

        /* Actually Transform the window */

	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	    /* Adjust the window in the matrix to prepare for transformation */
	    matrixScale (&wTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
	    matrixTranslate(&wTransform, 
		    WIN_REAL_X(w) + WIN_REAL_W(w)/2.0, 
		    WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0, 0.0);
        matrixRotate(&wTransform, fww->transform.angX, 1.0, 0.0, 0.0);
        matrixRotate(&wTransform, fww->transform.angY, 0.0, 1.0, 0.0);
        matrixRotate(&wTransform, fww->transform.angZ, 0.0, 0.0, 1.0);        
       
	    matrixScale(&wTransform, scaleX, 1.0, 0.0);
        matrixScale(&wTransform, 1.0, scaleY, 0.0);

	    matrixTranslate(&wTransform, 
		    -(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
		    -(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0), 0.0);

        /* Create rects for input after we've dealt
         * with output
         */

        /* It is safe to over-write variables
         * as they will not be used to calculate
         * output regions again
         */

        CompVector corneri1 = { .v = { WIN_REAL_X (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
        CompVector corneri2 = { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w), 1.0f, 1.0f } };
        CompVector corneri3 = { .v = { WIN_REAL_X (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };
        CompVector corneri4 = { .v = { WIN_REAL_X (w) + WIN_REAL_W (w), WIN_REAL_Y (w) + WIN_REAL_H (w), 1.0f, 1.0f } };

        matrixGetIdentity (&inputTransform);
        matrixScale (&inputTransform, 1.0f, 1.0f, 1.0f / w->screen->width);
        matrixTranslate(&inputTransform, 
            WIN_REAL_X(w) + WIN_REAL_W(w)/2.0, 
            WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0, 0.0);
        matrixRotate (&inputTransform, fww->transform.angX, 1.0f, 0.0f, 0.0f);
        matrixRotate (&inputTransform, fww->transform.angY, 0.0f, 1.0f, 0.0f);
        matrixRotate (&inputTransform, fww->transform.angZ, 0.0f, 0.0f, 1.0f);
        matrixScale(&inputTransform, fww->transform.scaleX, 1.0, 0.0);
        matrixScale(&inputTransform, 1.0, fww->transform.scaleY, 0.0);
        matrixTranslate(&inputTransform, 
            -(WIN_REAL_X(w) + WIN_REAL_W(w)/2.0), 
            -(WIN_REAL_Y(w) + WIN_REAL_H(w)/2.0), 0.0);

        FWRotateProjectVector(w, corneri1, inputTransform, &xScreen1, &yScreen1, &zScreen1);
        FWRotateProjectVector(w, corneri2, inputTransform, &xScreen2, &yScreen2, &zScreen2);
        FWRotateProjectVector(w, corneri3, inputTransform, &xScreen3, &yScreen3, &zScreen3);
        FWRotateProjectVector(w, corneri4, inputTransform, &xScreen4, &yScreen4, &zScreen4);

        fww->inputRect = FWCreateSizedRect(xScreen1, xScreen2, xScreen3, xScreen4,
                                            yScreen1, yScreen2, yScreen3, yScreen4);

    }

    /* Animation. We calculate how much increment
     * a window must rotate / scale per paint by
     * using the set destination attributes minus
     * the old attributes divided by the time
     * remaining.
     */

    if (fww->doAnimate)
    {
        fww->transform.angX += (float) fww->animate.steps * (fww->animate.destAngX - fww->transform.angX);
        fww->transform.angY += (float) fww->animate.steps * (fww->animate.destAngY - fww->transform.angY);
        fww->transform.angZ += (float) fww->animate.steps * (fww->animate.destAngZ - fww->transform.angZ);
        
        fww->transform.scaleX += (float) fww->animate.steps * (fww->animate.destScaleX - fww->transform.scaleX);        
        fww->transform.scaleY += (float) fww->animate.steps * (fww->animate.destScaleY - fww->transform.scaleY);
                
        fww->animate.aTimeRemaining--;
        addWindowDamage (w);
        
        if (fww->animate.aTimeRemaining <= 0 || 
             ((fww->transform.angX >= fww->animate.destAngX - 0.05 &&
              fww->transform.angX <= fww->animate.destAngX + 0.0 ) &&
             (fww->transform.angY >= fww->animate.destAngY - 0.05 &&
              fww->transform.angY <= fww->animate.destAngY + 0.05 ) &&
             (fww->transform.angZ >= fww->animate.destAngZ - 0.05 &&
              fww->transform.angZ <= fww->animate.destAngZ + 0.05 ) &&
             (fww->transform.scaleX >= fww->animate.destScaleX - 0.05 &&
              fww->transform.scaleX <= fww->animate.destScaleX + 0.05 ) &&
             (fww->transform.scaleY >= fww->animate.destScaleY - 0.05 &&
              fww->transform.scaleY <= fww->animate.destScaleY + 0.05 )))
        {
            fww->resetting = FALSE;

            fww->transform.angX = fww->animate.destAngX;
            fww->transform.angY = fww->animate.destAngY;
            fww->transform.angZ = fww->animate.destAngZ;
            fww->transform.scaleX = fww->animate.destScaleX;
            fww->transform.scaleY = fww->animate.destScaleX;
            
            fww->doAnimate = FALSE;
            fww->animate.aTimeRemaining = freewinsGetResetTime (w->screen);
            fww->animate.cTimeRemaining = freewinsGetResetTime (w->screen);
            /*if (FWCanShape (w))
                FWShapeInput (w);*/
        }
    }

    // Check if there are rotated windows
    if(fww->transform.angX != 0.0 || fww->transform.angY != 0.0 || fww->transform.angZ != 0.0 || fww->transform.scaleX != 1.0 || fww->transform.scaleY != 1.0){
        if( !fww->rotated ){
	    fws->rotatedWindows++;
	    fww->rotated = TRUE;
        }
    }else{
        if( fww->rotated ){
	    fws->rotatedWindows--;
	    fww->rotated = FALSE;
        }
    }

    if(wasCulled)
	glDisable(GL_CULL_FACE);
	

    UNWRAP(fws, w->screen, paintWindow);
    status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
    WRAP(fws, w->screen, paintWindow, FWPaintWindow);

    if(wasCulled)
	glEnable(GL_CULL_FACE);

    return status;
}

/* Paint the window axis help onto the screen */
static Bool FWPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){

    Bool wasCulled, status;
    CompTransform zTransform;
    float x, y;
    int j;

    FREEWINS_SCREEN(s);
    FREEWINS_DISPLAY(s->display);

    if(fws->rotatedWindows > 0)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP(fws, s, paintOutput);
    status = (*s->paintOutput)(s, sAttrib, transform, region, output, mask);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    // z-axis circle/*{{{*/
    if(fwd->axisHelp && fwd->focusWindow){
    	
	x = WIN_REAL_X(fwd->focusWindow) + WIN_REAL_W(fwd->focusWindow)/2.0;
	y = WIN_REAL_Y(fwd->focusWindow) + WIN_REAL_H(fwd->focusWindow)/2.0;

	wasCulled = glIsEnabled(GL_CULL_FACE);
	zTransform = *transform;

	transformToScreenSpace(s, output, -DEFAULT_Z_CAMERA, &zTransform);

	glPushMatrix();
	glLoadMatrixf(zTransform.m);

	if(wasCulled)
	    glDisable(GL_CULL_FACE);

    CompWindow *w = fwd->focusWindow;
    FREEWINS_WINDOW (w);

    if (freewinsGetShowCircle (s))
    {

	glColor4usv  (freewinsGetCircleColor (s));
	glEnable(GL_BLEND);

    //float x1, x2, y1, y2;

	glBegin(GL_POLYGON);
	for(j=0; j<360; j += 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();

	glDisable(GL_BLEND);
	glColor4usv  (freewinsGetLineColor (s));
	glLineWidth(3.0);

	glBegin(GL_LINE_LOOP);
	for(j=360; j>=0; j -= 10)
	    glVertex3f( x + 100 * cos(D2R(j)), y + 100 * sin(D2R(j)), 0.0 );
	glEnd ();

    float rad0;

    rad0 = sqrt(pow((x - WIN_REAL_X (w)), 2) + pow((y - WIN_REAL_Y (w)), 2));

	glBegin(GL_LINE_LOOP);
	for(j=360; j>=0; j -= 10)
	    glVertex3f( x + rad0 * cos(D2R(j)), y + rad0 * sin(D2R(j)), 0.0 );
	glEnd ();

    }

    /* Draw the bounding box */

    if (freewinsGetShowRegion (s))
    {

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);
    glColor4us (0x2fff, 0x2fff, 0x4fff, 0x4fff);
    glRecti (fww->outputRect.x1, fww->outputRect.y1, fww->outputRect.x2, fww->outputRect.y2);
    glColor4us (0x2fff, 0x2fff, 0x4fff, 0x9fff);
    glBegin (GL_LINE_LOOP);
    glVertex2i (fww->outputRect.x1, fww->outputRect.y1);
    glVertex2i (fww->outputRect.x2, fww->outputRect.y1);
    glVertex2i (fww->outputRect.x1, fww->outputRect.y2);
    glVertex2i (fww->outputRect.x2, fww->outputRect.y2);
    glEnd ();
    glColor4usv (defaultColor);
    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    }

    if (freewinsGetShowCross (s))
    {
	
	glColor4usv  (freewinsGetCrossLineColor (s));
	glBegin(GL_LINES);
	glVertex3f(x, y - (WIN_REAL_H (fwd->focusWindow) / 2), 0.0f);
	glVertex3f(x, y + (WIN_REAL_H (fwd->focusWindow) / 2), 0.0f);
	glEnd ();
	
	glBegin(GL_LINES);
	glVertex3f(x - (WIN_REAL_W (fwd->focusWindow) / 2), y, 0.0f);
	glVertex3f(x + (WIN_REAL_W (fwd->focusWindow) / 2), y, 0.0f);
	glEnd ();

    }

	if(wasCulled)
	    glEnable(GL_CULL_FACE);

	glColor4usv(defaultColor);
	glPopMatrix ();
    }

    return status;
}

/* Damage the Window Rect */
static Bool FWDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

    Bool status = TRUE;
    FREEWINS_DISPLAY(w->screen->display);
    FREEWINS_SCREEN(w->screen);
    FREEWINS_WINDOW(w);

    if (fww->rotated)
    {
        REGION region;

        region.rects = &region.extents;
        region.numRects = region.size = 1;

        region.extents.x1 = fww->outputRect.x1;
        region.extents.x2 = fww->outputRect.x2;
        region.extents.y1 = fww->outputRect.y1;
        region.extents.y2 = fww->outputRect.y2;

        damageScreenRegion (w->screen, &region);
    }
    else
    {
        status = FALSE;
    }

    if (fwd->axisHelp)
        damageScreen (w->screen);
        /* TODO: Calculate a region for the axisHelp */

    UNWRAP(fws, w->screen, damageWindowRect);
    status |= (*w->screen->damageWindowRect)(w, initial, rect);
    //(*w->screen->damageWindowRect)(w, initial, &fww->outputRect);
    WRAP(fws, w->screen, damageWindowRect, FWDamageWindowRect);

    // true if damaged something
    return status;
}

/* Information on window resize */
static void FWWindowResizeNotify(CompWindow *w, int dx, int dy, int dw, int dh){

    FREEWINS_WINDOW(w);
    FREEWINS_SCREEN(w->screen);

    fww->midX += dw;
    fww->midY += dh;

    /*if (FWCanShape (w))
        FWShapeInput (w);*/

    UNWRAP(fws, w->screen, windowResizeNotify);
    (*w->screen->windowResizeNotify)(w, dx, dy, dw, dh);
    WRAP(fws, w->screen, windowResizeNotify, FWWindowResizeNotify);
}

static void
FWWindowMoveNotify (CompWindow *w,
		       int        dx,
		       int        dy,
		       Bool       immediate)
{
    FREEWINS_SCREEN (w->screen);

    CompWindow *useWindow;

    useWindow = FWGetRealWindow (w); /* Did we move an IPW and not the actual window? */
    if (useWindow)
        moveWindow (useWindow, dx, dy, TRUE, FALSE);
    else
        FWAdjustIPW (w); /* We moved a window but not the IPW, so adjust it */

    UNWRAP (fws, w->screen, windowMoveNotify);
    (*w->screen->windowMoveNotify) (w, dx, dy, immediate);
    WRAP (fws, w->screen, windowMoveNotify, FWWindowMoveNotify);
}

/* ------ Actions -------------------------------------------------------*/

/* Initiate Mouse Rotation */
static Bool initiateFWRotate (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s)
    {

    FREEWINS_SCREEN (s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_fleur);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");

    }
    
    
    if(useW){
	FREEWINS_WINDOW(useW);
	
	fww->allowRotation = TRUE;
	fww->allowScaling = FALSE;
	
	fwd->grabWindow = useW;
	
	fwd->grab = grabRotate;
	
	fwd->oldX = fwd->click_root_x;
	fwd->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->midX;
	dy = fwd->click_win_y - fww->midY;

	fww->grabLeft = (dx > 0 ? FALSE : TRUE);
	fww->grabTop = (dy > 0 ? FALSE : TRUE);

	dx = ABS(dx);
	dy = ABS(dy);
	
	if (!freewinsGetAlwaysZ (w->screen)) // Check the option
	{
	    if( (dx>dy?dx:dy) > 100 ) // Check the position
	        {
	            fww->zaxis = TRUE;
	        }
	    else
	        {
	            fww->zaxis = FALSE;
	        }
	}
	else // It's always 3D rotation
	{
	    fww->zaxis = FALSE;
	}
	}
	
    return TRUE;
}

/* Initiate Scaling */
static Bool initiateFWScale (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    float dx, dy;
    
    FREEWINS_DISPLAY(d);

    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);
    useW = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s)
    {

	FREEWINS_SCREEN(s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

	fws->rotateCursor = XCreateFontCursor (s->display->display, XC_plus);	

	if(!otherScreenGrabExist(s, "freewins", 0))
	    if(!fws->grabIndex)
		fws->grabIndex = pushScreenGrab(s, fws->rotateCursor, "freewins");

    }
    
    
    if(useW){
	FREEWINS_WINDOW(useW);
	
	fww->allowScaling = TRUE;
	fww->allowRotation = FALSE;
	
	fwd->grabWindow = useW;
	
	/* Find out the corner we clicked in */
	
	/* Check for Y axis clicking (Top / Bottom) */
	if (fwd->click_win_y > fww->midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_win_x > fww->midX)
	        fww->corner = CornerBottomRight;
	    else if (fwd->click_win_x < fww->midX)
	        fww->corner = CornerBottomLeft;
	}
	else if (fwd->click_win_y < fww->midY)
	{
	    /* Check for X axis clicking (Left / Right) */
	    if (fwd->click_win_x > fww->midX)
	        fww->corner = CornerTopRight;
	    else if (fwd->click_win_x < fww->midX)
	        fww->corner = CornerTopLeft;
	}
	
	fwd->grab = grabScale;

	fwd->oldX = fwd->click_root_x;
	fwd->oldY = fwd->click_root_y;

	dx = fwd->click_win_x - fww->midX;
	dy = fwd->click_win_y - fww->midY;

	fww->grabLeft = (dx > 0 ? FALSE : TRUE);
	fww->grabTop = (dy > 0 ? FALSE : TRUE);

	dx = ABS(dx);
	dy = ABS(dy);
	
	fww->zaxis = TRUE;	
	
	}
    
    return TRUE;
}

#define GET_WINDOW \
    CompWindow *tW, *w; \
    CompScreen *s; \
    Window xid; \
    FWWindowInputInfo *info; \
    xid = getIntOptionNamed (option, nOption, "window", 0); \
    tW = findWindowAtDisplay (d, xid); \
    w = tW; \
    s = findScreenAtDisplay (d, getIntOptionNamed (option, nOption, "root", 0)); \
    if (s) \
    { \
	FREEWINS_SCREEN(s); \
    for (info = fws->transformedWindows; info; info = info->next) \
    { \
        if (tW->id == info->ipw) \
        w = FWGetRealWindow (tW); \
        break; \
    } \
    } \


/* Repetitive Stuff */

static void
FWSetPrepareRotation (CompWindow *w, float dx, float dy, float dz, float dsu, float dsd)
{
    FREEWINS_WINDOW (w);

    fww->animate.oldAngX = fww->transform.angX; 
    fww->animate.oldAngY = fww->transform.angY; 
    fww->animate.oldAngZ = fww->transform.angZ;

    fww->animate.oldScaleX = fww->transform.scaleX; 
    fww->animate.oldScaleY = fww->transform.scaleY;

    fww->animate.destAngX = fww->transform.angX + dx; 
    fww->animate.destAngY = fww->transform.angY + dy; 
    fww->animate.destAngZ = fww->transform.angZ + dz;

    fww->animate.destScaleX = fww->transform.scaleX + dsu; 
    fww->animate.destScaleY = fww->transform.scaleY + dsd;

    fww->animate.aTimeRemaining = freewinsGetRotateIncrementTime (w->screen); 
    fww->animate.cTimeRemaining = freewinsGetRotateIncrementTime (w->screen); 
    fww->doAnimate = TRUE; // Start animating
}

#define ROTATE_INC freewinsGetRotateIncrementAmount (w->screen)
#define NEG_ROTATE_INC freewinsGetRotateIncrementAmount (w->screen) *-1

#define SCALE_INC freewinsGetScaleIncrementAmount (w->screen)
#define NEG_SCALE_INC freewinsGetScaleIncrementAmount (w->screen) *-1

static Bool FWRotateUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, 0, ROTATE_INC, 0, 0, 0);
    
    return TRUE;
    
}

static Bool FWRotateDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, 0, NEG_ROTATE_INC, 0, 0, 0);
    
    return TRUE;
    
}

static Bool FWRotateLeft (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {

    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, ROTATE_INC, 0, 0, 0, 0);
    
    return TRUE;
    
}

static Bool FWRotateRight (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, NEG_ROTATE_INC, 0, 0, 0, 0);
    
    return TRUE;
    
}

static Bool FWRotateClockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, 0, 0, ROTATE_INC, 0, 0);
    
    return TRUE;
    
}

static Bool FWRotateCounterclockwise (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
        FWSetPrepareRotation (w, 0, 0, NEG_ROTATE_INC, 0, 0);
    
    return TRUE;
    
}

static Bool FWScaleUp (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, SCALE_INC, SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        /*if (FWCanShape (w))
        FWShapeInput (w);*/
    }
    
    return TRUE;
    
}

static Bool FWScaleDown (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption) {
    
    GET_WINDOW
    if (w)
    {
        FWSetPrepareRotation (w, 0, 0, 0, NEG_SCALE_INC, NEG_SCALE_INC);
        addWindowDamage (w); // Smoothen Painting
        /*if (FWCanShape (w))
            FWShapeInput (w);*/
    }
    
    return TRUE;
    
}

/* Callable action to rotate a window to the angle provided
 * x: Set angle to x degrees
 * y: Set angle to y degrees
 * z: Set angle to z degrees
 * window: The window to apply the transformation to
 */
static Bool freewinsRotateWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        float x, y, z;
        
        y = getFloatOptionNamed(option, nOption, "x", 0.0f);
        x = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);
        
        fww->transform.angX = x;
        fww->transform.angY = y;
        fww->transform.angZ = z;

        addWindowDamage (w);
        
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/* Callable action to increment window rotation by the angles provided
 * x: Increment angle by x degrees
 * y: Increment angle by y degrees
 * z: Increment angle by z degrees
 * window: The window to apply the transformation to
 */
static Bool freewinsIncrementRotateWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    //FREEWINS_DISPLAY(d);
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        float x, y, z;
        
        x = getFloatOptionNamed(option, nOption, "x", 0.0f);
        y = getFloatOptionNamed(option, nOption, "y", 0.0f);
        z = getFloatOptionNamed(option, nOption, "z", 0.0f);
        
        /* Respect dx, dy, dz, first */
        fww->transform.angX += x;
        fww->transform.angY += y;
        fww->transform.angZ += z;

        addWindowDamage (w);
        
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/* Callable action to scale a window to the scale provided
 * x: Set scale to x factor
 * y: Set scale to y factor
 * window: The window to apply the transformation to
 */
static Bool freewinsScaleWindow (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
	CompWindow *w;

    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    
    if (w)
    {
        FREEWINS_WINDOW(w);
        
        fww->transform.scaleX = getFloatOptionNamed(option, nOption, "x", 0.0f);
        fww->transform.scaleY = getFloatOptionNamed(option, nOption, "y", 0.0f);

        addWindowDamage (w);
    }
    else
    {
        return FALSE;
    }
    
    /*if (FWCanShape (w))
        FWShapeInput (w);*/
    
    return TRUE;
}

/* Toggle Axis-Help Display */
static Bool toggleFWAxis (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){

    CompScreen *s;

    FREEWINS_DISPLAY(d);

    s = findScreenAtDisplay(d, getIntOptionNamed(option, nOption, "root", 0));

    fwd->axisHelp = !fwd->axisHelp;
    if (s)
        damageScreen (s);

    return TRUE;
}

/* Reset the Rotation and Scale to 0 and 1 */
/* TODO: Rename to resetFWTransform */
static Bool resetFWRotation (CompDisplay *d, CompAction *action, 
	CompActionState state, CompOption *option, int nOption){
    
    CompWindow* w;
    CompWindow *useW;
    CompScreen* s;
    FWWindowInputInfo *info;
    Window xid, root;
    
    w = findWindowAtDisplay (d, getIntOptionNamed(option, nOption, "window", 0));
    useW = findWindowAtDisplay (d, xid);
    
    xid = getIntOptionNamed (option, nOption, "window", 0);
    w = findWindowAtDisplay (d, xid);

    root = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, root);

    if (s)
    {

    FREEWINS_SCREEN (s);

    for (info = fws->transformedWindows; info; info = info->next)
    {
        if (w->id == info->ipw)
        /* The window we just grabbed was actually
         * an IPW, get the real window instead
         */
        useW = FWGetRealWindow (w);
    }

    }

    if(useW){
	FREEWINS_WINDOW(useW);

    addWindowDamage (useW);

	if( fww->rotated ){
	    FREEWINS_SCREEN(useW->screen);
	    fws->rotatedWindows--;
	    fww->rotated = FALSE;
	}

    if (FWCanShape (useW))
        FWHandleFWInputInfo (useW);

    // Set values to animate from
	fww->animate.oldAngX = fww->transform.angX;
    fww->animate.oldAngY = fww->transform.angY;
    fww->animate.oldAngZ = fww->transform.angZ;
	fww->animate.oldScaleX = fww->transform.scaleX;
	fww->animate.oldScaleY = fww->transform.scaleY;
	
	// Set values to animate to
	fww->animate.destAngX = 0.0f;
	fww->animate.destAngY = 0.0f;
	fww->animate.destAngZ = 0.0f;
	
	fww->animate.destScaleX = 1.0f;
	fww->animate.destScaleY = 1.0f;
	
	// Reset unsnapped values too
	
    fww->transform.unsnapAngX = 0.0f;
    fww->transform.unsnapAngY = 0.0f;
    fww->transform.unsnapAngZ = 0.0f;
    
    fww->transform.unsnapScaleX = 1.0f;
    fww->transform.unsnapScaleY = 1.0f;

	
	fww->doAnimate = TRUE;
	fww->resetting = TRUE;
    }
    
    return TRUE;
}

/* ------ Plugin Initialisation ---------------------------------------*/

/* Window initialisation / cleaning */
static Bool freewinsInitWindow(CompPlugin *p, CompWindow *w){
    FWWindow *fww;
    FREEWINS_SCREEN(w->screen);
    FREEWINS_DISPLAY(w->screen->display);

    if( !(fww = (FWWindow*)malloc( sizeof(FWWindow) )) )
	return FALSE;

    fww->transform.angX = 0.0;
    fww->transform.angY = 0.0;
    fww->transform.angZ = 0.0;

    fww->midX = WIN_REAL_W(w)/2.0;
    fww->midY = WIN_REAL_H(w)/2.0;

    fww->outputRect.x1 = WIN_OUTPUT_X (w);
    fww->outputRect.x2 = WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w);
    fww->outputRect.y1 = WIN_OUTPUT_Y (w);
    fww->outputRect.y2 = WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w);

    fwd->grab = grabNone;
    fww->zaxis = FALSE;

    fww->rotated = FALSE;
    
    // Don't allow anything yet
    fww->allowScaling = FALSE;
    fww->allowRotation = FALSE;
    fww->doAnimate = FALSE;
    fww->resetting = FALSE;
    
    // Don't allow incorrect window drawing as soon as the plugin is started
    
    fww->transform.scaleX = 1.0;
    fww->transform.scaleY = 1.0;
    
    fww->transform.unsnapScaleX = 1.0;
    fww->transform.unsnapScaleY = 1.0;
    
    fww->animate.aTimeRemaining = freewinsGetResetTime (w->screen);
    fww->animate.cTimeRemaining = freewinsGetResetTime (w->screen);

    fww->input = NULL;

    w->base.privates[fws->windowPrivateIndex].ptr = fww;
    
    // Shape window back to normal
    /*if (FWCanShape (w))
        FWShapeInput (w); - disabled as it causes problems*/

    return TRUE;
}

static void freewinsFiniWindow(CompPlugin *p, CompWindow *w){

    FREEWINS_WINDOW(w);
    FREEWINS_DISPLAY(w->screen->display);
    
    /* Shape window back to normal */
    fww->transform.scaleX = 1.0f;
    fww->transform.scaleY = 1.0f;
    
    /*if (FWCanShape (w))
        FWShapeInput (w);*/

    if(fwd->grabWindow == w){
	fwd->grabWindow = NULL;
    }

   free(fww); 
}

/* Screen initialization / cleaning */
static Bool freewinsInitScreen(CompPlugin *p, CompScreen *s){
    FWScreen *fws;

    FREEWINS_DISPLAY(s->display);

    if( !(fws = (FWScreen*)malloc( sizeof(FWScreen) )) )
	return FALSE;

    if( (fws->windowPrivateIndex = allocateWindowPrivateIndex(s)) < 0){
	free(fws);
	return FALSE;
    }

    fws->grabIndex = 0;
    fws->rotatedWindows = 0;
    fws->transformedWindows = NULL;

    s->base.privates[fwd->screenPrivateIndex].ptr = fws;
    
    WRAP(fws, s, preparePaintScreen, FWPreparePaintScreen);
    WRAP(fws, s, paintWindow, FWPaintWindow);
    WRAP(fws, s, paintOutput, FWPaintOutput);

    WRAP(fws, s, damageWindowRect, FWDamageWindowRect);

    WRAP(fws, s, windowResizeNotify, FWWindowResizeNotify);
    WRAP(fws, s, windowMoveNotify, FWWindowMoveNotify);

    return TRUE;
}

static void freewinsFiniScreen(CompPlugin *p, CompScreen *s){

    FREEWINS_SCREEN(s);

    freeWindowPrivateIndex(s, fws->windowPrivateIndex);

    UNWRAP(fws, s, preparePaintScreen);
    UNWRAP(fws, s, paintWindow);
    UNWRAP(fws, s, paintOutput);

    UNWRAP(fws, s, damageWindowRect);

    UNWRAP(fws, s, windowResizeNotify);
    UNWRAP(fws, s, windowMoveNotify);
    free(fws);
}

/* Display initialization / cleaning */
static Bool freewinsInitDisplay(CompPlugin *p, CompDisplay *d){

    FWDisplay *fwd; 

    if( !(fwd = (FWDisplay*)malloc( sizeof(FWDisplay) )) )
	return FALSE;
    
    // Set variables correctly
    fwd->grabWindow = 0;
    fwd->axisHelp = FALSE;
    fwd->focusWindow = 0;
     
    if( (fwd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(fwd);
	return FALSE;
    }
    
    // Spit out a warning if there is no shape extension
    if (!d->shapeExtension)
        compLogMessage(d, "freewins", CompLogLevelInfo, "No input shaping extension. Input shaping disabled");


    /* BCOP Action initiation */
    freewinsSetInitiateRotationButtonInitiate(d, initiateFWRotate);
    freewinsSetInitiateScaleButtonInitiate(d, initiateFWScale);
    freewinsSetResetButtonInitiate(d, resetFWRotation);
    freewinsSetResetKeyInitiate(d, resetFWRotation);
    freewinsSetToggleAxisKeyInitiate(d, toggleFWAxis);
    
    // Rotate / Scale Up Down Left Right
    freewinsSetScaleUpKeyInitiate(d, FWScaleUp);
    freewinsSetScaleDownKeyInitiate(d, FWScaleDown);
    freewinsSetScaleUpButtonInitiate(d, FWScaleUp);
    freewinsSetScaleDownButtonInitiate(d, FWScaleDown);

    freewinsSetRotateUpKeyInitiate(d, FWRotateUp);
    freewinsSetRotateDownKeyInitiate(d, FWRotateDown);
    freewinsSetRotateLeftKeyInitiate(d, FWRotateLeft);
    freewinsSetRotateRightKeyInitiate(d, FWRotateRight);
    freewinsSetRotateCKeyInitiate(d, FWRotateClockwise);
    freewinsSetRotateCcKeyInitiate(d, FWRotateCounterclockwise);
    
    freewinsSetRotateInitiate (d, freewinsRotateWindow);
    freewinsSetIncrementRotateInitiate (d, freewinsIncrementRotateWindow);
    freewinsSetScaleInitiate (d, freewinsScaleWindow);
    
    d->base.privates[displayPrivateIndex].ptr = fwd;
    WRAP(fwd, d, handleEvent, FWHandleEvent);
    
    return TRUE;
}

static void freewinsFiniDisplay(CompPlugin *p, CompDisplay *d){

    FREEWINS_DISPLAY(d);
    
    freeScreenPrivateIndex(d, fwd->screenPrivateIndex);

    UNWRAP(fwd, d, handleEvent);

    free(fwd);
}

static CompBool
freewinsInitObject (CompPlugin *p,
		 CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	    (InitPluginObjectProc) 0, /* InitCore */
	    (InitPluginObjectProc) freewinsInitDisplay,
	    (InitPluginObjectProc) freewinsInitScreen, 
	    (InitPluginObjectProc) freewinsInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
freewinsFiniObject (CompPlugin *p,
		 CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* InitCore */
	(FiniPluginObjectProc) freewinsFiniDisplay,
	(FiniPluginObjectProc) freewinsFiniScreen, 
	(FiniPluginObjectProc) freewinsFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

/* Plugin initialization / cleaning */
static Bool freewinsInit(CompPlugin *p){
    
    
    if( (displayPrivateIndex = allocateDisplayPrivateIndex()) < 0 )
	return FALSE;

	compAddMetadataFromFile (&freewinsMetadata, p->vTable->name);

    return TRUE;
}


static void freewinsFini(CompPlugin *p){

    if(displayPrivateIndex >= 0)
	freeDisplayPrivateIndex( displayPrivateIndex );
}

/* Plugin implementation export */
CompPluginVTable freewinsVTable = {
    "freewins",
    0,
    freewinsInit,
    freewinsFini,
    freewinsInitObject,
    freewinsFiniObject,
    0,
    0
};

CompPluginVTable *getCompPluginInfo (void){ return &freewinsVTable; }

